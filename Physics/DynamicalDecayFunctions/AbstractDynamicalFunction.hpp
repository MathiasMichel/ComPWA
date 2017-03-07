//-------------------------------------------------------------------------------
// Copyright (c) 2013 Stefan Pflueger.
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the GNU Public License v3.0
// which accompanies this distribution, and is available at
// http://www.gnu.org/licenses/gpl.html
//
// Contributors:
//   Stefan Pflueger - initial API and implementation
//----------------------------------------------------------------------------------

#ifndef PHYSICS_HELICITYAMPLITUDE_ABSTRACTDYNAMICALFUNCTION_HPP_
#define PHYSICS_HELICITYAMPLITUDE_ABSTRACTDYNAMICALFUNCTION_HPP_

#include <complex>

#include "boost/property_tree/ptree_fwd.hpp"

#include "Core/ParameterList.hpp"

namespace ComPWA {

class dataPoint;

namespace Physics {
namespace DynamicalFunctions {

struct ExternalParameters {
  ParameterList parameters_;
  std::map<std::string, std::string> parameter_name_mapping_;
};

class AbstractDynamicalFunction {
  virtual void initialiseParameters(
      const boost::property_tree::ptree& parameter_info,
      const ExternalParameters& external_parameters) =0;

protected:
  ParameterList parameter_list_;
public:
  AbstractDynamicalFunction();
  virtual ~AbstractDynamicalFunction();

  virtual std::complex<double> evaluate(const dataPoint& point,
      unsigned int evaluation_index) const =0;

  virtual std::complex<double> evaluate(unsigned int storage_index,
      unsigned int data_index, unsigned int evaluation_index) const =0;

  const ParameterList& getParameterList() const;
};

} /* namespace DynamicalFunctions */
} /* namespace Physics */
} /* namespace ComPWA */

#endif /* PHYSICS_HELICITYAMPLITUDE_ABSTRACTDYNAMICALFUNCTION_HPP_ */