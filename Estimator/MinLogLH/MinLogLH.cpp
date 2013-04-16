#include <sstream>
#include <iostream>
#include <memory>
#include <vector>
#include <cmath>

#include "Estimator/MinLogLH/MinLogLH.hpp"
#include "Core/Event.hpp"
#include "Core/Particle.hpp"
#include "Core/ParameterList.hpp"

MinLogLH::MinLogLH(std::shared_ptr<Amplitude> inPIF, std::shared_ptr<Data> inDIF)
  : pPIF_(inPIF), pDIF_(inDIF){

}

MinLogLH::MinLogLH(std::shared_ptr<Amplitude> inPIF, std::shared_ptr<Data> inDIF, std::shared_ptr<Data> inPHSP)
  : pPIF_(inPIF), pDIF_(inDIF), pPHSP_(inPHSP){

}

MinLogLH::~MinLogLH(){

}

double MinLogLH::controlParameter(ParameterList& minPar){
  unsigned int nEvents = pDIF_->getNEvents();
  unsigned int nPHSPEvts = pPHSP_->getNEvents();
  unsigned int nParts = ((Event)pDIF_->getEvent(0)).getNParticles();

  //check if able to handle this many particles
  if(nParts<2 || nParts>3){
      //TODO: exception
      return 0;
  }


  double norm = 0;
  if(nParts==2){
    //norm by numerical integral
    norm = pPIF_->integral(minPar);
  }else if(nParts==3){
    //norm by phasespace monte-carlo
    if(pPHSP_){
      for(unsigned int phsp=0; phsp<nPHSPEvts; phsp++){
        Event theEvent(pPHSP_->getEvent(phsp));
        //TODO: not just for 3 particles?
        if(theEvent.getNParticles()!=3) continue;
        const Particle &a(theEvent.getParticle(0));
        const Particle &b(theEvent.getParticle(1));
        const Particle &c(theEvent.getParticle(2));

        std::vector<double> x;
        double masssqa = 0, masssqb = 0, masssqc = 0;
        masssqa += (pow(a.E+b.E,2) - pow(a.px+b.px ,2) - pow(a.py+b.py ,2) - pow(a.pz+b.pz ,2));
        masssqb += (pow(a.E+c.E,2) - pow(a.px+c.px ,2) - pow(a.py+c.py ,2) - pow(a.pz+c.pz ,2));
        masssqc += (pow(c.E+b.E,2) - pow(c.px+b.px ,2) - pow(c.py+b.py ,2) - pow(c.pz+b.pz ,2));
        x.push_back(sqrt(masssqa));
        x.push_back(sqrt(masssqb));
        x.push_back(sqrt(masssqc));

        double intens = pPIF_->intensity(x, minPar);
        if(intens>0){
          norm+=intens;
        }
      }
      norm/=nPHSPEvts;
     // norm=log(norm);
    }//else{
      //TODO: Exceptions no PHSPMC, try numerical integration?
    //}
  }

  /*std::cout << std::endl << "ControlPar LH: " << std::endl;
  std::cout << "Events: " << nEvents << std::endl;
  for(unsigned int i=0; i<minPar.GetNDouble(); i++){
    std::cout << minPar.GetParameterValue(i) << " ";
  }
  std::cout << std::endl;*/

  double lh=0; //calculate LH:
  switch(nParts){ //TODO: other cases, better "x" description (selection of particles?)
  case 2:{
    for(unsigned int evt = 0; evt < nEvents; evt++){
      Event theEvent(pDIF_->getEvent(evt));

     // TODO: try read exceptions

      std::vector<double> x;
      if( nParts != theEvent.getNParticles()) continue; //TODO: real event count?

      const Particle &a(theEvent.getParticle(0));
      const Particle &b(theEvent.getParticle(1));

      double masssq = 0;
      masssq += (pow(a.E+b.E,2) - pow(a.px+b.px ,2) - pow(a.py+b.py ,2) - pow(a.pz+b.pz ,2));
      x.push_back(sqrt(masssq));

      double intens = pPIF_->intensity(x, minPar);
      if(intens>0){
        lh -= (log(intens/norm));
      }
    }

    break;
  }
  case 3:{
    for(unsigned int evt = 0; evt < nEvents; evt++){
      Event theEvent(pDIF_->getEvent(evt));
      const Particle &a(theEvent.getParticle(0));
      const Particle &b(theEvent.getParticle(1));
      const Particle &c(theEvent.getParticle(2));

      if( nParts != theEvent.getNParticles()) continue; //TODO: real event count?

      std::vector<double> x;
      double masssqa = 0, masssqb = 0, masssqc = 0;
      masssqa += (pow(a.E+b.E,2) - pow(a.px+b.px ,2) - pow(a.py+b.py ,2) - pow(a.pz+b.pz ,2));
      masssqb += (pow(a.E+c.E,2) - pow(a.px+c.px ,2) - pow(a.py+c.py ,2) - pow(a.pz+c.pz ,2));
      masssqc += (pow(c.E+b.E,2) - pow(c.px+b.px ,2) - pow(c.py+b.py ,2) - pow(c.pz+b.pz ,2));
      x.push_back(sqrt(masssqa));
      x.push_back(sqrt(masssqb));
      x.push_back(sqrt(masssqc));

      double intens = pPIF_->intensity(x, minPar);
      if(intens>0){
        lh -= (log(intens));
      }

    }
    lh/=nEvents;
    lh += norm;
    break;
  }
  default:{
    //TODO: exception "i dont know how to handle this data"
    break;
  }
  }//end switch-case

  //std::cout << "ControlPar list " << minPar.GetNDouble() <<std::endl;

  return lh;
}
