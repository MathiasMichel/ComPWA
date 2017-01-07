/*
 * FitResult.cpp
 *
 *  Created on: Jan 15, 2014
 *      Author: weidenka
 */

#include <numeric>
#include <math.h>

#include <gsl/gsl_sf_gamma.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>

#include <boost/numeric/ublas/symmetric.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>

#include "Optimizer/Minuit2/MinuitResult.hpp"
#include "Core/ProgressBar.hpp"

namespace ComPWA {
namespace Optimizer {
namespace Minuit2 {

using namespace boost::log;
using namespace ROOT::Minuit2;

using ComPWA::Estimator::Estimator;

int multivariateGaussian(const gsl_rng *rnd, const int vecSize,
    const gsl_vector *in, const gsl_matrix *cov, gsl_vector *res) {
  gsl_matrix *tmpM = gsl_matrix_alloc(vecSize, vecSize);
  gsl_matrix_memcpy(tmpM, cov);
  gsl_linalg_cholesky_decomp(tmpM);
  for (unsigned int i = 0; i < vecSize; i++)
    gsl_vector_set(res, i, gsl_ran_ugaussian(rnd));

  gsl_blas_dtrmv(CblasLower, CblasNoTrans, CblasNonUnit, tmpM, res);
  gsl_vector_add(res, in);
  gsl_matrix_free(tmpM);

  return 0;
}

MinuitResult::MinuitResult(std::shared_ptr<ControlParameter> esti,
    FunctionMinimum result) :
    useCorrelatedErrors(0) {
  estimator = std::static_pointer_cast<Estimator>(esti);
  _amp = estimator->getAmplitude();
  init(result);
}

void MinuitResult::setResult(std::shared_ptr<ControlParameter> esti,
    FunctionMinimum result) {
  estimator = std::static_pointer_cast<Estimator>(esti);
  _amp = estimator->getAmplitude();
  init(result);
}

void MinuitResult::init(FunctionMinimum min) {
  nRes = 0;
  MnUserParameterState minState = min.UserState();
  using namespace boost::numeric::ublas;

  if (minState.HasCovariance()) {
    MnUserCovariance minuitCovMatrix = minState.Covariance();
    /* Size of Minuit covariance vector is given by dim*(dim+1)/2.
     * dim is the dimension of the covariance matrix.
     * The dimension can therefore be calculated as dim = -0.5+-0.5 sqrt(8*size+1)
     */
    unsigned int dim = minuitCovMatrix.Nrow();
    globalCC = minState.GlobalCC().GlobalCC();
    symmetric_matrix<double, upper> covMatrix(dim, dim);
    symmetric_matrix<double, upper> corrMatrix(dim, dim);
    //	if(minuitCovM.size()==dim*(dim+1)/2){
    for (unsigned i = 0; i < covMatrix.size1(); ++i)
      for (unsigned j = i; j < covMatrix.size2(); ++j) {
        double entry = minuitCovMatrix(j, i);
        covMatrix(i, j) = entry;
        if (i == j)
          variance.push_back(sqrt(entry));
      }
    for (unsigned i = 0; i < covMatrix.size1(); ++i)
      for (unsigned j = i; j < covMatrix.size2(); ++j) {
        double denom = variance[i] * variance[j];
        corrMatrix(i, j) = covMatrix(i, j) / denom;
      }
    cov = covMatrix;
    corr = corrMatrix;
  }
  else
    BOOST_LOG_TRIVIAL(error)<<"MinuitResult: no valid correlation matrix available!";
  initialLH = -1;
  finalLH = minState.Fval();
  edm = minState.Edm();
  isValid = min.IsValid();
  covPosDef = min.HasPosDefCovar();
  hasValidParameters = min.HasValidParameters();
  hasValidCov = min.HasValidCovariance();
  hasAccCov = min.HasAccurateCovar();
  hasReachedCallLimit = min.HasReachedCallLimit();
  hesseFailed = min.HesseFailed();
  errorDef = min.Up();
  nFcn = min.NFcn();

  const gsl_rng_type * T;
  gsl_rng_env_setup();
  T = gsl_rng_default;
  r = gsl_rng_alloc(T);

  useCorrelatedErrors = 0;
  if (_amp && _amp->hasTree())
    setUseTree(1);
  return;

}

void MinuitResult::genSimpleOutput(std::ostream& out) {
  for (unsigned int o = 0; o < finalParameters.GetNDouble(); o++) {
    std::shared_ptr<DoubleParameter> outPar =
        finalParameters.GetDoubleParameter(o);
    out << outPar->GetValue() << " ";
    if (outPar->HasError())
      out << outPar->GetError() << " ";
  }
  out << "\n";

  return;
}

void MinuitResult::smearParameterList(ParameterList& newParList) {
  unsigned int nFree = 0;
  for (unsigned int o = 0; o < finalParameters.GetNDouble(); o++)
    if (!finalParameters.GetDoubleParameter(o)->IsFixed())
      nFree++;
  unsigned int t = 0;
  gsl_vector* oldPar = gsl_vector_alloc(nFree);
  for (unsigned int o = 0; o < finalParameters.GetNDouble(); o++) {
    std::shared_ptr<DoubleParameter> outPar =
        finalParameters.GetDoubleParameter(o);
    if (outPar->IsFixed())
      continue;
    gsl_vector_set(oldPar, t, outPar->GetValue());
    t++;
  }
  gsl_matrix* gslCov = gsl_matrix_alloc(nFree, nFree);
  for (unsigned int i = 0; i < cov.size1(); i++)
    for (unsigned int j = 0; j < cov.size2(); j++)
      gsl_matrix_set(gslCov, i, j, cov(i, j));

  gsl_vector* newPar = gsl_vector_alloc(nFree);
  multivariateGaussian(r, nFree, oldPar, gslCov, newPar);    //generate set of smeared parameters

  newParList = ParameterList(finalParameters);    //deep copy of finalParameters
  t = 0;
  for (unsigned int o = 0; o < newParList.GetNDouble(); o++) {
    std::shared_ptr<DoubleParameter> outPar = newParList.GetDoubleParameter(o);
    if (outPar->IsFixed())
      continue;
    outPar->SetValue(newPar->data[t]);    //set floating values to smeard values
    t++;
  }
}

void MinuitResult::calcFractionError() {
  if (fractionList.GetNDouble() != _amp->getNumberOfResonances())
    throw std::runtime_error(
        "MinuitResult::calcFractionError() parameterList empty! Calculate fit fractions first!");
  nRes = fractionList.GetNDouble();
  if (useCorrelatedErrors) {
    /* Exact error calculation */
    unsigned int numberOfSets = 1000;
    BOOST_LOG_TRIVIAL(info)<< "Calculating errors of fit fractions using "<<numberOfSets<<" sets of parameters...";
    std::vector<ParameterList> fracVect;
    progressBar bar(numberOfSets);
    for (unsigned int i = 0; i < numberOfSets; i++) {
      bar.nextEvent();
      ParameterList newPar;
      smearParameterList(newPar);
      _amp->setParameterList(newPar);    //smear all free parameters according to cov matrix
      ParameterList tmp;
      calcFraction(tmp);
      fracVect.push_back(tmp);
    }
    //Calculate standart deviation
    for (unsigned int o = 0; o < nRes; o++) {
      double mean = 0, sqSum = 0., stdev = 0;
      for (unsigned int i = 0; i < fracVect.size(); i++) {
        double tmp = fracVect.at(i).GetDoubleParameter(o)->GetValue();
        mean += tmp;
        sqSum += tmp * tmp;
      }
      unsigned int s = fracVect.size();
      sqSum /= s;
      mean /= s;
      stdev = std::sqrt(sqSum - mean * mean);    //this is crosscecked with the RMS of the distribution
      fractionList.GetDoubleParameter(o)->SetError(stdev);
    }
    //		std::cout<<"frac error: ";
    //			for(unsigned int i=0; i<fracError.size();i++) std::cout<<fracError.at(i)<<" ";
    //			std::cout<<std::endl;
    _amp->setParameterList(finalParameters);    //set correct fit result
  }
  return;
}

void MinuitResult::calcFraction() {
  if (!fractionList.GetNDouble()) {
    calcFraction(fractionList);
    calcFractionError();
  }
  else
    BOOST_LOG_TRIVIAL(warning)<< "MinuitResult::calcFractions() fractions already calculated. Skip!";
  }

void MinuitResult::calcFraction(ParameterList& parList) {
  if (!_amp)
    throw std::runtime_error(
        "MinuitResult::calcFractions() | no amplitude set, can't calculate fractions!");
  if (parList.GetNDouble())
    throw std::runtime_error(
        "MinuitResult::calcFractions() | ParameterList not empty!");

  double norm = -1;
  ParameterList currentPar;
  _amp->copyParameterList(currentPar);
  //	if(!useTree) norm = _amp->integral();
  //	else {//if we have a tree, use it. Much faster especially in case of correlated errors in calcFractionError()
  //		std::shared_ptr<FunctionTree> tree = estimator->getTree();
  //		tree->recalculate();
  //		double phspVolume = Kinematics::instance()->getPhspVolume();
  //		/*We need the intensity over the PHSP without efficiency correction. Therefore we
  //		 * access node 'Amplitude' and sum up its values.*/
  //		std::shared_ptr<TreeNode> amplitudeNode = tree->head()->getChildNode("Amplitude_Phsp");
  //		if(!amplitudeNode){
  //			BOOST_LOG_TRIVIAL(error)<<"MinuitResult::calcFraction() : Can't find node 'Amplitude_Phsp' in tree!";
  //			throw BadParameter("Node not found!");
  //		}
  //		std::shared_ptr<MultiComplex> normPar = std::dynamic_pointer_cast<MultiComplex>(amplitudeNode->getValue());//node 'Amplitude'
  //		unsigned int numPhspEvents = normPar->GetNValues();
  //		for(unsigned int i=0; i<numPhspEvents;i++)
  //			norm+=std::norm(normPar->GetValue(i));
  //		norm = norm*phspVolume/numPhspEvents; //correct calculation of normalization
  //	}

  //in case of unbinned efficiency correction to tree does not provide an integral w/o efficiency correction
  norm = _amp->integral();
  if (norm < 0)
    throw std::runtime_error(
        "MinuitResult::calcFraction() normalization can't be calculated");
  BOOST_LOG_TRIVIAL(debug)<<"MinuitResult::calcFraction() norm="<<norm;
  nRes = _amp->getNumberOfResonances();
  for (unsigned int i = 0; i < nRes; i++) {    //fill matrix
    double resInt = _amp->getAmpIntegral(i);    //this is simply the factor 2J+1, because the resonance is already normalized
    std::string resName = _amp->getNameOfResonance(i);
    std::shared_ptr<DoubleParameter> magPar = currentPar.GetDoubleParameter(
        "mag_" + resName);
    double mag = magPar->GetValue();    //value of magnitude
    double magError = 0;
    if (magPar->HasError())
      magError = magPar->GetError();    //error of magnitude
    parList.AddParameter(
        std::shared_ptr<DoubleParameter>(
            new DoubleParameter(resName + "_FF", mag * mag * resInt / norm,
                2 * mag * resInt / norm * magError)));
  }
  return;
}

void MinuitResult::genOutput(std::ostream& out, std::string opt) {
  bool printTrue = 0;
  bool printParam = 1, printCorrMatrix = 1, printCovMatrix = 1;
  if (opt == "P") {    //print only parameters
    printCorrMatrix = 0;
    printCovMatrix = 0;
  }
  if (trueParameters.GetNParameter())
    printTrue = 1;
  out << std::endl;
  out << "--------------MINUIT2 FIT RESULT----------------" << std::endl;
  if (!isValid)
    out << "		*** MINIMUM NOT VALID! ***" << std::endl;
  out << std::setprecision(10);
  out << "Initial Likelihood: " << initialLH << std::endl;
  out << "Final Likelihood: " << finalLH << std::endl;

  out << "Estimated distance to minimumn: " << edm << std::endl;
  out << "Error definition: " << errorDef << std::endl;
  out << "Number of calls: " << nFcn << std::endl;
  if (hasReachedCallLimit)
    out << "		*** LIMIT OF MAX CALLS REACHED! ***" << std::endl;
  out << "CPU Time : " << time / 60 << "min" << std::endl;
  out << std::setprecision(5) << std::endl;

  if (!hasValidParameters)
    out << "		*** NO VALID SET OF PARAMETERS! ***" << std::endl;
  if (printParam) {
    out << "PARAMETERS:" << std::endl;
    TableFormater* tableResult = new TableFormater(&out);
    printFitParameters(tableResult);
  }

  if (!hasValidCov)
    out << "		*** COVARIANCE MATRIX NOT VALID! ***" << std::endl;
  if (!hasAccCov)
    out << "		*** COVARIANCE MATRIX NOT ACCURATE! ***" << std::endl;
  if (!covPosDef)
    out << "		*** COVARIANCE MATRIX NOT POSITIVE DEFINITE! ***" << std::endl;
  if (hesseFailed)
    out << "		*** HESSE FAILED! ***" << std::endl;
  if (hasValidCov) {
    unsigned int n = 0;
    if (printCovMatrix) {
      out << "COVARIANCE MATRIX:" << std::endl;
      TableFormater* tableCov = new TableFormater(&out);
      printCorrelationMatrix(tableCov);
    }
    if (printCorrMatrix) {
      out << "CORRELATION MATRIX:" << std::endl;
      TableFormater* tableCorr = new TableFormater(&out);
      printCorrelationMatrix(tableCorr);
    }
  }
  out << "FIT FRACTIONS:" << std::endl;
  TableFormater* fracTable = new TableFormater(&out);
  printFitFractions(fracTable);    //calculate and print fractions if amplitude is set

  double penalty = estimator->calcPenalty();
  out << std::setprecision(10);
  out << "FinalLH w/o penalty: " << finalLH - penalty << std::endl;
  /* The Akaike (AIC) and Bayesian (BIC) information criteria are described in
   * Schwarz, Anals of Statistics 6 No.2: 461-464 (1978)
   * and
   * IEEE Transacrions on Automatic Control 19, No.6:716-723 (1974) */
  out << "AIC: " << calcAIC() << std::endl;
  out << "BIC: " << calcBIC() << std::endl;
  out << std::endl;
  out << std::setprecision(5);

  return;
}

double MinuitResult::calcAIC() {
  if (!fractionList.GetNDouble())
    calcFraction();
  double r = 0;
  for (int i = 0; i < fractionList.GetNDouble(); i++) {
    double val = fractionList.GetDoubleParameter(i)->GetValue();
    if (val > 0.001)
      r += val;
  }
  return (finalLH + 2 * r);
}
double MinuitResult::calcBIC() {
  if (!fractionList.GetNDouble())
    calcFraction();
  double r = 0;
  for (int i = 0; i < fractionList.GetNDouble(); i++) {
    double val = fractionList.GetDoubleParameter(i)->GetValue();
    if (val > 0.001)
      r += val;
  }
  return (finalLH + r * std::log(estimator->getNEvents()));
}

void MinuitResult::printFitFractions(TableFormater* fracTable) {
  BOOST_LOG_TRIVIAL(info)<< "Calculating fit fractions...";
  calcFraction();
  double sum, sumErrorSq;

  //print matrix
  fracTable->addColumn("Resonance",15);//add empty first column
  fracTable->addColumn("Fraction",15);//add empty first column
  fracTable->addColumn("Error",15);//add empty first column
  fracTable->addColumn("Significance",15);//add empty first column
  fracTable->header();
  for(unsigned int i=0;i<fractionList.GetNDouble(); ++i) {
    std::shared_ptr<DoubleParameter> tmpPar = fractionList.GetDoubleParameter(i);
    *fracTable << tmpPar->GetName()
    << tmpPar->GetValue()
    << tmpPar->GetError()    //assume symmetric errors here
    << std::abs(tmpPar->GetValue()/tmpPar->GetError());
    sum += tmpPar->GetValue();
    sumErrorSq += tmpPar->GetError()*tmpPar->GetError();
  }
  fracTable->delim();
  *fracTable << "Total" << sum << sqrt(sumErrorSq) << " ";
  fracTable->footer();

  return;
}

void MinuitResult::printCorrelationMatrix(TableFormater* tableCorr) {
  if (!hasValidCov)
    return;
  tableCorr->addColumn(" ", 15);    //add empty first column
  tableCorr->addColumn("GlobalCC", 10);    //global correlation coefficient
  for (unsigned int o = 0; o < finalParameters.GetNDouble(); o++) {
    std::shared_ptr<DoubleParameter> ppp = finalParameters.GetDoubleParameter(
        o);
    if (ppp->IsFixed())
      continue;
    tableCorr->addColumn(ppp->GetName(), 15);    //add columns in correlation matrix
  }

  tableCorr->header();
  unsigned int n = 0;
  for (unsigned int o = 0; o < finalParameters.GetNDouble(); o++) {
    std::shared_ptr<DoubleParameter> ppp = initialParameters.GetDoubleParameter(
        o);
    std::shared_ptr<DoubleParameter> ppp2 = finalParameters.GetDoubleParameter(
        o);
    if (ppp->IsFixed())
      continue;
    *tableCorr << ppp->GetName();
    //				if(globalCC.size()>o)
    *tableCorr << globalCC[n];    //TODO: check if emtpy (don't know how this happened, but it did :)
    for (unsigned int t = 0; t < corr.size1(); t++) {
      if (n >= corr.size2()) {
        *tableCorr << " ";
        continue;
      }
      if (t >= n)
        *tableCorr << corr(n, t);
      else
        *tableCorr << "";
    }
    n++;
  }
  tableCorr->footer();
  return;
}

void MinuitResult::printCovarianceMatrix(TableFormater* tableCov) {
  if (!hasValidCov)
    return;
  tableCov->addColumn(" ", 15);    //add empty first column
  tableCov->addColumn("GlobalCC", 10);    //global correlation coefficient
  for (unsigned int o = 0; o < finalParameters.GetNDouble(); o++) {
    std::shared_ptr<DoubleParameter> ppp = finalParameters.GetDoubleParameter(
        o);
    if (ppp->IsFixed())
      continue;
    tableCov->addColumn(ppp->GetName(), 15);    //add columns in correlation matrix
  }
  //add columns first
  for (unsigned int o = 0; o < finalParameters.GetNDouble(); o++) {
    if (!finalParameters.GetDoubleParameter(o)->IsFixed())
      tableCov->addColumn(finalParameters.GetDoubleParameter(o)->GetName(), 15);    //add columns in covariance matrix
  }

  unsigned int n = 0;
  tableCov->header();
  for (unsigned int o = 0; o < finalParameters.GetNDouble(); o++) {
    std::shared_ptr<DoubleParameter> ppp = initialParameters.GetDoubleParameter(
        o);
    std::shared_ptr<DoubleParameter> ppp2 = finalParameters.GetDoubleParameter(
        o);
    if (ppp->IsFixed())
      continue;
    *tableCov << ppp->GetName();
    for (unsigned int t = 0; t < cov.size1(); t++) {
      if (n >= cov.size2()) {
        *tableCov << " ";
        continue;
      }
      if (t >= n)
        *tableCov << cov(n, t);
      else
        *tableCov << "";
    }
    n++;
  }
  tableCov->footer();
  return;
}

void MinuitResult::writeXML(std::string filename) {
  std::ofstream ofs(filename);
  {    // closing xml tag is missing if boost::archive cannot go out of scope before file closes
    boost::archive::xml_oarchive oa(ofs);
    oa << boost::serialization::make_nvp("EstimatorValue", finalLH);

    oa << boost::serialization::make_nvp("MinimumValid", isValid);
    oa << boost::serialization::make_nvp("ValidCovariance", hasValidCov);
    oa << boost::serialization::make_nvp("ValidParameters", hasValidParameters);
    oa << boost::serialization::make_nvp("PosDefCovar", covPosDef);
    oa << boost::serialization::make_nvp("AccurateCovar", hasAccCov);
    oa
        << boost::serialization::make_nvp("ReachedCallLimit",
            hasReachedCallLimit);
    oa << boost::serialization::make_nvp("HesseFailed", hesseFailed);

    oa << boost::serialization::make_nvp("FitParameters", finalParameters);
    oa << boost::serialization::make_nvp("FitFractions", fractionList);

    if (hasValidCov) {
      FitCovarianceMatrix cov_mat;

      unsigned int n = 0;
      for (unsigned int o = 0; o < finalParameters.GetNDouble(); o++) {
        std::shared_ptr<DoubleParameter> par1 =
            initialParameters.GetDoubleParameter(o);
        if (par1->IsFixed())
          continue;
        unsigned int t(n);
        for (unsigned int o2 = o; o2 < finalParameters.GetNDouble(); o2++) {
          std::shared_ptr<DoubleParameter> par2 =
              initialParameters.GetDoubleParameter(o2);
          if (par2->IsFixed())
            continue;
          cov_mat.insertCovarianceValue(
              std::make_pair(par1->GetName(), par2->GetName()), corr(n, t));

          ++t;
        }
        ++n;
      }
      oa << boost::serialization::make_nvp("CorrelationMatrix", cov_mat);
    }
  }
  ofs.close();
  return;
}

void MinuitResult::writeTeX(std::string filename) {
  std::ofstream out(filename);
  bool printTrue = 0;
  if (trueParameters.GetNParameter())
    printTrue = 1;
  TableFormater* tableResult = new TexTableFormater(&out);
  printFitParameters(tableResult);
  if (hasValidCov) {
    unsigned int n = 0;
    TableFormater* tableCov = new TexTableFormater(&out);
    printCorrelationMatrix(tableCov);
    TableFormater* tableCorr = new TexTableFormater(&out);
    printCorrelationMatrix(tableCorr);
  }
  TableFormater* fracTable = new TexTableFormater(&out);
  printFitFractions(fracTable);    //calculate and print fractions if amplitude is set
  out.close();
  return;
}
bool MinuitResult::hasFailed() {
  bool failed = 0;
  if (!isValid)
    failed = 1;
//	if(!covPosDef) failed=1;
//	if(!hasValidParameters) failed=1;
//	if(!hasValidCov) failed=1;
//	if(!hasAccCov) failed=1;
//	if(hasReachedCallLimit) failed=1;
//	if(hesseFailed) failed=1;

  return failed;
}

} /* namespace Minuit2 */
} /* namespace Optimizer */
} /* namespace ComPWA */
