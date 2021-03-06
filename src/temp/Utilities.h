#pragma once

#include <vector>
#include <map>
#include <tbb/concurrent_unordered_map.h>
template <class T> class TPt;
template <class T, class U> class TNodeEDatNet;
class TFlt;


#ifdef __linux__
void SystemPause();
#endif

int GetPhysicalProcessorCount();


//! @param minNumNodesPerRank How 'fat' the DAG should be.
//! @param minRanks How 'tall' the DAG should be.
//! @param probabilityEdge Chance of having an Edge in percent.
TPt<TNodeEDatNet<TFlt, TFlt>> GenerateRandomBayesianNetwork(unsigned int minNumNodesPerRank, unsigned int maxNumNodesPerRank, unsigned int minRanks, unsigned int maxRanks, unsigned int probabilityEdge);

void RandomGraphInitialization(TPt<TNodeEDatNet<TFlt, TFlt>> &pGraph);
void InitializationBeforePropagation(TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph);

//! Input : Directed graph with initialized weight edges.
//! Edges with a propagation probability strictly greater than dThreshold are ignored.
//! @note Initial nodes values are initialized to FLT_MAX.
TPt<TNodeEDatNet<TFlt, TFlt>> MIOA(const TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, int sourceNode, double dThreshold=0.0);

TPt<TNodeEDatNet<TFlt, TFlt>> GenerateDAG1(const TPt<TNodeEDatNet<TFlt, TFlt>> &pGraph, const std::vector<int>& seedNodes, double threshold = 0.0);

TPt<TNodeEDatNet<TFlt, TFlt>> GenerateDAG1(const TPt<TNodeEDatNet<TFlt, TFlt>> &pGraph, int sourceNode, double threshold = 0.0);

TPt<TNodeEDatNet<TFlt, TFlt>> GenerateDAG2(const TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, const std::vector<int> &vSeedIDs, double dThreshold=0.0);

TPt<TNodeEDatNet<TFlt, TFlt>> GenerateDAG2(const TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, int sourceNode, double dThreshold=0.0);

std::vector<int> MaxIncrementalInfluence(TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, int numRounds);

