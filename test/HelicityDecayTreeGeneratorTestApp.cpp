#include <fstream>

#include "boost/filesystem.hpp"

#include "Core/PhysConst.hpp"

#include "Physics/DecayTree/DecayGenerator.hpp"
#include "Physics/DecayTree/DecayGeneratorFacade.hpp"
#include "Physics/DecayTree/DecayGeneratorConfig.hpp"
#include "Physics/DecayTree/DecayXMLConfigReader.hpp"

int main(int argc, char **argv) {
  std::cout << "  ComPWA Copyright (C) 2013  Stefan Pflueger " << std::endl;
  std::cout
      << "  This program comes with ABSOLUTELY NO WARRANTY; for details see license.txt"
      << std::endl;
  std::cout << std::endl;

  if (argc < 2) {
    std::runtime_error("Error: please specify a config url!");
  }

  boost::filesystem::path output_path(argv[1]);
  output_path = output_path.parent_path();

  // first read the config file
  ComPWA::Physics::DecayTree::DecayGeneratorConfig::Instance().readConfig(
      std::string(argv[1]));

  ComPWA::Physics::DecayTree::DecayGenerator decay_generator;
  // initialize

  ComPWA::Physics::DecayTree::DecayGeneratorFacade decay_generator_facade(
      decay_generator);

  decay_generator_facade.setAllowedSpinQuantumNumbers(
      ComPWA::QuantumNumberIDs::SPIN, { 0, 1, 2, 4 }, 1, {
          ComPWA::QuantumNumberIDs::ORBITAL_ANGULAR_MOMENTUM });
  decay_generator_facade.setAllowedSpinQuantumNumbers(
      ComPWA::QuantumNumberIDs::ORBITAL_ANGULAR_MOMENTUM, { 0, 1, 2, 4 }, 1, { },
      ComPWA::Physics::DecayTree::QuantumNumberTypes::COMPOSITE_PARTICLE_BASED);
  decay_generator_facade.setAllowedSpinQuantumNumbers(
      ComPWA::QuantumNumberIDs::ISOSPIN, { 0, 1 }, 1, { });
  decay_generator_facade.setAllowedIntQuantumNumbers(
      ComPWA::QuantumNumberIDs::CHARGE, { -1, 0, 1 }, { });
  decay_generator_facade.setAllowedIntQuantumNumbers(
      ComPWA::QuantumNumberIDs::PARITY, { -1, 1 }, {
          ComPWA::QuantumNumberIDs::ORBITAL_ANGULAR_MOMENTUM });
  decay_generator_facade.setAllowedIntQuantumNumbers(
      ComPWA::QuantumNumberIDs::CPARITY, { -1, 1 }, {
          ComPWA::QuantumNumberIDs::CHARGE });

  decay_generator_facade.setConservedQuantumNumbers( {
      ComPWA::QuantumNumberIDs::CHARGE, ComPWA::QuantumNumberIDs::PARITY,
      ComPWA::QuantumNumberIDs::CPARITY, ComPWA::QuantumNumberIDs::SPIN,
      ComPWA::QuantumNumberIDs::ORBITAL_ANGULAR_MOMENTUM });

  /* ComPWA::Physics::DecayTree::IFParticleInfo if_particle =
   decay_generator.createIFParticleInfo("pi+");
   decay_generator.addFinalStateParticles(if_particle);
   if_particle = decay_generator.createIFParticleInfo("pi-");
   decay_generator.addFinalStateParticles(if_particle);

   if_particle = decay_generator.createIFParticleInfo("f0_980");
   decay_generator.setTopNodeState(if_particle);*/

  ComPWA::Physics::DecayTree::DecayConfiguration decay_config =
      decay_generator.createDecayConfiguration();

  decay_config.printInfo();

  ComPWA::Physics::DecayTree::DecayXMLConfigReader xml_config_io(decay_config);
  std::cout << "Saving decay configuration to xml file...\n";
  xml_config_io.writeConfig(output_path.string() + "/test.xml");
  std::cout << "Done!\n";

  return 0;
}

