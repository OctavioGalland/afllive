#ifndef _DEPENDENCIES_H_
#define _DEPENDENCIES_H_

#include <functional>
#include <set>
#include <vector>

class DependencyManager {
public:
    DependencyManager(unsigned nodeCount);
    void addDependency(unsigned dependency, unsigned dependant);
    void visitInOrder(std::function<void(unsigned )> action);
private:
    std::vector<std::pair<unsigned , unsigned> > _edges;
    unsigned _nodeCount;
};

#endif // _DEPENDENCIES_H_
