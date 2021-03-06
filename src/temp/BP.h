#pragma once

#include <vector>

template <class T> class TPt;
template <class T, class U> class TNodeEDatNet;
class TFlt;

#ifdef _DEBUG
void PropagateFromNode(TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, int sourceNodeID, bool bDisplayInfo = false);
#else
void PropagateFromNode(TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, int sourceNodeID);
#endif


void ParallelBPFromNode(TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, int sourceNodeID);
void ParallelBPFromNode_1DPartitioning(TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, int sourceNodeID);

#ifdef _DEBUG
void PropagateFromNode(TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, const std::vector<int> &vSeedIDs, bool bDisplayInfo = false);
#else
void PropagateFromNode(TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, const std::vector<int> &vSeedIDs);
#endif


void ParallelBPFromNode(TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, const std::vector<int> &vSeedIDs);
void ParallelBPFromNode_1DPartitioning(TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, const std::vector<int> &vSeedIDs);



//Calculate the overall influence from seed nodes
double InfluenceSpreadFromSeedNodes (TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph);
