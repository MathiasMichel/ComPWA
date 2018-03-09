""" sample script for the testing purposes using the decay
    JPsi -> gamma pi0 pi0
"""
from core.topology.graph import InteractionNode
from core.topology.topologybuilder import SimpleStateTransitionTopologyBuilder

from core.state.particle import (
    load_particle_list_from_xml, particle_list,
    initialize_graph, initialize_graphs_with_particles,
    ParticleQuantumNumberNames, InteractionQuantumNumberNames,
    create_spin_domain)
from core.state.propagation import (CSPPropagator)
from core.state.conservationrules import (AdditiveQuantumNumberConservation,
                                          ParityConservation,
                                          IdenticalParticleSymmetrization,
                                          SpinConservation,
                                          HelicityConservation,
                                          CParityConservation,
                                          GParityConservation,
                                          GellMannNishijimaRule)


# ------------------ Creation of topology graphs ------------------

TwoBodyDecayNode = InteractionNode("TwoBodyDecay", 1, 2)

SimpleBuilder = SimpleStateTransitionTopologyBuilder([TwoBodyDecayNode])

all_graphs = SimpleBuilder.build_graphs(1, 3)

print("we have " + str(len(all_graphs)) + " tolopogy graphs!\n")
print(all_graphs)

# ------------------ first stage of QN propagation ------------------

# load default particles from database/file
load_particle_list_from_xml('../particle_list.xml')
# print(particle_list)
print("loaded " + str(len(particle_list)) + " particles from xml file!")

# initialize the graph edges (intial and final state)
initial_state = [("J/psi", [-1, 1])]
final_state = [("gamma", [-1, 1]), ("pi0", [0]), ("pi0", [0])]
init_graphs = initialize_graph(
    all_graphs[0], initial_state, final_state)
print("initialized " + str(len(init_graphs)) + " graphs!")
test_graph = init_graphs[0]


conservation_rules = {
    'strict':
    [AdditiveQuantumNumberConservation(ParticleQuantumNumberNames.Charge),
     AdditiveQuantumNumberConservation(
         ParticleQuantumNumberNames.BaryonNumber),
     #  AdditiveQuantumNumberConservation(
     #      ParticleQuantumNumberNames.LeptonNumber),
     ParityConservation(),
     IdenticalParticleSymmetrization(),
     SpinConservation(ParticleQuantumNumberNames.Spin, False),
     HelicityConservation(),
     GellMannNishijimaRule()
     ],
    'non-strict':
    [SpinConservation(
        ParticleQuantumNumberNames.IsoSpin)]
}

quantum_number_domains = {
    ParticleQuantumNumberNames.Charge: [-2, -1, 0, 1, 2],
    ParticleQuantumNumberNames.BaryonNumber: [-2, -1, 0, 1, 2],
    #  ParticleQuantumNumberNames.LeptonNumber: [-2, -1, 0, 1, 2],
    ParticleQuantumNumberNames.Parity: [-1, 1],
    ParticleQuantumNumberNames.Spin: create_spin_domain([0, 1, 2]),
    ParticleQuantumNumberNames.IsoSpin: create_spin_domain([0]),
    InteractionQuantumNumberNames.L: create_spin_domain([0, 1, 2], True)
}

propagator = CSPPropagator(test_graph)
propagator.assign_conservation_laws_to_all_nodes(
    conservation_rules, quantum_number_domains)
solutions = propagator.find_solutions()

print("found " + str(len(solutions)) + " solutions!")

for g in solutions:
    print(g.node_props[0])
    print(g.node_props[1])
    print(g.edge_props[1])

# ------------------ second stage of QN propagation ------------------

# specify set of particles which are allowed to be intermediate particles
# if list is empty, then all particles in the default particle list are used
allowed_intermediate_particles = []

full_particle_graphs = initialize_graphs_with_particles(
    solutions, allowed_intermediate_particles)

# C and G parity need to know the actual particle (pid)
# so these rules only work in the second stage
test_graph = full_particle_graphs[0]
print(test_graph)
propagator_stage2 = CSPPropagator(test_graph)
propagator_stage2.assign_conservation_laws_to_all_nodes(
    {'strict': [CParityConservation()]},
    {ParticleQuantumNumberNames.Cparity: [-1, 1]})
propagator_stage2.assign_conservation_laws_to_node(
    1,
    {'strict': [
        GParityConservation()]},
    {ParticleQuantumNumberNames.Gparity: [-1, 1]}
)
solutions_stage2 = propagator_stage2.find_solutions()

print("found " + str(len(solutions_stage2)) + " particle solutions!")

for g in solutions_stage2:
    print(g)
