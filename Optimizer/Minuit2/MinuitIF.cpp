//-------------------------------------------------------------------------------
// Copyright (c) 2013 Mathias Michel.
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the GNU Public License v3.0
// which accompanies this distribution, and is available at
// http://www.gnu.org/licenses/gpl.html
//
// Contributors:
//     Mathias Michel - initial API and implementation
//-------------------------------------------------------------------------------
#include <vector>
#include <time.h>
#include <string>
#include <sstream>
#include <iostream>
#include <memory>
#include "Minuit2/MnUserParameters.h"
#include "Minuit2/MnMigrad.h"
#include "Minuit2/FunctionMinimum.h"
#include "Minuit2/MnMinos.h"
#include "Minuit2/MnHesse.h"
#include "Minuit2/MnStrategy.h"
#include "Minuit2/MnPrint.h"
#include "Minuit2/MinosError.h"
#include "Optimizer/Minuit2/MinuitIF.hpp"
#include "Core/ParameterList.hpp"
#include "Core/Parameter.hpp"
#include "Core/FitResult.hpp"

#include <boost/numeric/ublas/symmetric.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/timer.hpp>

namespace ComPWA {
namespace Optimizer {
namespace Minuit2 {

using namespace boost::log;
using namespace ROOT::Minuit2;

double shiftAngle(double v){
	double originalVal = v;
	double val = originalVal;
	double pi = ComPWA::PhysConst::Instance().findConstant("Pi").value_;
	while(val> pi) val-=2*pi;
	while(val< -pi ) val+=2*pi;
	if(val!=originalVal)
		BOOST_LOG_TRIVIAL(info) << "shiftAngle(): shifting parameter from "<<originalVal<< " to "<<val<<"!";
	return val;
}


MinuitIF::MinuitIF(std::shared_ptr<ControlParameter> esti, ParameterList& par) :
		_myFcn(esti, par), estimator(esti)
{

}

MinuitIF::~MinuitIF()
{

}

std::shared_ptr<FitResult> MinuitIF::exec(ParameterList& par){
	boost::timer time;
	ParameterList initialParList(par);

	MnUserParameters upar;
	BOOST_LOG_TRIVIAL(debug) << "MinuitIF::exec() | Number of parameters: "<<par.GetNDouble();
	for(unsigned int i=0; i<par.GetNDouble(); ++i){ //only doubles for minuit
		std::shared_ptr<DoubleParameter> actPat = par.GetDoubleParameter(i);
		//if no error is set or error set to 0 we use a default error,
		//otherwise minuit treads this parameter as fixed
		double error(0.01);
		if(actPat->HasError())
		  error = actPat->GetError();
		if(!actPat->IsFixed() && actPat->GetName().find("phase") != actPat->GetName().npos)
			actPat->SetValue( shiftAngle(actPat->GetValue()) );

		if( actPat->UseBounds() ){
			upar.Add(actPat->GetName(), actPat->GetValue(), error,
					actPat->GetMinValue(), actPat->GetMaxValue());
		}else
			upar.Add(actPat->GetName(), actPat->GetValue(),error);

		_myFcn.setNameID(i, actPat->GetName());

		if(actPat->IsFixed())
			upar.Fix(actPat->GetName());
	}

	//use MnStrategy class to set all options for the fit
	//	MnStrategy strat; //using default strategy = 1
	MinuitStrategy strat;//using default strategy = 1

	//read in xml configuration file for strategy settings
	const char* pPath = getenv("COMPWA_DIR");
	std::string path = std::string(pPath);
	std::ifstream ifs(path+"/Optimizer/Minuit2/MinuitStrategy.xml");
	boost::archive::xml_iarchive ia(ifs,boost::archive::no_header);
	ia >> BOOST_SERIALIZATION_NVP(strat);
	strat.init();//update parameters of MnStrategy mother class (IMPORTANT!)
	ifs.close();
	//write strategy settings
	//	std::ofstream ofs(path+"Optimizer/Minuit2/test.xml");
	//	boost::archive::xml_oarchive oa(ofs,boost::archive::no_header);
	//	oa << BOOST_SERIALIZATION_NVP(strat);
	//	ofs.close();
	BOOST_LOG_TRIVIAL(debug) << "Minuit strategy parameters: (from "<<
			path+"Optimizer/Minuit2/MinuitStrategy.xml"<<")";
	BOOST_LOG_TRIVIAL(debug) << "Gradient number of steps: "<<strat.GradientNCycles();
	BOOST_LOG_TRIVIAL(debug) << "Gradient step tolerance: "<<strat.GradientStepTolerance();
	BOOST_LOG_TRIVIAL(debug) << "Gradient tolerance: "<<strat.GradientTolerance();
	BOOST_LOG_TRIVIAL(debug) << "Hesse number of steps: "<<strat.HessianNCycles();
	BOOST_LOG_TRIVIAL(debug) << "Hesse gradient number of steps: "<<strat.HessianGradientNCycles();
	BOOST_LOG_TRIVIAL(debug) << "Hesse step tolerance: "<<strat.HessianStepTolerance();
	BOOST_LOG_TRIVIAL(debug) << "Hesse G2 tolerance: "<<strat.HessianG2Tolerance();

	//MIGRAD
	BOOST_LOG_TRIVIAL(info) <<"MinuitIF::exec() | starting migrad ";
	MnMigrad migrad(_myFcn, upar, strat);
	//	FunctionMinimum minMin = migrad(100,0.001);//(maxfcn,tolerance)
	FunctionMinimum minMin = migrad(50*par.GetNDouble()*par.GetNDouble());
	BOOST_LOG_TRIVIAL(info) <<"MinuitIF::exec() | migrad finished! Minimum is valid = "
			<<minMin.IsValid();
	if(!minMin.IsValid()) {
	  BOOST_LOG_TRIVIAL(info) <<"MinuitIF::exec() | Has covariance = "
	            <<minMin.HasCovariance();
    BOOST_LOG_TRIVIAL(info) <<"MinuitIF::exec() | Covariance valid = "
              <<minMin.HasValidCovariance();
    BOOST_LOG_TRIVIAL(info) <<"MinuitIF::exec() | Covariance accurate = "
          <<minMin.HasAccurateCovar();
    BOOST_LOG_TRIVIAL(info) <<"MinuitIF::exec() | Covariance pos def = "
              <<minMin.HasPosDefCovar();
    BOOST_LOG_TRIVIAL(info) <<"MinuitIF::exec() | Covariance made pos def = "
              <<minMin.HasMadePosDefCovar();
	  BOOST_LOG_TRIVIAL(info) <<"MinuitIF::exec() | Reached call limit = "
	            <<minMin.HasReachedCallLimit();
    BOOST_LOG_TRIVIAL(info) <<"MinuitIF::exec() | Parameters valid = "
              <<minMin.HasValidParameters();
    BOOST_LOG_TRIVIAL(info) <<"MinuitIF::exec() | Hesse failed = "
              <<minMin.HesseFailed();

	}

	//HESSE
	MnHesse hesse(strat);
	if(minMin.IsValid()) {
		BOOST_LOG_TRIVIAL(info) <<"MinuitIF::exec() | starting hesse";
		hesse(_myFcn,minMin);//function minimum minMin is updated by hesse
		BOOST_LOG_TRIVIAL(info) <<"MinuitIF::exec() | hesse finished";
	} else
		BOOST_LOG_TRIVIAL(info) <<"MinuitIF::exec() | migrad failed to find minimum! "
				"We skip hesse and minos!";

	//MINOS
	MnMinos minos(_myFcn,minMin,strat);

	//we copy parameters here because minos can still change the parameterList par
	ParameterList finalParList(par);
	//save minimzed values
	MnUserParameterState minState = minMin.UserState();
	for(unsigned int i=0; i<finalParList.GetNDouble(); ++i){
		std::shared_ptr<DoubleParameter> finalPar = finalParList.GetDoubleParameter(i);
		if(!finalPar->IsFixed()){
			double val=minState.Value(finalPar->GetName());
			if(finalPar->GetName().find("phase") != finalPar->GetName().npos)
				val =  shiftAngle(val);
			finalPar->SetValue(val);
			if(finalPar->GetErrorType()==ErrorType::ASYM && minMin.IsValid()){
				//asymmetric errors -> run minos
				BOOST_LOG_TRIVIAL(info) <<"MinuitIF::exec() | minos for parameter "<<i<< "...";
				MinosError err = minos.Minos(i);
				//lower = pair.first, upper= pair.second
				std::pair<double,double> assymErrors = err();
				finalPar->SetError( assymErrors.first, assymErrors.second );
			} else if(finalPar->GetErrorType()==ErrorType::SYM) {
				//symmetric errors -> migrad/hesse error
				finalPar->SetError(minState.Error(finalPar->GetName()));
			} else
				throw std::runtime_error("MinuitIF::exec() | unknown error type: "
						+std::to_string((long long int)finalPar->GetErrorType()));
		}
	}

	std::shared_ptr<FitResult> result(new MinuitResult(estimator, minMin));
	result->setInitialParameters(initialParList);
	result->setFinalParameters(finalParList);
	result->setTime(time.elapsed());

	return result;
}

} /* namespace Minuit2 */
} /* namespace Optimizer */
} /* namespace ComPWA */
