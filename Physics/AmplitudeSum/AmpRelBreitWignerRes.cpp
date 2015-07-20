//-------------------------------------------------------------------------------
// Copyright (c) 2013 Mathias Michel.
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the GNU Public License v3.0
// which accompanies this distribution, and is available at
// http://www.gnu.org/licenses/gpl.html
//
// Contributors:
//     Mathias Michel - initial API and implementation
//		Peter Weidenkaff - correct nominator, using dataPoint for data handling
//-------------------------------------------------------------------------------
//****************************************************************************
// Class for defining the relativistic Breit-Wigner resonance model, which
// includes the use of Blatt-Weisskopf barrier factors.
//****************************************************************************

// --CLASS DESCRIPTION [MODEL] --
// Class for defining the relativistic Breit-Wigner resonance model, which
// includes the use of Blatt-Weisskopf barrier factors.

#include <cmath>
#include "Physics/AmplitudeSum/AmpRelBreitWignerRes.hpp"
#include <stdlib.h>

AmpRelBreitWignerRes::AmpRelBreitWignerRes(const char *name,
		std::shared_ptr<DoubleParameter> mag, std::shared_ptr<DoubleParameter> phase,
		std::shared_ptr<DoubleParameter> mass, int subSys, Spin spin, Spin m, Spin n,
		std::shared_ptr<DoubleParameter> width,
		std::shared_ptr<DoubleParameter> mesonRadius,
		std::shared_ptr<DoubleParameter> motherRadius,
		int nCalls, normStyle nS) :
		AmpAbsDynamicalFunction(name, mag, phase, mass, subSys, spin, m, n,
				mesonRadius, motherRadius, nCalls, nS),
				_width(width)
{ }

AmpRelBreitWignerRes::~AmpRelBreitWignerRes() 
{
}

//std::complex<double> AmpRelBreitWignerRes::evaluate(dataPoint& point) {
//
//	return AmpAbsDynamicalFunction::evaluate(point);
//}

std::complex<double> AmpRelBreitWignerRes::evaluateAmp(dataPoint& point) {
	if(_ma == -999 || _mb == -999 ||_mc == -999 ||_M == -999){
		std::cout<<"Masses of decay products not set!"<<std::endl;
		return 0;
	}
	//do we have to recalculate the normalization?
	if( _width->GetValue() != tmp_width) {
		SetModified();
		tmp_width = _width->GetValue();
	}
	DalitzKinematics* kin = dynamic_cast<DalitzKinematics*>(Kinematics::instance());
	double mSq = -999;
	switch(_subSys){
	case 3: mSq=kin->getThirdVariableSq(point.getVal(0),point.getVal(1)); break;
	case 4: mSq=point.getVal(1); break;
	case 5: mSq=point.getVal(0); break;
	}

	return dynamicalFunction(mSq,_mass->GetValue(),_ma,_mb,_width->GetValue(),_spin,_mesonRadius->GetValue());
}
std::complex<double> AmpRelBreitWignerRes::dynamicalFunction(double mSq, double mR, double ma, double mb, double width, unsigned int J, double mesonRadius){
	std::complex<double> i(0,1);
	double sqrtS = sqrt(mSq);

	double barrier = Kinematics::FormFactor(sqrtS,ma,mb,J,mesonRadius)/Kinematics::FormFactor(mR,ma,mb,J,mesonRadius);

	std::complex<double> qTerm = std::pow((Kinematics::phspFactor(sqrtS,ma,mb) / Kinematics::phspFactor(mR,ma,mb))*mR/sqrtS, (2*J+ 1));
	//Calculate coupling constant to final state
	std::complex<double> g_final = widthToCoupling(mSq,mR,width,ma,mb,J,mesonRadius);

	//Coupling constant from production reaction. In case of a particle decay the production
	//coupling doesn't depend in energy since the CM energy is in the (RC) system fixed to the
	//mass of the decaying particle
	double g_production = 1;

	//-- Old approach
	//std::complex<double> denom(mR*mR - mSq + mR*(width*qTerm.imag()*barrier*barrier), (-1)*mR*(width*qTerm.real()*(mR/sqrtS)*barrier*barrier) );
	//std::complex<double> denom(mR*mR - mSq, (-1)*mR*(width*qTerm.real()*(mR/sqrtS)*barrier*barrier) );
	//std::complex<double> result = std::complex<double>(1,0) / denom; //OLD
	//-- New approach
	//std::complex<double> denom(mR*mR - mSq + sqrtS*(width*qTerm.imag()*barrier*barrier), (-1)*sqrtS*(width*qTerm.real()*barrier*barrier) );
	//std::complex<double> denom(mR*mR - mSq , (-1)*sqrtS*(width*qTerm.real()*barrier*barrier) );
	std::complex<double> denom = std::complex<double>( mR*mR - mSq,0) + (-1.0)*i*sqrtS*(width*qTerm*barrier);
	//	std::cout<<mR<<" "<<mSq<<" "<<qTerm<<std::endl;

	std::complex<double> result = g_final*g_production / denom;

	if(result.real()!=result.real() || result.imag()!=result.imag()){
		std::cout<<"AmpRelBreitWignerRes::dynamicalFunction() | "<<barrier<<" "<<mR<<" "<<mSq
				<<" "<<ma<<" "<<mb<<std::endl;
		return 0;
	}
	return result;
}

std::shared_ptr<FunctionTree> AmpRelBreitWignerRes::setupTree(
		allMasses& theMasses,allMasses& toyPhspSample,std::string suffix, ParameterList& params){

	DalitzKinematics* kin = dynamic_cast<DalitzKinematics*>(Kinematics::instance());
	double phspVol = kin->getPhspVolume();
	BOOST_LOG_TRIVIAL(info) << "AmpRelBreitWignerRes::setupBasicTree() | "<<_name;
	//------------Setup Tree---------------------
	std::shared_ptr<FunctionTree> newTree(new FunctionTree());
	//------------Setup Tree Pars---------------------
	std::shared_ptr<MultiDouble> m23sq( new MultiDouble("m23sq",theMasses.masses_sq.at( std::make_pair(2,3) )) );
	std::shared_ptr<MultiDouble> m13sq( new MultiDouble("m13sq",theMasses.masses_sq.at( std::make_pair(1,3) )) );
	std::shared_ptr<MultiDouble> m12sq( new MultiDouble("m12sq",theMasses.masses_sq.at( std::make_pair(1,2) )) );
	std::shared_ptr<MultiDouble> m23sq_phsp( new MultiDouble("m23sq_phsp",toyPhspSample.masses_sq.at( std::make_pair(2,3) )) );
	std::shared_ptr<MultiDouble> m13sq_phsp( new MultiDouble("m13sq_phsp",toyPhspSample.masses_sq.at( std::make_pair(1,3) )) );
	std::shared_ptr<MultiDouble> m12sq_phsp( new MultiDouble("m12sq_phsp",toyPhspSample.masses_sq.at( std::make_pair(1,2) )) );

	//----Strategies needed
	std::shared_ptr<MultAll> mmultStrat(new MultAll(ParType::MCOMPLEX));
	std::shared_ptr<MultAll> mmultDStrat(new MultAll(ParType::MDOUBLE));
	std::shared_ptr<AddAll> maddStrat(new AddAll(ParType::MCOMPLEX));
	std::shared_ptr<AbsSquare> msqStrat(new AbsSquare(ParType::MDOUBLE));
	std::shared_ptr<LogOf> mlogStrat(new LogOf(ParType::MDOUBLE));
	std::shared_ptr<MultAll> multStrat(new MultAll(ParType::COMPLEX));
	std::shared_ptr<MultAll> multDStrat(new MultAll(ParType::DOUBLE));
	std::shared_ptr<AddAll> addStrat(new AddAll(ParType::DOUBLE));
	std::shared_ptr<AddAll> addComplexStrat(new AddAll(ParType::COMPLEX));
	std::shared_ptr<AbsSquare> sqStrat(new AbsSquare(ParType::DOUBLE));
	std::shared_ptr<LogOf> logStrat(new LogOf(ParType::DOUBLE));
	std::shared_ptr<Complexify> complStrat(new Complexify(ParType::COMPLEX));
	std::shared_ptr<Inverse> invStrat(new Inverse(ParType::DOUBLE));
	std::shared_ptr<SquareRoot> sqRootStrat(new SquareRoot(ParType::DOUBLE));

	//----Add Nodes
	std::shared_ptr<BreitWignerStrategy> rbwStrat( new BreitWignerStrategy(_name,ParType::MCOMPLEX) );
	std::shared_ptr<BreitWignerPhspStrategy> rbwPhspStrat( new BreitWignerPhspStrategy(_name,ParType::MCOMPLEX) );
	std::shared_ptr<WignerDStrategy> angdStrat(	new WignerDStrategy(_name,ParType::MDOUBLE) );
	std::shared_ptr<WignerDPhspStrategy> angdPhspStrat(	new WignerDPhspStrategy(_name,ParType::MDOUBLE) );

	newTree->createHead("Reso_"+_name, mmultStrat, theMasses.nEvents); //Reso=BW*C_*AD*N_

	newTree->createNode("BW_"+_name, mmultStrat , "Reso_"+_name, theMasses.nEvents); //BW
	newTree->createNode("RelBW_"+_name, rbwStrat, "BW_"+_name, theMasses.nEvents); //BW
	newTree->createNode("C_"+_name, complStrat, "Reso_"+_name); //m0c
	newTree->createLeaf("Intens_"+_name, params.GetDoubleParameter("mag_"+_name), "C_"+_name); //r
	newTree->createLeaf("Phase_"+_name, params.GetDoubleParameter("phase_"+_name), "C_"+_name); //phi
	newTree->createNode("AngD_"+_name, angdStrat, "Reso_"+_name, theMasses.nEvents); //AD

	//Breit-Wigner
	newTree->createLeaf("m0_"+_name, params.GetDoubleParameter("m0_"+_name), "RelBW_"+_name); //m0
	newTree->createLeaf("m23sq", m23sq, "RelBW_"+_name); //ma
	newTree->createLeaf("m13sq", m13sq, "RelBW_"+_name); //mb
	newTree->createLeaf("m12sq", m12sq, "RelBW_"+_name); //mc
	newTree->createLeaf("subSysFlag_"+_name, _subSys, "RelBW_"+_name); //subSysFlag
	newTree->createLeaf("spin_"+_name, _spin, "RelBW_"+_name); //spin
	newTree->createLeaf("d_"+_name, params.GetDoubleParameter("d_"+_name), "RelBW_"+_name); //d
	newTree->createLeaf("width_"+_name, params.GetDoubleParameter("width_"+_name), "RelBW_"+_name); //resWidth
	//Angular distribution
	newTree->createLeaf("m23sq", m23sq, "AngD_"+_name); //ma
	newTree->createLeaf("m13sq", m13sq, "AngD_"+_name); //mb
	newTree->createLeaf("m12sq", m12sq, "AngD_"+_name); //mc
	newTree->createLeaf("M", kin->M, "AngD_"+_name); //M
	newTree->createLeaf("m1", kin->m1, "AngD_"+_name); //m1
	newTree->createLeaf("m2", kin->m2, "AngD_"+_name); //m2
	newTree->createLeaf("m3", kin->m3, "AngD_"+_name); //m3
	newTree->createLeaf("subSysFlag_"+_name, _subSys, "AngD_"+_name); //subSysFlag
	newTree->createLeaf("spin_"+_name,_spin, "AngD_"+_name); //spin
	newTree->createLeaf("m_"+_name, 0, "AngD_"+_name); //OutSpin 1
	newTree->createLeaf("n_"+_name, 0, "AngD_"+_name); //OutSpin 2

	//adding nodes and leafs for calculation of normalization
	if(_normStyle==normStyle::none){
		newTree->createLeaf("N_"+_name, 1., "BW_"+_name);
	}else{
		//Normalization parameter for dynamical amplitude
		newTree->createNode("N_"+_name, sqRootStrat, "BW_"+_name); //N = sqrt(NSq)
		newTree->createNode("NSq_"+_name, multDStrat, "N_"+_name); //NSq = (PhspVolume/N_phspMC * Sum(|A|^2))^-1
		newTree->createLeaf("PhspSize_"+_name, toyPhspSample.nEvents, "NSq_"+_name); // N_phspMC
		newTree->createLeaf("PhspVolume_"+_name, 1/phspVol, "NSq_"+_name); // 1/PhspVolume
		newTree->createNode("InvSum_"+_name, invStrat, "NSq_"+_name); //1/Sum(|A|^2)
		newTree->createNode("Sum_"+_name, addStrat, "InvSum_"+_name); //Sum(|A|^2)
		newTree->createNode("AbsVal_"+_name, msqStrat, "Sum_"+_name); //|A_i|^2
		//Breit-Wigner (Normalization)
		newTree->createNode("NormReso_"+_name, mmultStrat, "AbsVal_"+_name, toyPhspSample.nEvents); //BW

		newTree->createNode("NormBW_"+_name, rbwPhspStrat, "NormReso_"+_name, toyPhspSample.nEvents); //BW
		newTree->createLeaf("m0_"+_name, params.GetDoubleParameter("m0_"+_name), "NormBW_"+_name); //m0
		newTree->createLeaf("m23sq_phsp", m23sq_phsp, "NormBW_"+_name); //ma
		newTree->createLeaf("m13sq_phsp", m13sq_phsp, "NormBW_"+_name); //mb
		newTree->createLeaf("m12sq_phsp", m12sq_phsp, "NormBW_"+_name); //mc
		newTree->createLeaf("subSysFlag_"+_name, _subSys, "NormBW_"+_name); //subSysFlag
		newTree->createLeaf("spin_"+_name, _spin, "NormBW_"+_name); //spin
		newTree->createLeaf("d_"+_name, params.GetDoubleParameter("d_"+_name), "NormBW_"+_name); //d
		newTree->createLeaf("width_"+_name, params.GetDoubleParameter("width_"+_name), "NormBW_"+_name); //resWidth

		//Angular distribution (Normalization)
		newTree->createNode("NormAngD_"+_name, angdPhspStrat, "NormReso_"+_name, toyPhspSample.nEvents); //AD
		newTree->createLeaf("m23sq_phsp", m23sq_phsp, "NormAngD_"+_name); //ma
		newTree->createLeaf("m13sq_phsp", m13sq_phsp, "NormAngD_"+_name); //mb
		newTree->createLeaf("m12sq_phsp", m12sq_phsp, "NormAngD_"+_name); //mc
		newTree->createLeaf("M", kin->M, "NormAngD_"+_name); //M
		newTree->createLeaf("m1", kin->m1, "NormAngD_"+_name); //m1
		newTree->createLeaf("m2", kin->m2, "NormAngD_"+_name); //m2
		newTree->createLeaf("m3", kin->m3, "NormAngD_"+_name); //m3
		newTree->createLeaf("subSysFlag_"+_name, _subSys, "NormAngD_"+_name); //subSysFlag
		newTree->createLeaf("spin_"+_name,_spin, "NormAngD_"+_name); //spin
		newTree->createLeaf("m_"+_name, 0, "NormAngD_"+_name); //OutSpin 1
		newTree->createLeaf("n_"+_name, 0, "NormAngD_"+_name); //OutSpin 2
	}
	switch(_subSys){
	case 3:{ //reso in sys of particles 1&2
		newTree->createLeaf("ma_"+_name, kin->m1, "RelBW_"+_name); //ma
		newTree->createLeaf("mb_"+_name, kin->m2, "RelBW_"+_name); //mb
		if(_normStyle!=normStyle::none){
			newTree->createLeaf("ma_"+_name, kin->m1, "NormBW_"+_name); //ma
			newTree->createLeaf("mb_"+_name, kin->m2, "NormBW_"+_name); //mb
		}
		break;
	}
	case 4:{ //reso in sys of particles 1&3
		//newTree->createLeaf("mym_"+_name, m13, "RelBW_"+_name); //m
		newTree->createLeaf("ma_"+_name, kin->m1, "RelBW_"+_name); //ma
		newTree->createLeaf("mb_"+_name, kin->m3, "RelBW_"+_name); //mb
		if(_normStyle!=normStyle::none){
			newTree->createLeaf("ma_"+_name, kin->m1, "NormBW_"+_name); //ma
			newTree->createLeaf("mb_"+_name, kin->m3, "NormBW_"+_name); //mb
		}
		break;
	}
	case 5:{ //reso in sys of particles 2&3
		//newTree->createLeaf("mym_"+_name, m23, "RelBW_"+_name); //m
		newTree->createLeaf("ma_"+_name, kin->m2, "RelBW_"+_name); //ma
		newTree->createLeaf("mb_"+_name, kin->m3, "RelBW_"+_name); //mb
		if(_normStyle!=normStyle::none){
			newTree->createLeaf("ma_"+_name, kin->m2, "NormBW_"+_name); //ma
			newTree->createLeaf("mb_"+_name, kin->m3, "NormBW_"+_name); //mb
		}
		break;
	}
	default:{
		BOOST_LOG_TRIVIAL(error)<<"AmpSumIntensity::setupBasicTree(): Subsys not found!!";
	}
	}
	return newTree;

}
