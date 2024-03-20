#include "Dependencies.h"

#include <iostream>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>

DependencyManager::DependencyManager(unsigned nodeCount) : _nodeCount(nodeCount) {
}

void DependencyManager::addDependency(unsigned dependency, unsigned dependant) {
    _edges.push_back(std::make_pair(dependency, dependant));
}

extern char *__ah_arg_fuzz_ptr;
extern unsigned int ahArgOffset;
void DependencyManager::visitInOrder(std::function<void(unsigned)> action) {
    typedef boost::adjacency_list< boost::vecS, boost::vecS, boost::directedS > Graph;
    Graph g(_nodeCount);
    for (auto it : _edges) {
        boost::add_edge(it.first, it.second, g);
    }
    std::vector<uint64_t> vertices;
    try {
        boost::topological_sort(g, std::back_inserter(vertices));
    } catch (...) {
        std::cout << (char*)&__ah_arg_fuzz_ptr[ahArgOffset-256-4] << "\n";
    }
    for (auto it = vertices.rbegin(); it != vertices.rend(); it++) {
        action(*it);
    }
}

