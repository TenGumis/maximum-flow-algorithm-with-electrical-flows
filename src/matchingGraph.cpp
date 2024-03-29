#include "matchingGraph.h"

#include "flow.h"
#include "graph.h"

#include <cassert>
#include <cmath>
#include <queue>
#include <utility>

MatchingGraph MatchingGraph::toMatchingGraph(const Graph& directedGraph, Flow& flow, unsigned long flowValue)
{
  MatchingGraph matchingGraph;
  int idCounter = 0;
  for (const auto& edge : directedGraph.edges)
  {
    auto matchingNodeP = std::make_shared<MatchingNode>(idCounter++, edge->capacity);
    auto matchingNodeQ = std::make_shared<MatchingNode>(idCounter++, edge->capacity);

    matchingGraph.nodesP.push_back(matchingNodeP);
    matchingGraph.nodesQ.push_back(matchingNodeQ);

    auto newEdge = std::make_shared<MatchingEdge>(matchingNodeP.get(), matchingNodeQ.get(), flow.getFlow(edge.get()));
    edge->matchingEquivalent = newEdge.get();

    matchingGraph.edges.push_back(newEdge);
    matchingNodeP->edges.push_back(newEdge.get());
    matchingNodeQ->edges.push_back(newEdge.get());
  }
  for (int i = 0; i < directedGraph.nodes.size(); i++)
  {
    if (directedGraph.nodes[i].get() == directedGraph.s || directedGraph.nodes[i].get() == directedGraph.t)
    {
      continue;
    }

    auto matchingNodeP = std::make_shared<MatchingNode>(idCounter++);
    auto matchingNodeQ = std::make_shared<MatchingNode>(idCounter++);

    matchingGraph.nodesP.push_back(matchingNodeP);
    matchingGraph.nodesQ.push_back(matchingNodeQ);

    auto mainNewEdge = std::make_shared<MatchingEdge>(matchingNodeP.get(), matchingNodeQ.get());

    matchingGraph.edges.push_back(mainNewEdge);
    matchingNodeP->edges.push_back(mainNewEdge.get());
    matchingNodeQ->edges.push_back(mainNewEdge.get());
    for (auto outgoingEdge : directedGraph.nodes[i]->outgoingEdges)
    {
      matchingNodeQ->demand += outgoingEdge->capacity;
      mainNewEdge->flow += flow.getFlow(outgoingEdge);

      auto newEdge = std::make_shared<MatchingEdge>(outgoingEdge->matchingEquivalent->pNode,
                                                    matchingNodeQ.get(),
                                                    outgoingEdge->capacity - flow.getFlow(outgoingEdge));

      matchingGraph.edges.push_back(newEdge);
      matchingNodeQ->edges.push_back(newEdge.get());
      assert(outgoingEdge->matchingEquivalent->pNode);
      outgoingEdge->matchingEquivalent->pNode->edges.push_back(newEdge.get());
    }
    for (auto incomingEdge : directedGraph.nodes[i]->incomingEdges)
    {
      matchingNodeP->demand += incomingEdge->capacity;

      auto newEdge = std::make_shared<MatchingEdge>(matchingNodeP.get(),
                                                    incomingEdge->matchingEquivalent->qNode,
                                                    incomingEdge->capacity - flow.getFlow(incomingEdge));

      matchingGraph.edges.push_back(newEdge);
      matchingNodeP->edges.push_back(newEdge.get());
      assert(incomingEdge->matchingEquivalent->qNode);
      incomingEdge->matchingEquivalent->qNode->edges.push_back(newEdge.get());
    }
  }

  auto matchingNodePT = std::make_shared<MatchingNode>(idCounter++);
  matchingGraph.nodesP.push_back(matchingNodePT);
  for (auto incomingEdge : directedGraph.t->incomingEdges)
  {
    matchingNodePT->demand += incomingEdge->capacity;

    auto newEdge = std::make_shared<MatchingEdge>(matchingNodePT.get(),
                                                  incomingEdge->matchingEquivalent->qNode,
                                                  incomingEdge->capacity - flow.getFlow(incomingEdge));

    matchingGraph.edges.push_back(newEdge);
    matchingNodePT->edges.push_back(newEdge.get());
    incomingEdge->matchingEquivalent->qNode->edges.push_back(newEdge.get());
  }
  matchingNodePT->demand -= flowValue;

  auto matchingNodeQS = std::make_shared<MatchingNode>(idCounter++);
  matchingGraph.nodesQ.push_back(matchingNodeQS);
  for (auto outgoingEdge : directedGraph.s->outgoingEdges)
  {
    matchingNodeQS->demand += outgoingEdge->capacity;

    auto newEdge = std::make_shared<MatchingEdge>(outgoingEdge->matchingEquivalent->pNode,
                                                  matchingNodeQS.get(),
                                                  outgoingEdge->capacity - flow.getFlow(outgoingEdge));

    matchingGraph.edges.push_back(newEdge);
    matchingNodeQS->edges.push_back(newEdge.get());
    outgoingEdge->matchingEquivalent->pNode->edges.push_back(newEdge.get());
  }
  matchingNodeQS->demand -= flowValue;

  return matchingGraph;
}

void MatchingGraph::toNonPerfectMatching()
{
  for (const auto& edge : edges)
  {
    auto excess = static_cast<int>(edge->flow);

    edge->flow -= excess;
    edge->pNode->demand -= excess;
    edge->qNode->demand -= excess;
  }
  {
    auto idCounter = nodesP.size() + nodesQ.size();
    auto oldNodesSize = nodesP.size();
    for (int j = 0; j < oldNodesSize; j++)
    {
      auto node = nodesP[j];
      assert(node);
      if (node->demand <= 1)
      {
        continue;
      }

      auto excess = static_cast<int>(node->demand - 1);
      node->demand = 1;
      std::vector<MatchingNode*> newNodes;
      newNodes.push_back(node.get());
      for (int i = 0; i < excess; i++)
      {
        auto newNode = std::make_shared<MatchingNode>(idCounter++, 1);
        nodesP.push_back(newNode);
        newNodes.push_back(newNode.get());
      }

      std::vector<MatchingEdge*> edgesToRedistribute;
      for (auto edge : node->edges)
      {
        edgesToRedistribute.push_back(edge);
      }
      node->edges.clear();

      int counter = 0;
      double flow = 0;
      for (auto edge : edgesToRedistribute)
      {
        if (edge->flow + flow <= 1)
        {
          flow += edge->flow;
          edge->pNode = newNodes[counter];
          newNodes[counter]->edges.push_back(edge);
        }
        else
        {
          double edgeFlow = edge->flow;

          edge->pNode = newNodes[counter];
          edge->flow = 1.0 - flow;
          newNodes[counter]->edges.push_back(edge);

          flow = flow + edgeFlow - 1;
          counter++;
          if (counter >= newNodes.size())
          {
            break;
          }
          auto newEdge = std::make_shared<MatchingEdge>(newNodes[counter], edge->qNode, flow);

          edges.push_back(newEdge);
          newNodes[counter]->edges.push_back(newEdge.get());
          edge->qNode->edges.push_back(newEdge.get());
        }
      }
    }
  }
  {
    auto idCounter = nodesP.size() + nodesQ.size();
    auto oldNodesSize = nodesQ.size();
    for (int j = 0; j < oldNodesSize; j++)
    {
      auto node = nodesQ[j];
      assert(node);
      if (node->demand <= 1)
      {
        continue;
      }

      auto excess = static_cast<int>(node->demand - 1);
      node->demand = 1;
      std::vector<MatchingNode*> newNodes;
      newNodes.push_back(node.get());
      for (int i = 0; i < excess; i++)
      {
        auto newNode = std::make_shared<MatchingNode>(idCounter++, 1);
        nodesQ.push_back(newNode);
        newNodes.push_back(newNode.get());
      }

      std::vector<MatchingEdge*> edgesToRedistribute;
      for (auto edge : node->edges)
      {
        edgesToRedistribute.push_back(edge);
      }
      node->edges.clear();

      int counter = 0;
      double flow = 0;
      for (auto edge : edgesToRedistribute)
      {
        if (edge->flow + flow <= 1)
        {
          flow += edge->flow;
          edge->qNode = newNodes[counter];
          newNodes[counter]->edges.push_back(edge);
        }
        else
        {
          double edgeFlow = edge->flow;

          edge->qNode = newNodes[counter];
          edge->flow = 1.0 - flow;
          newNodes[counter]->edges.push_back(edge);

          flow = flow + edgeFlow - 1;
          counter++;
          if (counter >= newNodes.size())
          {
            break;
          }

          auto newEdge = std::make_shared<MatchingEdge>(edge->pNode, newNodes[counter], flow);

          edges.push_back(newEdge);
          newNodes[counter]->edges.push_back(newEdge.get());
          edge->pNode->edges.push_back(newEdge.get());
        }
      }
    }
  }
}

template <class T>
typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type almost_equal(T x, T y, int ulp)
{
  // the machine epsilon has to be scaled to the magnitude of the values used
  // and multiplied by the desired precision in ULPs (units in the last place)
  return std::fabs(x - y) <= std::numeric_limits<T>::epsilon() * std::fabs(x + y) * ulp
         // unless the result is subnormal
         || std::fabs(x - y) < std::numeric_limits<T>::min();
}

void MatchingGraph::toPerfectMatching()
{
  double totalMatching = 0;
  for (const auto& edge : edges)
  {
    totalMatching += edge->flow;
  }
  double dp = static_cast<double>(nodesP.size()) - totalMatching;
  double dq = static_cast<double>(nodesQ.size()) - totalMatching;

  unsigned int nodeIdCounter = nodesP.size() + nodesQ.size();
  {
    auto newNodesNumberP = static_cast<unsigned int>(dq + 1.0);
    std::priority_queue<std::pair<double, MatchingNode*>> deficitNodes;
    for (const auto& node : nodesQ)
    {
      auto deficit = node->getDeficit();
      if (!almost_equal(deficit, 0.0, 10))
      {
        deficitNodes.push({deficit, node.get()});
      }
    }

    std::vector<std::pair<double, MatchingNode*>> newNodes;
    for (auto i = 0u; i < newNodesNumberP; i++)
    {
      auto newNode = std::make_shared<MatchingNode>(nodeIdCounter++, 1);
      nodesP.push_back(newNode);
      newNodes.emplace_back(1.0, newNode.get());
    }

    auto currentNewNode = 0u;
    while (!deficitNodes.empty() && currentNewNode < newNodes.size())
    {
      auto currentNodePair = deficitNodes.top();
      deficitNodes.pop();
      if (newNodes[currentNewNode].first < currentNodePair.first)
      {
        double newEdgeFlow = newNodes[currentNewNode].first;
        newNodes[currentNewNode].first = 0;
        currentNodePair.first -= newEdgeFlow;

        auto newEdge =
                std::make_shared<MatchingEdge>(newNodes[currentNewNode].second, currentNodePair.second, newEdgeFlow);

        edges.push_back(newEdge);
        currentNodePair.second->edges.push_back(newEdge.get());
        newNodes[currentNewNode].second->edges.push_back(newEdge.get());

        currentNewNode++;
        if (currentNewNode >= newNodes.size())
        {
          break;
        }
      }
      newNodes[currentNewNode].first -= currentNodePair.first;

      auto newEdge = std::make_shared<MatchingEdge>(
              newNodes[currentNewNode].second, currentNodePair.second, currentNodePair.first);

      edges.push_back(newEdge);
      currentNodePair.second->edges.push_back(newEdge.get());
      newNodes[currentNewNode].second->edges.push_back(newEdge.get());
      if (newNodes[currentNewNode].first == 0.0)
      {
        currentNewNode++;
      }
    }
  }
  {
    auto newNodesNumberQ = static_cast<unsigned int>(dp + 1.0);
    std::priority_queue<std::pair<double, MatchingNode*>> deficitNodes;
    for (const auto& node : nodesP)
    {
      auto deficit = node->getDeficit();
      if (!almost_equal(deficit, 0.0, 10))
      {
        deficitNodes.push({deficit, node.get()});
      }
    }

    std::vector<std::pair<double, MatchingNode*>> newNodes;
    for (auto i = 0u; i < newNodesNumberQ; i++)
    {
      auto newNode = std::make_shared<MatchingNode>(nodeIdCounter++, 1);
      nodesQ.push_back(newNode);
      newNodes.emplace_back(1.0, newNode.get());
    }

    auto currentNewNode = 0u;
    while (!deficitNodes.empty() && currentNewNode < newNodes.size())
    {
      auto currentNodePair = deficitNodes.top();
      deficitNodes.pop();
      if (newNodes[currentNewNode].first < currentNodePair.first)
      {
        double newEdgeFlow = newNodes[currentNewNode].first;
        newNodes[currentNewNode].first = 0;
        currentNodePair.first -= newEdgeFlow;

        auto newEdge =
                std::make_shared<MatchingEdge>(currentNodePair.second, newNodes[currentNewNode].second, newEdgeFlow);

        edges.push_back(newEdge);
        currentNodePair.second->edges.push_back(newEdge.get());
        newNodes[currentNewNode].second->edges.push_back(newEdge.get());

        currentNewNode++;
        if (currentNewNode >= newNodes.size())
        {
          break;
        }
      }
      newNodes[currentNewNode].first -= currentNodePair.first;

      auto newEdge = std::make_shared<MatchingEdge>(
              currentNodePair.second, newNodes[currentNewNode].second, currentNodePair.first);

      edges.push_back(newEdge);
      currentNodePair.second->edges.push_back(newEdge.get());
      newNodes[currentNewNode].second->edges.push_back(newEdge.get());
      if (newNodes[currentNewNode].first == 0.0)
      {
        currentNewNode++;
      }
    }
  }
}