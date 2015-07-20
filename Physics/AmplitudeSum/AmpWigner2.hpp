//-------------------------------------------------------------------------------
// Copyright (c) 2013 Mathias Michel.
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the GNU Public License v3.0
// which accompanies this distribution, and is available at
// http://www.gnu.org/licenses/gpl.html
//
// Contributors:
//     Mathias Michel
//     Peter Weidenkaff
//-------------------------------------------------------------------------------

//! Angular distribution based on WignerD functions
/*!
 * @file AmpWigner2.hpp
 *\class AmpWigner2
 *The helicity angle for sub system \_subSys is calculated and the value of the WignerD function is returned
 */

#ifndef AMPWIGNER2
#define AMPWIGNER2

#include <vector>
#include <memory>

#include "qft++.h"

#include "Core/ParameterList.hpp"
#include "Core/Functions.hpp"
#include "Physics/DPKinematics/DalitzKinematics.hpp"
#include "Core/DataPoint.hpp"

//using namespace std;
class AmpWigner2{
public:
	AmpWigner2( unsigned int subSys, unsigned int spin );

	virtual ~AmpWigner2() {};

	virtual double evaluate(dataPoint& point) ;

	static double dynamicalFunction(int J, int mu, int muPrime, double cosTheta);
protected:
	unsigned int _subSys;
	unsigned int _spin;
};

class WignerDStrategy : public Strategy {
public:
	WignerDStrategy(const std::string resonanceName, ParType in):Strategy(in),name(resonanceName){ }

	virtual const std::string to_str() const { return ("WignerD of "+name);	}

	virtual bool execute(ParameterList& paras, std::shared_ptr<AbsParameter>& out);

protected:
	std::string name;
};

class WignerDPhspStrategy : public Strategy {
public:
	WignerDPhspStrategy(const std::string resonanceName, ParType in):Strategy(in),name(resonanceName){ }

	virtual const std::string to_str() const { return ("WignerD of "+name);	}

	virtual bool execute(ParameterList& paras, std::shared_ptr<AbsParameter>& out);

protected:
	std::string name;
};

#endif
