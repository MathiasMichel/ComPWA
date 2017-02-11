//-------------------------------------------------------------------------------
// Copyright (c) 2013 Mathias Michel.
//
// This file is part of ComPWA
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the GNU Public License v3.0
// which accompanies this distribution, and is available at
// http://www.gnu.org/licenses/gpl.html
//
// Contributors:
//     Mathias Michel - initial API and implementation
//-------------------------------------------------------------------------------
#include <sstream>
#include <iostream>
#include <memory>
#include <vector>
#include <utility>
#include <stdexcept>

#include "DataReader/RootReader/RootReader.hpp"
#include "Core/Kinematics.hpp"
#include "Core/Generator.hpp"

#include "TParticle.h"

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
using namespace boost::log;

namespace ComPWA {
namespace DataReader {
namespace RootReader {

RootReader::RootReader() {
  fFile = 0;    //need to do this to avoid seg. violation when destructor is called
}

RootReader::RootReader(TTree* tr, int size, const bool binned) :
    readSize(size), fBinned(binned) {
  fTree = tr;
  fFile = 0;    //need to do this to avoid seg. violation when destructor is called
  read();
}

RootReader::RootReader(const std::string inRootFile,
    const std::string inTreeName, int size, const bool binned) :
    fBinned(binned), fileName(inRootFile), treeName(inTreeName), readSize(size) {
  fFile = new TFile(fileName.c_str());
  if (fFile->IsZombie())
    throw std::runtime_error(
        "RootReader::RootReader() can't open data file: " + inRootFile);
  fTree = (TTree*) fFile->Get(treeName.c_str());
  if (!fTree)
    throw std::runtime_error(
        "RootReader::RootReader() Tree \"" + treeName
            + "\" can not be opened can't from file " + inRootFile + "! ");
  read();
  fFile->Close();
}

RootReader::~RootReader() {

}

bool RootReader::hasWeights() {
  bool has = 0;
  for (unsigned int evt = 0; evt < fEvents.size(); evt++) {
    if (fEvents.at(evt).getWeight() != 1.) {
      has = 1;
      break;
    }
  }
  return has;
}

void RootReader::Clear() {
  fEvents.clear();
}
void RootReader::setEfficiency(std::shared_ptr<Efficiency> eff) {
  for (unsigned int evt = 0; evt < fEvents.size(); evt++) {
    dataPoint e(fEvents.at(evt));
    double val = eff->evaluate(e);
    fEvents.at(evt).setEfficiency(val);
  }
}
void RootReader::resetEfficiency(double e) {
  for (unsigned int evt = 0; evt < fEvents.size(); evt++) {
    fEvents.at(evt).setEfficiency(e);
  }
}
void RootReader::reduce(unsigned int newSize) {
  if (newSize >= fEvents.size()) {
    BOOST_LOG_TRIVIAL(error)<< "RooReader::reduce() requested size too large, cant reduce sample!";
    return;
  }
  fEvents.resize(newSize);
}

void RootReader::reduceToPhsp() {
  std::vector<Event> tmp;
  for (unsigned int evt = 0; evt < fEvents.size(); evt++) {
    dataPoint p(fEvents.at(evt));
    if (Kinematics::instance()->isWithinPhsp(p))
      tmp.push_back(fEvents.at(evt));
  }
  BOOST_LOG_TRIVIAL(info)<<"RootReader::reduceToPhsp() | remove all events outside PHSP boundary from data sample.";
  BOOST_LOG_TRIVIAL(info)<<"RootReader::reduceToPhsp() | "<<tmp.size()<<" from "<<fEvents.size()<<"("<<((double)tmp.size())/fEvents.size()*100 <<"%) were kept.";
  fEvents = tmp;
  return;
}
std::shared_ptr<Data> RootReader::rndSubSet(unsigned int size,
    std::shared_ptr<Generator> gen) {
  std::shared_ptr<Data> newSample(new RootReader());
  unsigned int totalSize = getNEvents();
  unsigned int newSize = totalSize;
  /* 1th method: new Sample has exact size, but possibly events are added twice.
   * We would have to store all used events in a vector and search the vector at every event -> slow */
  /*unsigned int t=0;
   unsigned int d=0;
   while(t<newSize){
   d = (unsigned int) gen->getUniform()*totalSize;
   newSample->pushEvent(fEvents[d]);
   t++;
   }*/

  /* 2nd method: events are added once only, but total size of sample varies with sqrt(N) */
  for (unsigned int i = 0; i < totalSize; i++) {    //count how many events are not within PHSP
    dataPoint point(fEvents[i]);
    if (!Kinematics::instance()->isWithinPhsp(point))
      newSize--;
  }
  double threshold = (double) size / newSize;    //calculate threshold
  for (unsigned int i = 0; i < totalSize; i++) {
    dataPoint point(fEvents[i]);
    if (!Kinematics::instance()->isWithinPhsp(point))
      continue;
    if (gen->getUniform() < threshold)
      newSample->pushEvent(fEvents[i]);
  }
  return newSample;
}

void RootReader::read() {
  fParticles = new TClonesArray("TParticle");
  fTree->GetBranch("Particles")->SetAutoDelete(false);
  fTree->SetBranchAddress("Particles", &fParticles);
  fTree->SetBranchAddress("eff", &feventEff);
  fTree->SetBranchAddress("weight", &feventWeight);
  fTree->SetBranchAddress("charge", &fCharge);
  fTree->SetBranchAddress("flavour", &fFlavour);
  //	fEvent=0;
  bin();
  storeEvents();

}

const std::vector<std::string>& RootReader::getVariableNames() {
  if (!fVarNames.size()) {    //TODO: init
    fVarNames.push_back("dataname1");
    fVarNames.push_back("dataname2");
  }
  return fVarNames;
}
void RootReader::resetWeights(double w) {
  for (unsigned int i = 0; i < fEvents.size(); i++)
    fEvents.at(i).setWeight(w);
  return;
}

Event& RootReader::getEvent(const int i) {
  return fEvents.at(i);
}

allMasses RootReader::getMasses(const unsigned int startEvent,
    unsigned int nEvents) {
  if (!fEvents.size())
    return allMasses();
  if (!nEvents)
    nEvents = fEvents.size();
  unsigned int nParts = fEvents.at(0).getNParticles();

  //determine invMass combinations
  unsigned int nMasses = 0;
  std::vector<std::pair<unsigned int, unsigned int> > ids;
  for (unsigned int i = 0; i < nParts; i++)
    for (unsigned int j = i + 1; j < nParts; j++) {
      nMasses++;
      ids.push_back(std::make_pair(i + 1, j + 1));
    }

  if (startEvent + nEvents > fEvents.size()) {
    nEvents = fEvents.size() - startEvent;
    //Todo: Exception
  }

  unsigned int nSkipped = 0;    //count events which are outside PHSP boundary
  unsigned int nFilled = 0;    //count events which are outside PHSP boundary

  allMasses result(nMasses, ids);
  //calc and store inv masses
  for (unsigned int evt = startEvent; evt < startEvent + nEvents; evt++) {
    if (result.Fill(fEvents.at(evt)))
      nFilled++;
    else
      nSkipped++;
  }
  if (nSkipped)
    BOOST_LOG_TRIVIAL(debug)<<"RootReader::getMasses() "<<nSkipped
    <<"("<<(double)nSkipped/fEvents.size()*100<<"%) data points are "
    "outside the PHSP boundary. We skip these points!";

    //	std::cout<<"after      "<<result.masses_sq.at(std::make_pair(2,3)).size()<<std::endl;
  return result;
}

const int RootReader::getBin(const int i, double& m12, double& weight) {
  if (!fBinned)
    return -1;

  m12 = fBins[i].first;
  weight = fBins[i].second;

  return 1;
}

void RootReader::storeEvents() {
  if (readSize <= 0 || readSize > fTree->GetEntries())
    readSize = fTree->GetEntries();
  for (unsigned int evt = 0; evt < readSize; evt++) {
    Event tmp;
    fParticles->Clear();
    fTree->GetEntry(evt);

    // Get number of particle in TClonesrray
    unsigned int nParts = fParticles->GetEntriesFast();

    TParticle* partN;
    TLorentzVector inN;
    for (unsigned int part = 0; part < nParts; part++) {
      partN = 0;
      partN = (TParticle*) fParticles->At(part);
      if (!partN)
        continue;
      partN->Momentum(inN);
      tmp.addParticle(
          Particle(inN.X(), inN.Y(), inN.Z(), inN.E(), partN->GetPdgCode()));
    }    //particle loop
    tmp.setWeight(feventWeight);
    tmp.setCharge(fCharge);
    tmp.setFlavour(fFlavour);
    tmp.setEfficiency(feventEff);

    fEvents.push_back(tmp);
  }    //event loop
}

void RootReader::changeWeight(unsigned int event_index, double weight) {
  fEvents[event_index].setWeight(fEvents[event_index].getWeight() * weight);
}

void RootReader::writeData(std::string file, std::string trName) {
  if (file != "")
    fileName = file;
  if (trName != "")
    treeName = trName;
  BOOST_LOG_TRIVIAL(info)<< "RootReader: writing current vector of events to file "<<fileName;
  TFile* fFile = new TFile(fileName.c_str(), "UPDATE");
  if (fFile->IsZombie())
    throw std::runtime_error(
        "RootReader::RootReader() can't open data file: " + fileName);

  fTree = new TTree(treeName.c_str(), treeName.c_str());
  unsigned int numPart = fEvents[0].getNParticles();
  fParticles = new TClonesArray("TParticle", numPart);
  fTree->Branch("Particles", &fParticles);
  fTree->Branch("weight", &feventWeight, "weight/D");
  fTree->Branch("eff", &feventEff, "weight/D");
  fTree->Branch("charge", &fCharge, "charge/I");
  fTree->Branch("flavour", &fFlavour, "flavour/I");
  TClonesArray &partArray = *fParticles;

  TParticle* particle;

  TLorentzVector motherMomentum(0, 0, 0,
      Kinematics::instance()->getMotherMass());

  if (fEvents.begin() != fEvents.end()) {
    for (unsigned int i = 0; i < numPart; i++) {
      const Particle oldParticle = (*fEvents.begin()).getParticle(i);
      particle = (TParticle*) fParticles->ConstructedAt(i);
      particle->SetPdgCode(oldParticle.pid);
      particle->SetStatusCode(1);
      particle->SetProductionVertex(motherMomentum);
      particle->SetWeight(1.0);
    }
  }

  for (std::vector<Event>::iterator it = fEvents.begin(); it != fEvents.end();
      it++) {
    fParticles->Clear();
    feventWeight = (*it).getWeight();
    fCharge = (*it).getCharge();
    fFlavour = (*it).getFlavour();
    feventEff = (*it).getEfficiency();
    for (unsigned int i = 0; i < numPart; i++) {
      const Particle oldParticle = (*it).getParticle(i);
      particle = (TParticle*) fParticles->ConstructedAt(i);
      particle->SetMomentum(oldParticle.px, oldParticle.py, oldParticle.pz,
          oldParticle.E);
    }
    fTree->Fill();
  }
  fTree->Write("", TObject::kOverwrite, 0);
  fFile->Close();
  return;
}

void RootReader::bin() {
  double min = -1, max = -1;
  fmaxBins = 500;    //TODO setter, consturctor
  TLorentzVector in1, in2;

  //initialize min & max
  fTree->GetEntry(0);
  unsigned int nParts = fParticles->GetEntriesFast();
  if (nParts != 2)
    return;
  TParticle* part1 = (TParticle*) fParticles->At(0);    //pip
  TParticle* part2 = (TParticle*) fParticles->At(1);    //pim
  if (!part1 || !part2)
    return;
  part1->Momentum(in1);
  part2->Momentum(in2);
  double inm12 = (in1 + in2).Mag2();
  min = max = inm12;

  //find min and max
  for (unsigned int evt = 1; evt < fTree->GetEntries(); evt++) {
    fTree->GetEntry(evt);
    unsigned int nParts = fParticles->GetEntriesFast();
    if (nParts != 2)
      return;

    part1 = (TParticle*) fParticles->At(0);    //pip
    part2 = (TParticle*) fParticles->At(1);    //pim
    if (!part1 || !part2)
      return;
    part1->Momentum(in1);
    part2->Momentum(in2);

    inm12 = (in1 + in2).Mag2();

    if (min > inm12)
      min = inm12;
    if (max < inm12)
      max = inm12;
  }

  //initialize bins with weight zero
  min = sqrt(min);
  max = sqrt(max);
  double step = (max - min) / (double) fmaxBins;
  for (unsigned int bin = 0; bin < fmaxBins; bin++) {
    fBins[bin] = std::make_pair(min + bin * step, 0.0);
  }

  //fill bins
  for (unsigned int evt = 0; evt < fmaxBins; evt++) {
    fTree->GetEntry(evt);
    unsigned int nParts = fParticles->GetEntriesFast();
    if (nParts != 2)
      return;

    TParticle* part1 = (TParticle*) fParticles->At(0);    //pip
    TParticle* part2 = (TParticle*) fParticles->At(1);    //pim
    if (!part1 || !part2)
      return;
    part1->Momentum(in1);
    part2->Momentum(in2);
    inm12 = sqrt((in1 + in2).Mag2());

    int bin = (int) ((inm12 - min) / step);
    fBins[bin].second += 1.;
  }

}

std::vector<dataPoint> RootReader::getDataPoints() {
  std::vector<dataPoint> vecPoint;
  for (int i = 0; i < fEvents.size(); i++)
    vecPoint.push_back(dataPoint(fEvents.at(i)));
  return vecPoint;
}

} /* namespace RootReader */
} /* namespace DataReader */
} /* namespace ComPWA */

