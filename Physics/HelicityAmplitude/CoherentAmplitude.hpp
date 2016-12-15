//-------------------------------------------------------------------------------
// Copyright (c) 2013 Stefan Pflueger.
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the GNU Public License v3.0
// which accompanies this distribution, and is available at
// http://www.gnu.org/licenses/gpl.html
//
// Contributors:
//   Stefan Pflueger - initial API and implementation
//-------------------------------------------------------------------------------

#ifndef PHYSICS_HELICITYAMPLITUDE_COHERENTAMPLITUDE_HPP_
#define PHYSICS_HELICITYAMPLITUDE_COHERENTAMPLITUDE_HPP_

#include "Core/Amplitude.hpp"
#include "Core/Efficiency.hpp"

#include "Physics/HelicityAmplitude/TopologyAmplitude.hpp"
#include "Physics/HelicityAmplitude/TwoBodyDecayAmplitude.hpp"
#include "Physics/DynamicalDecayFunctions/AbstractDynamicalFunction.hpp"
#include "Physics/HelicityAmplitude/TwoBodyDecayAngularStrategy.hpp"
#include "Physics/DynamicalDecayFunctions/DynamicalFunctionStrategy.hpp"

namespace ComPWA {
namespace Physics {
namespace HelicityFormalism {

struct SequentialDecayInformation {
  unsigned int top_node_;
  std::map<unsigned int, IndexList> unique_id_decay_tree_;

  bool operator<(const SequentialDecayInformation &rhs) const {
    if (this->top_node_ < rhs.top_node_)
      return true;
    else if (this->top_node_ > rhs.top_node_)
      return false;
    if (this->unique_id_decay_tree_ < rhs.unique_id_decay_tree_)
      return true;

    return false;
  }
};

class CoherentAmplitude: public Amplitude {
  std::vector<TopologyAmplitude> topology_amplitudes_;

  std::map<unsigned int, std::shared_ptr<FunctionTree> > tree_;

  std::vector<ParticleStateInfo> particles_;
  std::map<std::set<unsigned int>, unsigned int> sequential_decay_amplitudes_map_;
  std::vector<std::shared_ptr<TreeNode> > sequential_decay_amplitudes_vec_;

  std::map<std::string, std::shared_ptr<TreeNode> > angular_part_nodes_;
  std::map<std::string,
      std::shared_ptr<TreeNode> > dynamical_part_nodes_;
  std::map<std::string, std::shared_ptr<AbsParameter> > dynamical_parameter_nodes_;
  std::map<std::string, std::shared_ptr<TreeNode> > full_two_body_decay_nodes_;
  std::map<std::string, std::vector<std::pair<std::string, std::shared_ptr<AbsParameter> > > > tree_leaves_;

  std::map<std::string, std::shared_ptr<TwoBodyDecayAngularStrategy> > angular_part_strategies_;
  std::map<std::string,
      std::shared_ptr<DynamicalFunctions::DynamicalFunctionStrategy> > dynamical_part_strategies_;

  std::shared_ptr<Efficiency> efficiency_;

  bool use_coherent_background;

  ParameterList parameters_;
  std::shared_ptr<DoubleParameter> result_value_;
  std::vector<std::vector<IndexList> > data_point_index_lists_;

  bool wasMaxAmplitudeValueCalculated_;
  double maxAmplitudeValue_;

  void init();

  void constructSequentialDecayTreeNodes(unsigned int storage_index);
  void constructCoherentAmpTree(unsigned int storage_index);
  void clearMaps();

  std::vector<IndexList> convertIndexLists(
      const std::vector<IndexList> evaluation_lists) const;

  std::vector<std::shared_ptr<AbsParameter> > createLeaves(
      const ParameterList& parameter_list);

  IndexList getListOfCoherentPartners(
      const std::pair<std::set<unsigned int>, unsigned int>& seq_amp) const;

  std::set<unsigned int> removeCoherentParticleIndices(
      const std::set<unsigned int>& particles_indices) const;
 /* bool isCoherentPartner(
      const std::pair<SequentialDecayInformation, unsigned int>& seq_amp_partner,
      const std::pair<SequentialDecayInformation, unsigned int>& seq_amp_ref) const;*/

  bool compareCoherentParticleStates(const ParticleStateInfo& ps,
      const ParticleStateInfo& ref) const;

public:
  CoherentAmplitude(const std::vector<TopologyAmplitude>& amplitude_trees);
  virtual ~CoherentAmplitude();

  const double integral();
  const double integral(ParameterList& par);
  const double normalization();
  const double normalization(ParameterList& par);
  double getMaxVal(ParameterList& par, std::shared_ptr<Generator> gen);
  double getMaxVal(std::shared_ptr<Generator> gen);

  virtual bool hasTree();
  std::shared_ptr<FunctionTree> getAmpTree(allMasses&, allMasses&, std::string);

  const ParameterList& intensity(const dataPoint& point, ParameterList& par);
  const ParameterList& intensity(const dataPoint& point);
  const ParameterList& intensityNoEff(const dataPoint& point);
  const ParameterList& intensity(std::vector<double> point, ParameterList& par);

  const bool fillStartParVec(ParameterList& outPar);
  void setParameterList(const ParameterList& par);
  bool copyParameterList(ParameterList& par);

  double getIntValue(std::string var1, double min1, double max1,
      std::string var2, double min2, double max2);

  void printAmps();
  void printFractions();

  Amplitude* Clone();
};

} /* namespace HelicityFormalism */
} /* namespace Physics */
} /* namespace ComPWA */

#endif /* PHYSICS_HELICITYAMPLITUDE_COHERENTAMPLITUDE_HPP_ */
