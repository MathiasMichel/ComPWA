//-------------------------------------------------------------------------------
// Copyright (c) 2013 Peter Weidenkaff.
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the GNU Public License v3.0
// which accompanies this distribution, and is available at
// http://www.gnu.org/licenses/gpl.html
//
// Contributors:
//     Peter Weidenkaff - initial API
//-------------------------------------------------------------------------------

//! PhysConst provides particle information to the framework
/*!
 * @file PhysConst.hpp
 *\class PhysConst
 *      PhysConst reads particle properties from an xml file. And provides
 *      it to the framework via a singleton structure. The structure of
 *      the xml file is defined in particleSchema.xsd. It is also intended
 *      to store other physical constants in this way.
 */

#ifndef PHYSCONST_HPP_
#define PHYSCONST_HPP_

#include <iostream>
#include <vector>
#include <Core/Parameter.hpp>

struct ParticleProperties {
  std::string name_;
  double mass_;
  double width_;
  int id_;
  int charge_;
  unsigned int isospin_;
  int isospin_z_;
  unsigned int spin_;
  int parity_;
  int cparity_;

  ParticleProperties() :
      name_(""), mass_(0.0), width_(0.0), id_(0), charge_(0), isospin_(0), isospin_z_(
          0), spin_(0), parity_(0), cparity_(0) {
  }
};

struct Constant {
  std::string name_;
  double value_;
  double error_;

  Constant() :
      name_(""), value_(0.0), error_(0.0) {
  }
};

class PhysConst {
public:
  //! returns existing instance of PhysConst, or creates a new one
  static PhysConst& Instance() {
    static PhysConst instance;
    return instance;
  }
  ~PhysConst();

  PhysConst(PhysConst const&) = delete;
  void operator=(PhysConst const&) = delete;

  const Constant& findConstant(const std::string& name) const;
  const ParticleProperties& findParticle(const std::string& name) const;
  const ParticleProperties& findParticle(int pid) const;
  bool particleExists(const std::string& name) const;

private:
  PhysConst();

  void readFile();

  std::string particleFileName;
  std::string particleDefaultFileName;
  std::string constantFileName;
  std::string constantDefaultFileName;
  std::vector<ParticleProperties> particle_properties_list_;
  std::vector<Constant> constants_list_;
};

#endif /* PHYSCONST_HPP_ */
