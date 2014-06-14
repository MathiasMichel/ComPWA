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
//****************************************************************************
// Abstract base class for dynamical functions.
//****************************************************************************

// --CLASS DESCRIPTION [MODEL] --
// Abstract base class for dynamical functions.

#ifndef AMP_ABS_DYNAMICAL_FUNCTION
#define AMP_ABS_DYNAMICAL_FUNCTION

#include <vector>
#include <complex>

#include "Core/Parameter.hpp"
#include "Core/DataPoint.hpp"

class AmpAbsDynamicalFunction {
public:
  AmpAbsDynamicalFunction(const char *name);

  AmpAbsDynamicalFunction(const AmpAbsDynamicalFunction&, const char*);

  virtual ~AmpAbsDynamicalFunction();

  //! Implementation of interface for streaming info about the strategy
  virtual const std::string to_str() const {
	  return (_name+"rel Breit-Wigner with Wigner_D");
  }

  //! Implementation of interface for executing a strategy
 /* virtual std::shared_ptr<AbsParameter> execute(const ParameterList& paras){
	  std::complex<double> result;

    result = evaluateTree(paras);

    //ParameterList out;
    // out.AddParameter(DoubleParameter("AddAllResult",result));
    return std::shared_ptr<AbsParameter>(new ComplexParameter("AddAllResult",result));
  };*/


  virtual void initialise() = 0; 
  virtual std::complex<double> evaluate(dataPoint& point) = 0;
  //! value of dynamical amplitude at \param point
  virtual std::complex<double> evaluateAmp(dataPoint& point) = 0;
  //! value of WignerD amplitude at \param point
  virtual double evaluateWignerD(dataPoint& point) = 0;

  //! Get resonance spin
  virtual double getSpin() = 0;
  //! check subsystem of resonance
  virtual bool isSubSys(const unsigned int) const = 0;
  //! Calculation integral |dynamical amplitude|^2
  virtual double integral(unsigned int nCalls=100000) const;
  //! Calculation integral |dynamical amplitude * WignerD|^2
  virtual double totalIntegral() const;

  virtual std::string GetName(){ return _name; };
  virtual std::string GetTitle(){ return GetName(); };
  virtual double GetNormalization(){ return _norm; };
  virtual void SetNormalization(double n){ _norm=n; };
 
protected:
  std::string _name;
  double _norm;

private:

};

#endif
