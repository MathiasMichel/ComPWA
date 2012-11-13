//! Physics Interface Base-Class.
/*! \class PIFBase
 * @file PIFBase.hpp
 * This class provides the interface to the model which tries to describe the
 * intensity. As it is pure virtual, one needs at least one implementation to
 * provide an model for the analysis which calculates intensities for an event on
 * basis model parameters. If a new physics-model is derived from and fulfills
 * this base-class, no change in other modules are necessary to work with the new
 * physics module.
*/

#ifndef PIFBASE_HPP_
#define PIFBASE_HPP_

#include <vector>
#include "PWAParameter.hpp"

class PIFBase
{

public:

  PIFBase()
	  {
	  }

  virtual ~PIFBase()
	{ /* nothing */	}

  //virtual const double intensity(double x, double M, double T) =0;
  virtual const double intensity(std::vector<double> x, std::vector<PWAParameter<double> > par) =0;

};

#endif
