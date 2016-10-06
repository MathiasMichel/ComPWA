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
//! Run-Manager for a simple fit.
/*! \class RunManager
 * @file RunManager.hpp
 * This class provides a RunManager for simple fits. To use it, you create
 * all modules you want to use and provide them to the RunManger. It checks
 * for compatibility and if set up correctly it starts the fitting procedure.
 */

/*! \mainpage ComPWA Documentation
 *
 * \section about About ComPWA
 *
 * ComPWA aims to provide a framework and toolkit for amplitude analysis in
 * a very general approach with clearly defined interfaces to extend its
 * features. Therefore, it can easily be extended for different experiments
 * and various models while maintaining a common structure, producing
 * comparable output, using the same fit-procedures and quality assurance
 * tools.
 *
 * \subsection structure Overview
 *
 * The basic modularization of the software can be seen in the following
 * picture. The four basic modules (Experiment, Physics, Estimation and
 * Optimization) each provide interfaces which need to be implemented
 * depending on your problem. They are represented in the framework by the
 * folder structure as well as in the build libraries. When attacking a new
 * problem using ComPWA, you might find that you can use existing module
 * implementations and only need to rewrite or extend part of it.
 *
 * \image html ComPWA_Modules.jpg
 * \image latex ComPWA_Modules.eps "ComPWA Modules" width=10cm
 *
 * \section install Installation
 *
 * \subsection step1 Build Tool: Boost.Build
 *
 * The ComPWA libraries and example-executables are build using the build tool
 * provided by the boost library. First, one needs to provide some paths to
 * external tools via environmental libraries, as can be seen in the setEnv
 * blank file. Secondly, you call the configure.pl which looks for available
 * modules and asks which you want to build. Now you can call bjam to compile
 * the code. All libraries will be located in the lib folder, all binaries in
 * the bin folder. Boost.Build uses Jamfiles for the build configuration. The
 * main file used when bjam is called is Jamroot in the root directory of
 * ComPWA. In the subdirectories of the interfaces and the actual
 * implementations you can find Jamfiles to manage the build process.
 *
 * \subsection step2 Used External Libraries
 *
 * The core Framework only needs a boost installation and a compiler
 * supporting c++11. But for the examples and most likely also for starting
 * some fits and plotting some output, it is recommended to have a root
 * installation (for plots and maybe DataReader::Data storage/reading) and a Minuit2
 * (for optimization) installation ready. Besides Minuit2 also Geneva can be
 * used as optimizer if a compatible installation is available.
 *
 * \section starting Getting Started
 * A good point to start learning how ComPWA works is by looking in the
 * IntegrationTestApp.cpp executable, where a simple fit is performed using
 * all modules of the framework. ...
 */

#ifndef _RUNMANAGER_HPP_
#define _RUNMANAGER_HPP_

#include <vector>
#include <memory>

#include "DataReader/Data.hpp"
#include "Estimator/Estimator.hpp"
#include "Core/Amplitude.hpp"
#include "Optimizer/Optimizer.hpp"
#include "Core/FitResult.hpp"
#include "Core/ParameterList.hpp"
#include "Core/Efficiency.hpp"
#include "Core/Generator.hpp"

namespace ComPWA {

class RunManager
{
public:

	RunManager();
	RunManager( std::shared_ptr<DataReader::Data>, std::shared_ptr<Amplitude>, std::shared_ptr<Optimizer::Optimizer>); //Fit
	RunManager(std::shared_ptr<Amplitude>, std::shared_ptr<Generator>); //Generate

	virtual ~RunManager();

	virtual void setData ( std::shared_ptr<DataReader::Data> d){ sampleData_ = d; };
	virtual std::shared_ptr<DataReader::Data> geData (){ return sampleData_; };
	virtual void setBackground ( std::shared_ptr<DataReader::Data> d){ sampleBkg_ = d; };
	virtual std::shared_ptr<DataReader::Data> getBackground (){ return sampleBkg_; };
	virtual void setPhspSample( std::shared_ptr<DataReader::Data> d){ samplePhsp_ = d; };
	virtual std::shared_ptr<DataReader::Data> getPhspSample(){ return samplePhsp_; };
	virtual void setAmplitude ( std::shared_ptr<Amplitude> d){ amp_ = d; };
	virtual std::shared_ptr<Amplitude> getAmplitude (){ return amp_; };
	virtual void setBkgAmplitude ( std::shared_ptr<Amplitude> d){ ampBkg_ = d; };
	virtual std::shared_ptr<Amplitude> getBkgAmplitude(){ return ampBkg_; };
	virtual void setOptimizer ( std::shared_ptr<Optimizer::Optimizer> d){ opti_ = d; };
	virtual std::shared_ptr<Optimizer::Optimizer> getOptimizer (){ return opti_; };
	virtual void setGenerator( std::shared_ptr<Generator> d){ gen_= d; };
	virtual std::shared_ptr<Generator> getGenerator(){ return gen_; };

	virtual std::shared_ptr<FitResult> startFit( ParameterList& );
	virtual bool generate ( int number );
	virtual bool generatePhsp ( int number );
	virtual bool generateBkg ( int number );

protected:
	std::shared_ptr<DataReader::Data> sampleData_; /*!< Pointer to DataReader::Data sample */
	std::shared_ptr<DataReader::Data> sampleBkg_; /*!< Pointer to background sample */
	std::shared_ptr<DataReader::Data> samplePhsp_; /*!< Pointer to phsp sample */
	std::shared_ptr<Amplitude> amp_; /*!< Pointer to signal model */
	std::shared_ptr<Amplitude> ampBkg_; /*!< Pointer to background model */
	std::shared_ptr<Optimizer::Optimizer> opti_; /*!< Pointer to Optimizer-Module */
	std::shared_ptr<Generator> gen_; /*!< Pointer to Generator-Module */
};

} /* namespace ComPWA */

#endif
