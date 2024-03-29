#ifndef _FLOW_H_
#define _FLOW_H_

#include "correctionFlow.h"
#include "undirectedGraph.h"

#include <vector>

class CorrectionFlow;

class Flow
{
 private:
  std::vector<double> flow;

 public:
  explicit Flow(unsigned int size);

  double getFlow(const Edge* edge) const;
  double getFlow(const UndirectedEdge* edge, const UndirectedNode* endpoint) const;
  void updateFlow(const UndirectedEdge* edge, const UndirectedNode* endpoint, double value);
  void setFlow(const UndirectedEdge* edge, const UndirectedNode* endpoint, double value);
  void setFlow(const Edge* edge, double value);

  void update(const UndirectedGraph& undirectedGraph,
              double stepSize,
              const std::vector<double>& potentials,
              const std::vector<double>& resistances);
  void applyCorrectionFlow(const UndirectedGraph& undirectedGraph, const CorrectionFlow& correctionFlow);
  void scaleDown();
};

#endif  // _FLOW_H_