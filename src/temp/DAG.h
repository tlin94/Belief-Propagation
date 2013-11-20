#pragma once

template <class T> class TPt;
class TFlt;

TPt<TNodeEDatNet<TFlt, TFlt>> GenerateRandomBayesianNetwork(int minNumNodesPerRank, int maxNumNodesPerRank, int minRanks, int maxRanks, int probabilityEdge);
TPt<TNodeEDatNet<TFlt, TFlt>> GenerateDAG1(TPt<TNodeEDatNet<TFlt, TFlt>> pGraph, double& threshold, std::vector<int>& seedNodes);
