
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

namespace utils {

#define MAKE_CONVERSION_FROM_STD_VEC_TO_IGRAPH(IGRAPH_VEC)                     \
  template <typename IN1>                                                      \
  static inline void StdVectorToIgraphVectorT(IN1 &vectR, IGRAPH_VEC##_t *v) { \
    size_t n = vectR.size();                                                   \
                                                                               \
    /* Make sure that there is enough space for the items in v */              \
    IGRAPH_VEC##_resize(v, n);                                                 \
                                                                               \
    /* Copy all the items */                                                   \
    for (size_t i = 0; i < n; ++i) {                                           \
      IGRAPH_VEC##_set(v, i, vectR[i]);                                        \
    }                                                                          \
  }

MAKE_CONVERSION_FROM_STD_VEC_TO_IGRAPH(igraph_vector);
MAKE_CONVERSION_FROM_STD_VEC_TO_IGRAPH(igraph_vector_int);

static inline void StdVectorToIgraphVectorT(std::vector<std::string> &vectR,
                                            igraph_strvector_t *v) {
  size_t n = vectR.size();
  igraph_strvector_resize(v, n);
  for (size_t i = 0; i < n; ++i) {
    igraph_strvector_set(v, i, vectR[i].c_str());
  }
}

// MAKE_CONVERSION_FROM_STD_VEC_TO_IGRAPH(igraph_strvector);

#define MAKE_CONVERSION_FROM_IGRAPH_VEC_TO_STD(IGRAPH_VEC)                     \
  template <typename IN>                                                       \
  static inline void igraphVectorTToStdVector(IGRAPH_VEC##_t *v,               \
                                              std::vector<IN> &vectL) {        \
    long n = IGRAPH_VEC##_size(v);                                             \
                                                                               \
    /* Make sure that there is enough space for the items in v */              \
    vectL.clear();                                                             \
    vectL.reserve(n);                                                          \
                                                                               \
    /* Copy all the items */                                                   \
    for (size_t i = 0; i < n; ++i) {                                           \
      vectL.push_back(IGRAPH_VEC##_get(v, i));                                 \
    }                                                                          \
  }

MAKE_CONVERSION_FROM_IGRAPH_VEC_TO_STD(igraph_vector);
MAKE_CONVERSION_FROM_IGRAPH_VEC_TO_STD(igraph_vector_int);
MAKE_CONVERSION_FROM_IGRAPH_VEC_TO_STD(igraph_strvector);

} // namespace utils

namespace cereal {
////////////////////////////////////////////////////////////////
// serialization of igraph objects
template <class Archive>
inline void CEREAL_SAVE_FUNCTION_NAME(Archive &ar, igraph_t const &graph) {
  size_t numVertices = igraph_vcount(&graph);
  size_t numEdges = igraph_ecount(&graph);

  igraph_vector_int_t allEdges;
  igraph_vector_int_init(&allEdges, numEdges);
  if (igraph_edges(&graph, igraph_ess_all(IGRAPH_EDGEORDER_ID), &allEdges)) {
    throw std::runtime_error("Failed to get all edges");
  }
  std::vector<long int> edges;
  utils::igraphVectorTToStdVector(&allEdges, edges);
  igraph_vector_int_destroy(&allEdges);

  ar(numVertices);
  ar(numEdges);
  ar(edges);

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
  size_t numVertexAttributes = igraph_strvector_size(&vnames);
  ar(make_size_tag(numVertexAttributes));
  for (size_t i = 0; i < numVertexAttributes; i++) {
    const char *name = igraph_strvector_get(&vnames, i);
    ar(std::string(name));
    ar(igraph_vector_int_get(&vtypes, i));
    switch (igraph_vector_int_get(&vtypes, i)) {
    // case IGRAPH_ATTRIBUTE_DEFAULT:
    case IGRAPH_ATTRIBUTE_NUMERIC: {
      igraph_vector_t results;
      igraph_vector_init(&results, numVertices);
      igraph_cattribute_VANV(&graph, igraph_strvector_get(&vnames, i),
                             igraph_vss_all(), &results);
      std::vector<double> attributes;
      utils::igraphVectorTToStdVector(&results, attributes);
      ar(attributes);
      igraph_vector_destroy(&results);
    } break;
    case IGRAPH_ATTRIBUTE_STRING: {
      igraph_strvector_t strresults;
      igraph_strvector_init(&strresults, numVertices);
      igraph_cattribute_VASV(&graph, igraph_strvector_get(&vnames, i),
                             igraph_vss_all(), &strresults);
      std::vector<std::string> strattributes;
      utils::igraphVectorTToStdVector(&strresults, strattributes);
      ar(strattributes);
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
  size_t numEdgeAttributes = igraph_strvector_size(&enames);
  ar(make_size_tag(numEdgeAttributes));
  for (size_t i = 0; i < numEdgeAttributes; i++) {
    const char *name = igraph_strvector_get(&enames, i);
    ar(std::string(name));
    ar(igraph_vector_int_get(&etypes, i));
    switch (igraph_vector_int_get(&etypes, i)) {
    // case IGRAPH_ATTRIBUTE_DEFAULT:
    case IGRAPH_ATTRIBUTE_NUMERIC: {
      igraph_vector_t results;
      igraph_vector_init(&results, numEdges);
      igraph_cattribute_EANV(&graph, igraph_strvector_get(&enames, i),
                             igraph_ess_all(IGRAPH_EDGEORDER_ID), &results);
      std::vector<double> attributes;
      utils::igraphVectorTToStdVector(&results, attributes);
      ar(attributes);
      igraph_vector_destroy(&results);
    } break;
    case IGRAPH_ATTRIBUTE_STRING: {
      igraph_strvector_t strresults;
      igraph_strvector_init(&strresults, numEdges);
      igraph_cattribute_EASV(&graph, igraph_strvector_get(&enames, i),
                             igraph_ess_all(IGRAPH_EDGEORDER_ID), &strresults);
      std::vector<std::string> strattributes;
      utils::igraphVectorTToStdVector(&strresults, strattributes);
      ar(strattributes);
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
  size_t numEdges;
  ar(numVertices);
  ar(numEdges);
  std::vector<long int> edges;
  ar(edges);
  igraph_vector_int_t allEdges;
  igraph_vector_int_init(&allEdges, numEdges);
  utils::StdVectorToIgraphVectorT(edges, &allEdges);

  igraph_add_vertices(&graph, numVertices, nullptr);
  igraph_add_edges(&graph, &allEdges, nullptr);

  // deserialize vertex attributes
  size_type numVertexAttributes;
  ar(make_size_tag(numVertexAttributes));

  for (size_t i = 0; i < numVertexAttributes; ++i) {
    std::string attributeName;
    ar(attributeName);
    int attributeType;
    ar(attributeType);
    switch (attributeType) {
    // case IGRAPH_ATTRIBUTE_DEFAULT:
    case IGRAPH_ATTRIBUTE_NUMERIC: {
      std::vector<double> attributes;
      ar(attributes);
      igraph_vector_t results;
      igraph_vector_init(&results, attributes.size());
      utils::StdVectorToIgraphVectorT(attributes, &results);
      igraph_cattribute_VAN_setv(&graph, attributeName.c_str(), &results);
      igraph_vector_destroy(&results);
    }; break;
    case IGRAPH_ATTRIBUTE_STRING: {
      std::vector<std::string> strattributes;
      ar(strattributes);
      igraph_strvector_t strresults;
      igraph_strvector_init(&strresults, strattributes.size());
      utils::StdVectorToIgraphVectorT(strattributes, &strresults);
      igraph_cattribute_VAS_setv(&graph, attributeName.c_str(), &strresults);
      igraph_strvector_destroy(&strresults);
    }; break;
    default:
      throw std::runtime_error("This attribute type (" +
                               std::to_string(attributeType) +
                               ") is not supported");
    }
  }

  // and same for edge attributes
  size_type numEdgeAttributes;
  ar(make_size_tag(numEdgeAttributes));
  for (size_t i = 0; i < numEdgeAttributes; ++i) {
    std::string attributeName;
    ar(attributeName);
    int attributeType;
    ar(attributeType);
    switch (attributeType) {
    // case IGRAPH_ATTRIBUTE_DEFAULT:
    case IGRAPH_ATTRIBUTE_NUMERIC: {
      std::vector<double> attributes;
      ar(attributes);
      igraph_vector_t results;
      igraph_vector_init(&results, 1);
      utils::StdVectorToIgraphVectorT(attributes, &results);
      igraph_cattribute_EAN_setv(&graph, attributeName.c_str(), &results);
      igraph_vector_destroy(&results);
    } break;
    case IGRAPH_ATTRIBUTE_STRING: {
      std::vector<std::string> strattributes;
      ar(strattributes);
      igraph_strvector_t strresults;
      igraph_strvector_init(&strresults, 1);
      utils::StdVectorToIgraphVectorT(strattributes, &strresults);
      igraph_cattribute_EAS_setv(&graph, attributeName.c_str(), &strresults);
      igraph_strvector_destroy(&strresults);
    } break;
    default:
      throw std::runtime_error("This attribute type (" +
                               std::to_string(attributeType) +
                               ") is not supported");
    }
  }
}
} // namespace cereal

int main(int argc, char **argv) {
  std::cout << "Running" << std::endl;
  std::string file = "serialised-igraph.bin";
  igraph_set_attribute_table(&igraph_cattribute_table);

  {
    igraph_t graph;

    igraph_empty(&graph, 0, IGRAPH_UNDIRECTED);
    igraph_rng_seed(igraph_rng_default(), 42);

    // igraph_erdos_renyi_game(&graph, IGRAPH_ERDOS_RENYI_GNM, 2500, 3000,
    //                         IGRAPH_UNDIRECTED, IGRAPH_LOOPS_TWICE);
    igraph_add_vertices(&graph, 2500, 0);

    for (int j = 1; j < 2500; ++j) {
      igraph_add_edge(&graph, j, (j % 1000) + 1);
    }

    // add vertex attributes
    igraph_cattribute_VAN_set(&graph, "id", 1, 12);
    igraph_cattribute_VAN_set(&graph, "id", 2499, 2);
    igraph_cattribute_VAN_set(&graph, "type", 1, 2.9);

    // add edge attributes
    igraph_cattribute_EAN_set(&graph, "id", 1, 3.1);

    // actually do the serialisation
    std::ofstream os(file);
    cereal::BinaryOutputArchive oarchive(os);
    oarchive(graph);
    std::cout << "Serialized graph with " << igraph_vcount(&graph)
              << " vertices and " << igraph_ecount(&graph) << " edges."
              << std::endl;

    igraph_destroy(&graph);
  }

  {
    // do the deserialisation
    std::ifstream is(file);
    cereal::BinaryInputArchive iarchive(is);

    igraph_t graph;
    igraph_empty(&graph, 0, IGRAPH_UNDIRECTED);
    iarchive(graph);

    // success?
    std::cout << "Deserialized graph with " << igraph_vcount(&graph)
              << " vertices and " << igraph_ecount(&graph) << " edges."
              << std::endl;
  }
}
