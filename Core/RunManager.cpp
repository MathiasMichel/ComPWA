//-------------------------------------------------------------------------------
// Copyright (c) 2013 Mathias Michel.
//
// This file is part of ComPWA, check license.txt for details
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the GNU Public License v3.0
// which accompanies this distribution, and is available at
// http://www.gnu.org/licenses/gpl.html
//
// Contributors:
//     Mathias Michel - initial API and implementation
//		Peter Weidenkaff - adding functionality to generate set of events
//-------------------------------------------------------------------------------
#include <memory>
#include <ctime>

#include <omp.h>
#include <time.h>

#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/generator_iterator.hpp>
#include <boost/log/trivial.hpp>
#include <boost/progress.hpp>

#include "DataReader/Data.hpp"
#include "Estimator/Estimator.hpp"
#include "Optimizer/Optimizer.hpp"
#include "Core/ParameterList.hpp"
#include "Core/Event.hpp"
#include "Core/Generator.hpp"
#include "Core/ProgressBar.hpp"

#include "Core/RunManager.hpp"

namespace ComPWA {

using namespace boost::log;
using DataReader::Data;
using Optimizer::Optimizer;

RunManager::RunManager() {
}
RunManager::RunManager(std::shared_ptr<Data> inD,
    std::shared_ptr<Amplitude> inP, std::shared_ptr<Optimizer::Optimizer> inO) :
    sampleData_(inD), amp_(inP), opti_(inO) {
}
RunManager::RunManager(std::shared_ptr<Amplitude> inP,
    std::shared_ptr<Generator> gen) :
    gen_(gen), amp_(inP) {
}

RunManager::~RunManager() {
  BOOST_LOG_TRIVIAL(info)<< "~RunManager: Last seed: "<<gen_->getSeed();
}

std::shared_ptr<FitResult> RunManager::startFit(ParameterList& inPar) {
  BOOST_LOG_TRIVIAL(info)<< "RunManager: Starting fit.";
  BOOST_LOG_TRIVIAL(info) << "RunManager: Input data contains "<<sampleData_->getNEvents()<<" events.";
  std::shared_ptr<FitResult> result = opti_->exec(inPar);
  BOOST_LOG_TRIVIAL(info) << "RunManager: fit end. Result ="<<result->getResult()<<".";

  return result;
}
bool RunManager::generate(int number) {
  if (number < 0)
    throw std::runtime_error(
        "RunManager: generate() negative number of events: "
            + std::to_string((long double) number));
  if (!(gen_ && amp_))
    throw std::runtime_error(
        "RunManager: generate() requirements not fulfilled");
  if (!sampleData_)
    throw std::runtime_error("RunManager: generate() not sample set");
  if (sampleData_->getNEvents() > 0)
    throw std::runtime_error(
        "RunManager: generate() dataset not empty! abort!");

  //Determing an estimate on the maximum of the physics amplitude using 100k events.
  double genMaxVal = 1.15 * amp_->getMaxVal(gen_);

  BOOST_LOG_TRIVIAL(info)<< "Generating MC: ["<<number<<" events] ";
  BOOST_LOG_TRIVIAL(info)<< "Using "<<genMaxVal<< " as maximum value for random number generation!";

  unsigned int startTime = clock();

  Generator* genNew = (&(*gen_))->Clone();    //copy generator for every thread
  double AMPpdf;

  unsigned int limit;
  unsigned int totalCalls = 0;
  unsigned int acceptedEvents = 0;
  if (samplePhsp_)
    limit = samplePhsp_->getNEvents();
  else
    limit = 100000000;    //set large limit, should never be reached
  Event tmp;
  //progressBar bar(number);
  for (unsigned int i = 0; i < limit; i++) {
    if (samplePhsp_)    //if phsp sample is set -> use it
      tmp = samplePhsp_->getEvent(i);
    else
      //otherwise generate event
      genNew->generate(tmp);
    totalCalls++;

   /* if (ComPWA::Particle::invariantMass(tmp.getParticle(0), tmp.getParticle(1))
        > 4 * tmp.getParticle(0).getMassSquare() + 0.2) {
      continue;
    }*/

    double weight = tmp.getWeight();
    /* reset weights: the weights are taken into account by hit and miss. The resulting
     * sample is therefore unweighted */
    tmp.setWeight(1.);    //reset weight
    tmp.setEfficiency(1.);    //reset weight
    dataPoint point(tmp);
     double ampRnd = genNew->getUniform() * genMaxVal;
    ParameterList list;
    list = amp_->intensity(point);    //unfortunatly not thread safe
    AMPpdf = *list.GetDoubleParameter(0);
     if (genMaxVal < (AMPpdf * weight)) {
     std::stringstream ss;
     ss << "RunManager::generate: error in HitMiss procedure. "
     << "Maximum value of random number generation smaller then amplitude maximum! "
     << genMaxVal << " < " << (AMPpdf * weight);
     throw std::runtime_error(ss.str());
     }
     if (ampRnd > (weight * AMPpdf))
     continue;
    sampleData_->pushEvent(tmp);    //Unfortunately not thread safe
    acceptedEvents++;
   // bar.nextEvent();
    if (acceptedEvents >= number)
      i = limit;    //continue if we have a sufficienct number of events
  }
  if (sampleData_->getNEvents() < number)
    BOOST_LOG_TRIVIAL(error)<< "RunManager::generate() not able to generate "<<number<<" events. "
    "Phsp sample too small. Current size of sample is now "<<sampleData_->getNEvents();
  BOOST_LOG_TRIVIAL(info)<< "Efficiency of toy MC generation: "<<(double)sampleData_->getNEvents()/totalCalls;
//	BOOST_LOG_TRIVIAL(info) << "RunManager: generate time="<<(clock()-startTime)/CLOCKS_PER_SEC/60<<"min.";

  return true;
}
;
bool RunManager::generateBkg(int number) {
  if (number == 0)
    return 0;
  if (!(ampBkg_ && gen_))
    throw std::runtime_error(
        "RunManager: generateBkg() requirements not fulfilled");
  if (!sampleBkg_)
    throw std::runtime_error(
        "RunManager: generateBkg() no background sample set");
  if (sampleBkg_->getNEvents() > 0)
    throw std::runtime_error(
        "RunManager: generateBkg() dataset not empty! abort!");
  //Determing an estimate on the maximum of the physics amplitude using 100k events.
  double genMaxVal = 1.2 * ampBkg_->getMaxVal(gen_);

  unsigned int totalCalls = 0;
  BOOST_LOG_TRIVIAL(info)<< "Generating background MC: ["<<number<<" events] ";

  unsigned int startTime = clock();

  Generator* genNew = (&(*gen_))->Clone();    //copy generator for every thread
  double AMPpdf;

  unsigned int limit;
  unsigned int acceptedEvents = 0;
  if (samplePhsp_) {
    limit = samplePhsp_->getNEvents();
  }
  else
    limit = 100000000;    //set large limit, should never be reached
  Event tmp;
  progressBar bar(number);
  for (unsigned int i = 0; i < limit; i++) {
    if (samplePhsp_)    //if phsp sample is set -> use it
      tmp = samplePhsp_->getEvent(i);
    else
      //otherwise generate event
      gen_->generate(tmp);
    totalCalls++;
    double weight = tmp.getWeight();
    /* reset weights: the weights are taken into account by hit and miss. The resulting
     * sample is therefore unweighted */
    tmp.setWeight(1.);    //reset weight
    tmp.setEfficiency(1.);    //reset weight
    dataPoint point(tmp);
    double ampRnd = genNew->getUniform() * genMaxVal;
    ParameterList list;
    list = ampBkg_->intensity(point);    //unfortunatly not thread safe
    AMPpdf = *list.GetDoubleParameter(0);
    if (genMaxVal < (AMPpdf * weight)) {
      std::stringstream ss;
      ss << "RunManager::generateBkg: error in HitMiss procedure. "
          << "Maximum value of random number generation smaller then amplitude maximum! "
          << genMaxVal << " < " << (AMPpdf * weight);
      throw std::runtime_error(ss.str());
    }
    if (ampRnd > (weight * AMPpdf))
      continue;
    sampleBkg_->pushEvent(tmp);    //unfortunatly not thread safe
    acceptedEvents++;
    bar.nextEvent();
    if (acceptedEvents >= number)
      i = limit;    //continue if we have a sufficienct number of events
  }
  if (sampleData_->getNEvents() < number)
    BOOST_LOG_TRIVIAL(error)<< "RunManager::generateBkg() not able to generate "<<number<<" events. "
    "Phsp sample too small. Current size of sample is now "<<sampleBkg_->getNEvents();
  BOOST_LOG_TRIVIAL(info)<< "Efficiency of toy MC generation: "<<(double)sampleData_->getNEvents()/totalCalls;

  return true;
}
;

bool RunManager::generatePhsp(int number) {
  if (number == 0)
    return 0;
  if (!samplePhsp_)
    throw std::runtime_error("RunManager: generatePhsp() not phsp sample set");
  if (samplePhsp_->getNEvents() > 0)
    throw std::runtime_error(
        "RunManager: generatePhsp() dataset not empty! abort!");

  BOOST_LOG_TRIVIAL(info)<< "Generating phase-space MC: ["<<number<<" events] ";

  Generator* genNew = (&(*gen_))->Clone();    //copy generator for every thread

  //progressBar bar(number);
  for (unsigned int i = 0; i < number; i++) {
    if (i > 0)
      i--;
    Event tmp;
    genNew->generate(tmp);
    double ampRnd = genNew->getUniform();
    if (ampRnd > tmp.getWeight())
      continue;
    /* reset weights: the weights are taken into account by hit and miss on the weights.
     * The resulting sample is therefore unweighted */
    tmp.setWeight(1.);    //reset weight
    tmp.setEfficiency(1.);
    i++;
    samplePhsp_->pushEvent(tmp);    //unfortunatly not thread safe
    //bar.nextEvent();
  }
  return true;
}
;

} /* namespace ComPWA */
