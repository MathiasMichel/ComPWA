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
//		Peter Weidenkaff - adding functionality to generate set of
//events
//-------------------------------------------------------------------------------
//! Run-Manager for a simple fit.
/*! \class PythonFit
 * @file PythonFit.hpp
 * This class provides a Manager for simple fits. It creates a set of modules
 * for an unbinned likelihood fit of a three-body final state.
 */

#ifndef _PYTHONFIT_HPP_
#define _PYTHONFIT_HPP_

#include <vector>
#include <memory>
#include <string>

#include <boost/program_options.hpp>
#include <boost/serialization/export.hpp>

#include "TVector3.h"
#include "TFile.h"
#include "TTree.h"
#include "TPython.h"

// Core header files go here
#include "Core/Event.hpp"
#include "Core/Particle.hpp"
#include "Core/Parameter.hpp"
#include "Core/ParameterList.hpp"
#include "Core/FunctionTree.hpp"
#include "Core/TableFormater.hpp"
#include "Core/AbsParameter.hpp"
#include "Core/Logging.hpp"

// ComPWA header files go here
#include "DataReader/RootReader/RootReader.hpp"
#include "DataReader/RootReader/RootEfficiency.hpp"
#include "DataReader/CorrectionTable.hpp"
#include "DataReader/DataCorrection.hpp"
#include "Estimator/MinLogLH/MinLogLH.hpp"
#include "Optimizer/Minuit2/MinuitIF.hpp"
#include "Optimizer/Minuit2/MinuitResult.hpp"
#include "Optimizer/Geneva/GenevaIF.hpp"
#include "Optimizer/Geneva/GenevaResult.hpp"
#include "Physics/HelicityFormalism.hpp"

#include "Tools/RunManager.hpp"
#include "Tools/RootGenerator.hpp"
#include "Tools/FitFractions.hpp"

template<typename T>
PyObject* convertToPy(const T& cxxObj) {
	T* newCxxObj = new T(cxxObj);
	return TPython::ObjectProxy_FromVoidPtr(newCxxObj, newCxxObj->ClassName(), true);
}

//template<typename T>
//PyObject* convertToPy(const T* cxxObj) {
//	return TPython::ObjectProxy_FromVoidPtr(&cxxObj, cxxObj->ClassName());
//};

template<typename T>
T convertFromPy(PyObject* pyObj) {
	TObject* TObj = (TObject*)(TPython::ObjectProxy_AsVoidPtr(pyObj));
	T cxxObj = dynamic_cast<T>(TObj);
	return cxxObj;
}

PyObject* convertTreeToPy(TTree* cxxTree) {
	TTree* newCxxTree = cxxTree->CloneTree();
	return TPython::ObjectProxy_FromVoidPtr(newCxxTree, newCxxTree->ClassName(), true);
};

class PythonFit {
public:
  PythonFit();

  virtual ~PythonFit();

  virtual int StartFit();

  virtual void setConfigFile(std::string fileName);

  virtual PyObject* testTree();

  virtual PyObject* testTVector3(PyObject* pyObj);

  inline virtual void useGeneva(){geneva=true;};

  /**Generate phase space events by Hit&Miss
   *
   * @param number Number of events to generate
   * @return
   */
  //virtual bool generatePhsp(int number);

  /**Generate signal events by Hit&Miss
   * 1) In case no phsp sample is set and the @param number is larger zero,
   * phsp events are generated on the fly.
   * 2) In case a phsp sample is set and @param number is smaller zero,
   * the whole sample is used for event generation.
   *
   * @param number Number of events to generate
   * @return
   */
  //virtual bool generate(int number);

  /**Generate background events by Hit&Miss
   * 1) In case no phsp sample is set and the @param number is larger zero, phsp
   * events
   * are generated on the fly.
   * 2) In case a phsp sample is set and @param number is smaller zero, the
   * whole sample
   * is used for event generation.
   *
   * @param number Number of events to generate
   * @return
   */
  //virtual bool generateBkg(int number);

protected:
  int argc;
  char ** argv;
  bool geneva;
  std::string optConfigFile;


};

#endif