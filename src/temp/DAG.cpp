#include <iostream>
#include <limits>
#include <float.h>
#include <map>
#include <queue>
#include <vector>
#include "Snap.h"
#include "DAG.h"

using namespace std;

TPt<TNodeEDatNet<TFlt, TFlt>> GenerateRandomBayesianNetwork(int minNumNodesPerRank, int maxNumNodesPerRank, int minRanks, int maxRanks, int probabilityEdge)
{
	srand (time (NULL));
	auto pGraph = TNodeEDatNet<TFlt, TFlt>::New();
	int nodes = 0;
	int ranks = minRanks + (rand () % (maxRanks - minRanks + 1));

	for (int i = 0; i < ranks; i++)
	{
		/* New nodes of 'higher' rank than all nodes generated till now.  */
		int new_nodes = minNumNodesPerRank + (rand () % (maxNumNodesPerRank - minNumNodesPerRank + 1));

		/* Edges from old nodes ('nodes') to new ones ('new_nodes').  */
		for (int j = 0; j < nodes; ++j)
			for (int k = 0; k < new_nodes; ++k)
				if ( (rand () % 100) < probabilityEdge)
				{
					if(!pGraph->IsNode(j))
					{
						pGraph->AddNode(j);
						pGraph->SetNDat(j, (double)rand() / RAND_MAX);
					}
					if(!pGraph->IsNode(k+nodes))
					{
						pGraph->AddNode(k+nodes);
						pGraph->SetNDat(k+nodes, (double)rand() / RAND_MAX);
					}
					pGraph->AddEdge(j,k+nodes);
					pGraph->SetEDat(j,k+nodes, (double)rand() / RAND_MAX);
				}

		nodes += new_nodes; /* Accumulate into old node set.  */
	}
	return pGraph;
}

TPt<TNodeEDatNet<TFlt, TFlt>> GenerateDAG1(TPt<TNodeEDatNet<TFlt, TFlt>> pGraph, double& threshold, std::vector<int>& seedNodes){

	struct Node{
		int nid;
		double rank;
		bool operator<(const Node& n) const { return rank > n.rank;}
	};

	auto pGraph_DAG1 = TNodeEDatNet<TFlt, TFlt>::New();
	std::map<int, int> parentNodes;
	std::map<int,bool> visitedNodes;
	std::priority_queue<Node> pq;

	//Initialize the edge weight
	for (auto EI = pGraph->BegEI(); EI < pGraph->EndEI(); EI++)
	{
		pGraph->SetEDat(EI.GetSrcNId(),EI.GetDstNId(),(double)rand() / RAND_MAX);
	}

	//Initialize the original graph
	for (auto NI = pGraph->BegNI(); NI < pGraph->EndNI(); NI++)
	{
		int NID = NI.GetId();
		pGraph->SetNDat(NID,DBL_MAX);
		visitedNodes.insert(std::make_pair(NID,false));
	}

	//Add root node (ID: INT_MAX) as seed nodes' parent
	pGraph->AddNode(INT_MAX);
	pGraph->SetNDat(INT_MAX,1.0);
	pq.push((Node){INT_MAX,1.0});
	for (int srcNode: seedNodes)
	{
		parentNodes[srcNode]=INT_MAX;
		pGraph->AddEdge(INT_MAX,srcNode);
		pGraph->SetEDat(INT_MAX,srcNode,1.0);
	}



	// Beginning of Dijkstra tree
	for (int v =0 ; v<pGraph->GetNodes(); ++v)
	{
		int nodeToVisit = -1;
		while (!pq.empty() && visitedNodes[nodeToVisit = pq.top().nid])
			pq.pop();

		if (nodeToVisit == -1) break;
		visitedNodes[nodeToVisit]=true;
		//cout<<nodeToVisit<<endl;

		auto srcNI =pGraph->GetNI(nodeToVisit);
		int numChildren =srcNI.GetOutDeg();
		for (int i=0;i<numChildren;++i)
		{
			int dstNID=srcNI.GetOutNId(i);
			double rank = srcNI.GetDat().Val - log(srcNI.GetOutEDat(i).Val);
			//cout<<"src's rank "<< srcNI.GetDat().Val<< " edge's rank: "<<- log(srcNI.GetOutEDat(i).Val)<<" rank: "<<rank<<endl;
			if (rank >= threshold)
			{
				if( rank!=0.0 &&  visitedNodes.find(dstNID)->second==false && rank < pGraph->GetNDat(dstNID))
				{
					parentNodes[dstNID]=nodeToVisit;
					pq.push((Node){dstNID,rank});
					pGraph->SetNDat(dstNID,rank);
				}
			}
		}
	} 	// End of Dijkstra tree

	//Remove the root node and its edges

	pGraph->DelNode(INT_MAX);
	for (int srcNode: seedNodes) parentNodes.erase(srcNode);
	// Add edges to the DAG1 graph from parentNodes
	for (auto it = parentNodes.begin();it!=parentNodes.end();++it)
	{
		if(!pGraph_DAG1->IsNode(it->second)) pGraph_DAG1->AddNode(it->second);
		if(!pGraph_DAG1->IsNode(it->first)) pGraph_DAG1->AddNode(it->first);
		pGraph_DAG1->AddEdge(it->second,it->first);
		pGraph_DAG1->SetEDat(it->second,it->first,pGraph->GetEDat(it->second,it->first));

	}
	// Add back other edges with the condition r(u)<r(v)
	for (auto EI = pGraph->BegEI(); EI < pGraph->EndEI(); EI++)
	{
			int u = EI.GetSrcNId(), v = EI.GetDstNId();
			if(pGraph->GetNDat(u)< pGraph->GetNDat(v))
			{
				if (!pGraph_DAG1->IsEdge(u,v))
				{
					pGraph_DAG1->AddEdge(u,v);
					pGraph_DAG1->SetEDat(u,v,EI.GetDat());
				}
			}
	}

//cout<<parentNodes.size()<<endl;
	return pGraph_DAG1;
}

