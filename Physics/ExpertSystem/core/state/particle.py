""" This module defines a particle as a collection of quantum numbers and
things related to this"""
from copy import deepcopy
from enum import Enum
from abc import ABC, abstractmethod
from numpy import arange

import xmltodict

from core.topology.graph import (
    get_initial_state_edges, get_final_state_edges)


XMLLabelConstants = Enum('XMLLabelConstants',
                         'Name Type Value QuantumNumber \
                          Class Projection Parameter')


def get_xml_label(enum):
    """return the the correctly formatted xml label
    as required by ComPWA and xmltodict"""

    # the xml attribute prefix is needed as the xmltodict module uses that
    attribute_prefix = '@'
    if enum is (XMLLabelConstants.QuantumNumber or
                XMLLabelConstants.Parameter):
        return enum.name
    else:
        return attribute_prefix + enum.name


class Spin():
    def __init__(self, mag, proj):
        self.__magnitude = float(mag)
        self.__projection = float(proj)
        if self.__magnitude < self.__projection:
            raise ValueError("The spin projection cannot be larger than the"
                             "magnitude")

    def magnitude(self):
        return self.__magnitude

    def projection(self):
        return self.__projection

    def __eq__(self, other):
        if isinstance(other, Spin):
            return (self.__magnitude == other.magnitude() and
                    self.__projection == other.projection())
        else:
            return NotImplemented


def create_spin_domain(list_of_magnitudes, set_projection_zero=False):
    domain_list = []
    for mag in list_of_magnitudes:
        if set_projection_zero:
            domain_list.append(Spin(mag, 0.0))
        else:
            for proj in arange(-mag, mag + 1, 1.0):
                domain_list.append(Spin(mag, proj))
    return domain_list


QuantumNumberClasses = Enum('QuantumNumberClasses', 'Int Spin')

"""definition of quantum number names for particles"""
ParticleQuantumNumberNames = Enum(
    'ParticleQuantumNumbers', 'All Charge Spin IsoSpin Parity Cparity')

"""definition of quantum number names for interaction nodes"""
InteractionQuantumNumberNames = Enum('InteractionQuantumNumbers', 'L')

QNNameClassMapping = {ParticleQuantumNumberNames.Charge: QuantumNumberClasses.Int,
                      ParticleQuantumNumberNames.Spin: QuantumNumberClasses.Spin,
                      ParticleQuantumNumberNames.IsoSpin: QuantumNumberClasses.Spin,
                      ParticleQuantumNumberNames.Parity: QuantumNumberClasses.Int,
                      ParticleQuantumNumberNames.Cparity: QuantumNumberClasses.Int,
                      InteractionQuantumNumberNames.L: QuantumNumberClasses.Spin
                      }


class AbstractQNConverter(ABC):
    @abstractmethod
    def parse_from_dict(self, data_dict):
        pass

    @abstractmethod
    def convert_to_dict(self, qn_type, qn_value):
        pass


class IntQNConverter(AbstractQNConverter):
    value_label = get_xml_label(XMLLabelConstants.Value)
    type_label = get_xml_label(XMLLabelConstants.Type)
    class_label = get_xml_label(XMLLabelConstants.Class)

    def parse_from_dict(self, data_dict):
        return int(data_dict[self.value_label])

    def convert_to_dict(self, qn_type, qn_value):
        return {self.type_label: qn_type.name,
                self.class_label: QuantumNumberClasses.Int.name,
                self.value_label: str(qn_value)}


class SpinQNConverter(AbstractQNConverter):
    type_label = get_xml_label(XMLLabelConstants.Type)
    class_label = get_xml_label(XMLLabelConstants.Class)
    value_label = get_xml_label(XMLLabelConstants.Value)
    proj_label = get_xml_label(XMLLabelConstants.Projection)

    def parse_from_dict(self, data_dict):
        mag = data_dict[self.value_label]
        proj = data_dict[self.proj_label]
        return Spin(mag, proj)

    def convert_to_dict(self, qn_type, qn_value):
        return {self.type_label: qn_type.name,
                self.class_label: QuantumNumberClasses.Spin.name,
                self.value_label: str(qn_value.magnitude()),
                self.proj_label: str(qn_value.projection())}


QNClassConverterMapping = {
    QuantumNumberClasses.Int: IntQNConverter(),
    QuantumNumberClasses.Spin: SpinQNConverter()
}


'''def get_attributes_for_qn(qn_name):
    qn_attr = []
    if qn_name in QNNameClassMapping:
        qn_class = QNNameClassMapping[qn_name]
        if qn_class in QNClassAttributes:
            qn_attr = QNClassAttributes[qn_class]
    return qn_attr'''


particle_list = []


def initialize_graph(graph, initial_state, final_state):
    is_edges = get_initial_state_edges(graph)
    if len(initial_state) != len(is_edges):
        raise ValueError("The graph initial state and the supplied initial"
                         "state are of different size! (" +
                         str(len(is_edges)) + " != " +
                         str(len(initial_state)) + ")")
    fs_edges = get_final_state_edges(graph)
    if len(final_state) != len(fs_edges):
        raise ValueError("The graph final state and the supplied final"
                         "state are of different size! (" +
                         str(len(fs_edges)) + " != " +
                         str(len(final_state)) + ")")

    new_graphs = initialize_edges(
        graph, is_edges + fs_edges, initial_state + final_state)

    return new_graphs


def initialize_edges(graph, edges, particles):
    # TODO: combinatorics here!
    for edge in zip(edges, [x[0] for x in particles]):
        # lookup the particle in the list
        name_label = get_xml_label(XMLLabelConstants.Name)
        found_particles = [
            p for p in particle_list if (p[name_label] == edge[1])]
        # and attach quantum numbers
        if found_particles:
            if len(found_particles) > 1:
                raise ValueError(
                    "more than one particle with name "
                    + str(edge[1]) + " found!")
            graph.edge_props[edge[0]] = found_particles[0]

    # now add more quantum numbers given by user (spin_projection)
    new_graphs = [graph]
    for edge in zip(edges, [x[1] for x in particles]):
        temp_graphs = new_graphs
        new_graphs = []
        for g in temp_graphs:
            new_graphs.extend(
                populate_edge_with_spin_projections(g, edge[0], edge[1]))

    return new_graphs


def populate_edge_with_spin_projections(graph, edge_id, spin_projections):
    qns_label = get_xml_label(XMLLabelConstants.QuantumNumber)
    type_label = get_xml_label(XMLLabelConstants.Type)
    class_label = get_xml_label(XMLLabelConstants.Class)
    type_value = ParticleQuantumNumberNames.Spin
    class_value = QNNameClassMapping[type_value]

    new_graphs = []

    qn_list = graph.edge_props[edge_id][qns_label]
    index_list = [qn_list.index(x) for x in qn_list
                  if (type_label in x and class_label in x) and
                  (x[type_label] == type_value.name and
                   x[class_label] == class_value.name)]
    if index_list:
        for spin_proj in spin_projections:
            graph_copy = deepcopy(graph)
            graph_copy.edge_props[edge_id][qns_label][index_list[0]][get_xml_label(
                XMLLabelConstants.Projection)] = spin_proj
            new_graphs.append(graph_copy)

    return new_graphs


def load_particle_list_from_xml(file_path):
    with open(file_path, "rb") as xmlfile:
        full_dict = xmltodict.parse(xmlfile)
        for p in full_dict['ParticleList']['Particle']:
            particle_list.append(p)
    """tree = ET.parse(file_path)
    root = tree.getroot()
    # loop over particles
    for particle_xml in root:
        extract_particle(particle_xml)"""


"""
def extract_particle(particle_xml):
    particle = {}
    QN_LIST_LABEL = XMLLabelConstants.QN_LIST_LABEL
    QN_CLASS_LABEL = XMLLabelConstants.QN_CLASS_LABEL
    TYPE_LABEL = XMLLabelConstants.VAR_TYPE_LABEL
    VALUE_LABEL = XMLLabelConstants.VAR_VALUE_LABEL
    QN_PROJECTION_LABEL = XMLLabelConstants.QN_PROJECTION_LABEL

    particle['Name'] = particle_xml.attrib['Name']
    for part_prop in particle_xml:
        if "Parameter" in part_prop.tag:
            if 'Parameters' not in particle:
                particle['Parameters'] = []
            particle['Parameters'].append(
                {TYPE_LABEL: part_prop.attrib[TYPE_LABEL], VALUE_LABEL: part_prop.find(VALUE_LABEL).text})
        elif QN_LIST_LABEL in part_prop.tag:
            if QN_LIST_LABEL not in particle:
                particle[QN_LIST_LABEL] = []
            particle[QN_LIST_LABEL].append(
                {TYPE_LABEL: part_prop.attrib[TYPE_LABEL], VALUE_LABEL: part_prop.attrib[VALUE_LABEL]})
        elif "DecayInfo" in part_prop.tag:
            continue
        else:
            particle[part_prop.tag] = part_prop.text

    particle_list.append(particle)
"""