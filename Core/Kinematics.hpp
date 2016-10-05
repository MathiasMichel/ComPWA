//-------------------------------------------------------------------------------
// Copyright (c) 2013 Peter Weidenkaff.
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the GNU Public License v3.0
// which accompanies this distribution, and is available at
// http://www.gnu.org/licenses/gpl.html
//
// Contributors:
//		Peter Weidenkaff -
//-------------------------------------------------------------------------------

#ifndef KINEMATICS_HPP_
#define KINEMATICS_HPP_

#include <vector>
#include <string>

namespace ComPWA {

class Event;
class dataPoint;

class Kinematics {
public:
  //! singleton pattern
  static Kinematics* instance();

  //! vector with names of variables, e.g. vec[0]=m23sq, vec[1]=m13sq
  const std::vector<std::string>& getVarNames() const {
    return variable_names_;
  }

  unsigned int getVariableIndex(std::string name) const;

  //! checks of data point is within phase space boundaries
  virtual bool isWithinPhsp(const dataPoint& point) = 0;
  //! mass of mother particle
  virtual double getMotherMass() const = 0;
  //! calculated the PHSP volume of the current decay by MC integration
  virtual double getPhspVolume();
  //! converts Event to dataPoint
  virtual void eventToDataPoint(const Event& ev, dataPoint& point) const;
  //! get mass of particles
  virtual double getMass(unsigned int num) const = 0;
  //! get pid of particles
  virtual int getPID(unsigned int num) const {
    return pids_[num];
  }
  //! get mass of paticles
  virtual double getMass(std::string name) const = 0;
  //! Get number of particles
  virtual unsigned int getNumberOfParticles() const {
    return number_of_particles_;
  }
  //! Get number of variables
  virtual unsigned int getNumberOfVariables() const {
    return variable_names_.size();
  }

protected:
  static Kinematics* inst_;

  bool is_PS_area_calculated_;
  double PS_area_;

  unsigned int number_of_particles_;
  std::vector<std::string> variable_names_;
  std::vector<int> pids_;

  Kinematics();
  virtual ~Kinematics();

  virtual double calculatePSArea() =0;
  virtual void translateEventToDataPoint(const Event& ev,
      dataPoint& point) const =0;

  // delete methods to ensure that there will only be one instance
  Kinematics(const Kinematics&) = delete;
  void operator=(const Kinematics&) = delete;
};

} /* namespace ComPWA */

#endif /* KINEMATICS_HPP_ */
