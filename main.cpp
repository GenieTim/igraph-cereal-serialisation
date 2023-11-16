
extern "C" {
#include <igraph/igraph.h>
}
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <type_traits>

#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/xml.hpp>
// #include <cereal/types/map.hpp>
// #include <cereal/types/set.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

namespace cereal {
////////////////////////////////////////////////////////////////
// serialization of igraph objects
// a numeric vector
template <class Archive>
inline void CEREAL_SAVE_FUNCTION_NAME(Archive &ar,
                                      igraph_vector_int_t const &vec) {
  size_type n = igraph_vector_int_size(&vec);
  ar(make_size_tag(n));
  for (size_type i = 0; i < n; ++i) {
    ar(igraph_vector_int_get(&vec, i));
  }
}

template <class Archive>
inline void CEREAL_LOAD_FUNCTION_NAME(Archive &ar, igraph_vector_int_t &vec) {
  size_type n;
  ar(make_size_tag(n));
  igraph_vector_int_resize(&vec, n);
  for (size_type i = 0; i < n; ++i) {
    long int val;
    ar(val);
    igraph_vector_int_set(&vec, i, val);
  }
}

template <class Archive>
inline void CEREAL_SAVE_FUNCTION_NAME(Archive &ar, igraph_vector_t const &vec) {
  size_type n = igraph_vector_size(&vec);
  ar(make_size_tag(n));
  for (size_type i = 0; i < n; ++i) {
    ar(igraph_vector_get(&vec, i));
  }
}

template <class Archive>
inline void CEREAL_LOAD_FUNCTION_NAME(Archive &ar, igraph_vector_t &vec) {
  size_type n;
  ar(make_size_tag(n));
  igraph_vector_resize(&vec, n);
  for (size_type i = 0; i < n; ++i) {
    double val;
    ar(val);
    igraph_vector_set(&vec, i, val);
  }
}

template <class Archive>
inline void CEREAL_SAVE_FUNCTION_NAME(Archive &ar,
                                      igraph_strvector_t const &vec) {
  size_type n = igraph_strvector_size(&vec);
  ar(make_size_tag(n));
  for (size_type i = 0; i < n; ++i) {
    std::string val = igraph_strvector_get(&vec, i);
    ar(val);
  }
}

template <class Archive>
inline void CEREAL_LOAD_FUNCTION_NAME(Archive &ar, igraph_strvector_t &vec) {
  size_type n;
  ar(make_size_tag(n));
  igraph_strvector_resize(&vec, n);
  std::string val;
  val.reserve(50);
  for (size_type i = 0; i < n; ++i) {
    val.clear();
    ar(val);
    igraph_strvector_set(&vec, i, val.c_str());
  }
}

// the graph
template <class Archive>
inline void CEREAL_SAVE_FUNCTION_NAME(Archive &ar, igraph_t const &graph) {
  size_t numVertices = igraph_vcount(&graph);
  ar(make_nvp("num_vertices", numVertices));
  size_t numEdges = igraph_ecount(&graph);
  ar(make_nvp("num_edges", numEdges));

  igraph_vector_int_t allEdges;
  igraph_vector_int_init(&allEdges, numEdges);
  if (igraph_edges(&graph, igraph_ess_all(IGRAPH_EDGEORDER_ID), &allEdges)) {
    throw std::runtime_error("Failed to get all edges");
  }

  ar(make_nvp("edges", allEdges));
  igraph_vector_int_destroy(&allEdges);

  // after storing the edges, must also store the attributes
  // query them first
  igraph_strvector_t gnames;
  igraph_strvector_init(&gnames, 1);
  igraph_vector_int_t gtypes;
  igraph_vector_int_init(&gtypes, 1);
  igraph_strvector_t vnames;
  igraph_strvector_init(&vnames, 1);
  igraph_vector_int_t vtypes;
  igraph_vector_int_init(&vtypes, 1);
  igraph_strvector_t enames;
  igraph_strvector_init(&enames, 1);
  igraph_vector_int_t etypes;
  igraph_vector_int_init(&etypes, 1);
  igraph_cattribute_list(&graph, &gnames, &gtypes, &vnames, &vtypes, &enames,
                         &etypes);

  if (igraph_strvector_size(&gnames) != 0) {
    throw std::runtime_error(
        "Graph attributes serialization not supported yet.");
  }

  // serizalize vertex attributes
  size_type numVertexAttributes = igraph_strvector_size(&vnames);
  assert(igraph_strvector_size(&vnames) == igraph_vector_int_size(&vtypes));
  ar(make_nvp("vertex_attr_names", vnames));
  ar(make_nvp("vertex_attr_types", vtypes));
  //   ar(make_size_tag(numVertexAttributes));
  for (size_t i = 0; i < numVertexAttributes; i++) {
    const char *name = igraph_strvector_get(&vnames, i);
    std::string namestr = std::string(name);
    switch (igraph_vector_int_get(&vtypes, i)) {
    // case IGRAPH_ATTRIBUTE_DEFAULT:
    case IGRAPH_ATTRIBUTE_NUMERIC: {
      igraph_vector_t results;
      igraph_vector_init(&results, numVertices);
      igraph_cattribute_VANV(&graph, igraph_strvector_get(&vnames, i),
                             igraph_vss_all(), &results);
      ar(make_nvp("vertex_attr_" + namestr, results));
      igraph_vector_destroy(&results);
    } break;
    case IGRAPH_ATTRIBUTE_STRING: {
      igraph_strvector_t strresults;
      igraph_strvector_init(&strresults, numVertices);
      igraph_cattribute_VASV(&graph, igraph_strvector_get(&vnames, i),
                             igraph_vss_all(), &strresults);
      ar(make_nvp("vertex_attr_" + namestr, strresults));
      igraph_strvector_destroy(&strresults);
    } break;
    default:
      throw std::runtime_error(
          "This attribute type (" +
          std::to_string(igraph_vector_int_get(&vtypes, i)) +
          ") is not supported");
    }
  }

  // serizalize edge attributes
  size_type numEdgeAttributes = igraph_strvector_size(&enames);
  assert(igraph_strvector_size(&enames) == igraph_vector_int_size(&etypes));
  ar(make_nvp("edge_attr_names", enames));
  ar(make_nvp("edge_attr_types", etypes));
  //   ar(make_size_tag(numEdgeAttributes * 3));
  for (size_t i = 0; i < numEdgeAttributes; i++) {
    const char *name = igraph_strvector_get(&enames, i);
    std::string namestr = std::string(name);
    switch (igraph_vector_int_get(&etypes, i)) {
    // case IGRAPH_ATTRIBUTE_DEFAULT:
    case IGRAPH_ATTRIBUTE_NUMERIC: {
      igraph_vector_t results;
      igraph_vector_init(&results, numEdges);
      igraph_cattribute_EANV(&graph, igraph_strvector_get(&enames, i),
                             igraph_ess_all(IGRAPH_EDGEORDER_ID), &results);
      ar(make_nvp("edge_attr_" + namestr, results));
      igraph_vector_destroy(&results);
    } break;
    case IGRAPH_ATTRIBUTE_STRING: {
      igraph_strvector_t strresults;
      igraph_strvector_init(&strresults, numEdges);
      igraph_cattribute_EASV(&graph, igraph_strvector_get(&enames, i),
                             igraph_ess_all(IGRAPH_EDGEORDER_ID), &strresults);
      ar(make_nvp("edge_attr_" + namestr, strresults));
      igraph_strvector_destroy(&strresults);
    } break;
    default:
      throw std::runtime_error(
          "This attribute type (" +
          std::to_string(igraph_vector_int_get(&etypes, i)) +
          ") is not supported");
    }
  }

  igraph_strvector_destroy(&gnames);
  igraph_strvector_destroy(&enames);
  igraph_strvector_destroy(&vnames);

  igraph_vector_int_destroy(&gtypes);
  igraph_vector_int_destroy(&etypes);
  igraph_vector_int_destroy(&vtypes);
}

template <class Archive>
inline void CEREAL_LOAD_FUNCTION_NAME(Archive &ar, igraph_t &graph) {
  size_t numVertices;
  ar(make_nvp("num_vertices", numVertices));
  size_t numEdges;
  ar(make_nvp("num_edges", numEdges));
  igraph_vector_int_t allEdges;
  igraph_vector_int_init(&allEdges, numEdges);
  ar(make_nvp("edges", allEdges));

  igraph_add_vertices(&graph, numVertices, nullptr);
  igraph_add_edges(&graph, &allEdges, nullptr);
  igraph_vector_int_destroy(&allEdges);

  // deserialize vertex attributes
  igraph_strvector_t vnames;
  igraph_strvector_init(&vnames, 1);
  ar(make_nvp("vertex_attr_names", vnames));
  igraph_vector_int_t vtypes;
  igraph_vector_int_init(&vtypes, 1);
  ar(make_nvp("vertex_attr_types", vtypes));

  size_type numVertexAttributes = igraph_vector_int_size(&vtypes);
  for (size_t i = 0; i < numVertexAttributes; ++i) {
    std::string attributeName = std::string(igraph_strvector_get(&vnames, i));
    int attributeType = igraph_vector_int_get(&vtypes, i);
    switch (attributeType) {
    // case IGRAPH_ATTRIBUTE_DEFAULT:
    case IGRAPH_ATTRIBUTE_NUMERIC: {
      igraph_vector_t results;
      igraph_vector_init(&results, numVertices);
      ar(make_nvp("vertex_attr_" + attributeName, results));
      igraph_cattribute_VAN_setv(&graph, attributeName.c_str(), &results);
      igraph_vector_destroy(&results);
    }; break;
    case IGRAPH_ATTRIBUTE_STRING: {
      igraph_strvector_t strresults;
      igraph_strvector_init(&strresults, numVertices);
      ar(make_nvp("vertex_attr_" + attributeName, strresults));
      igraph_cattribute_VAS_setv(&graph, attributeName.c_str(), &strresults);
      igraph_strvector_destroy(&strresults);
    }; break;
    default:
      throw std::runtime_error("This attribute type (" +
                               std::to_string(attributeType) +
                               ") is not supported");
    }
  }
  igraph_vector_int_destroy(&vtypes);
  igraph_strvector_destroy(&vnames);

  // and same for edge attributes
  igraph_strvector_t enames;
  igraph_strvector_init(&enames, 1);
  ar(make_nvp("edge_attr_names", enames));
  igraph_vector_int_t etypes;
  igraph_vector_int_init(&etypes, 1);
  ar(make_nvp("edge_attr_types", etypes));

  size_t numEdgeAttributes = igraph_vector_int_size(&etypes);
  for (size_t i = 0; i < numEdgeAttributes; ++i) {
    std::string attributeName = std::string(igraph_strvector_get(&enames, i));
    int attributeType = igraph_vector_int_get(&etypes, i);
    switch (attributeType) {
    // case IGRAPH_ATTRIBUTE_DEFAULT:
    case IGRAPH_ATTRIBUTE_NUMERIC: {
      igraph_vector_t results;
      igraph_vector_init(&results, 1);
      ar(make_nvp("edge_attr_" + attributeName, results));
      igraph_cattribute_EAN_setv(&graph, attributeName.c_str(), &results);
      igraph_vector_destroy(&results);
    } break;
    case IGRAPH_ATTRIBUTE_STRING: {
      igraph_strvector_t strresults;
      igraph_strvector_init(&strresults, 1);
      ar(make_nvp("edge_attr_" + attributeName, strresults));
      igraph_cattribute_EAS_setv(&graph, attributeName.c_str(), &strresults);
      igraph_strvector_destroy(&strresults);
    } break;
    default:
      throw std::runtime_error("This attribute type (" +
                               std::to_string(attributeType) +
                               ") is not supported");
    }
  }
  igraph_vector_int_destroy(&etypes);
  igraph_strvector_destroy(&enames);
}
} // namespace cereal

int main(int argc, char **argv) {
  std::cout << "Running" << std::endl;
  std::string file = "serialised-igraph.json";
  igraph_set_attribute_table(&igraph_cattribute_table);

  {
    igraph_t graph;

    igraph_empty(&graph, 0, IGRAPH_UNDIRECTED);
    igraph_rng_seed(igraph_rng_default(), 42);

    igraph_add_vertices(&graph, 25, 0);

    for (int j = 1; j < 25; ++j) {
      igraph_add_edge(&graph, j, (j % 10) + 1);
    }

    // add vertex attributes
    igraph_cattribute_VAN_set(&graph, "id", 1, 12);
    igraph_cattribute_VAN_set(&graph, "id", 24, 2);
    igraph_cattribute_VAN_set(&graph, "type", 1, 2.9);

    // add edge attributes
    igraph_cattribute_EAN_set(&graph, "id", 1, 3.1);

    // actually do the serialisation
    std::ofstream os(file);
    cereal::JSONOutputArchive oarchive(os);
    oarchive(graph);
    std::cout << "Serialized graph with " << igraph_vcount(&graph)
              << " vertices and " << igraph_ecount(&graph) << " edges."
              << std::endl;

    igraph_destroy(&graph);
  }

  {
    // do the deserialisation
    std::ifstream is(file);
    cereal::JSONInputArchive iarchive(is);

    igraph_t graph;
    igraph_empty(&graph, 0, IGRAPH_UNDIRECTED);
    iarchive(graph);

    // success?
    std::cout << "Deserialized graph with " << igraph_vcount(&graph)
              << " vertices and " << igraph_ecount(&graph) << " edges."
              << std::endl;
    igraph_destroy(&graph);
  }
}
