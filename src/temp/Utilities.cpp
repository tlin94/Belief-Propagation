#include <iostream>
#include <map>
#include <algorithm>
#include <queue>
#include <float.h>
#include <tbb/spin_mutex.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/parallel_for.h>
#include <tbb/concurrent_vector.h>
#include <tbb/partitioner.h>
#include "Utilities.h"
#include "BP.h"
#include "omp.h"
#include <Snap.h>



using namespace std;

#ifdef WIN32

//--------------------------------------------------------------------------------------
// Helper functions for querying information about the processors in the current
// system.  ( Copied from the doc page for GetLogicalProcessorInformation() )
//--------------------------------------------------------------------------------------
typedef BOOL (WINAPI *LPFN_GLPI)(
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, 
	PDWORD);


//  Helper function to count bits in the processor mask
static DWORD CountBits(ULONG_PTR bitMask)
{
	DWORD LSHIFT = sizeof(ULONG_PTR)*8 - 1;
	DWORD bitSetCount = 0;
	DWORD bitTest = 1 << LSHIFT;
	DWORD i;

	for( i = 0; i <= LSHIFT; ++i)
	{
		bitSetCount += ((bitMask & bitTest)?1:0);
		bitTest/=2;
	}

	return bitSetCount;
}

int GetPhysicalProcessorCount()
{
	DWORD procCoreCount = 0;    // Return 0 on any failure.  That'll show them.

	LPFN_GLPI Glpi;

	Glpi = (LPFN_GLPI) GetProcAddress(
		GetModuleHandle(TEXT("kernel32")),
		"GetLogicalProcessorInformation");
	if (NULL == Glpi) 
	{
		// GetLogicalProcessorInformation is not supported
		return procCoreCount;
	}

	BOOL done = FALSE;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
	DWORD returnLength = 0;

	while (!done) 
	{
		DWORD rc = Glpi(buffer, &returnLength);

		if (FALSE == rc) 
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
			{
				if (buffer) 
					free(buffer);

				buffer=(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(
					returnLength);

				if (NULL == buffer) 
				{
					// Allocation failure\n
					return procCoreCount;
				}
			} 
			else 
			{
				// Unanticipated error
				return procCoreCount;
			}
		} 
		else done = TRUE;
	}

	DWORD byteOffset = 0;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = buffer;
	while (byteOffset < returnLength) 
	{
		if (ptr->Relationship == RelationProcessorCore) 
		{
			if(ptr->ProcessorCore.Flags)
			{
				//  Hyperthreading or SMT is enabled.
				//  Logical processors are on the same core.
				procCoreCount += 1;
			}
			else
			{
				//  Logical processors are on different cores.
				procCoreCount += CountBits(ptr->ProcessorMask);
			}
		}
		byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
		ptr++;
	}

	free (buffer);

	return procCoreCount;
}

#endif

#ifdef __linux__
#include "stdio.h"

int GetPhysicalProcessorCount()
{
	FILE * fp;
	char res[128];
	fp = popen("/bin/cat /proc/cpuinfo |grep -c '^processor'","r");
	fread(res, 1, sizeof(res)-1, fp);
	fclose(fp);
	return res[0];
}

#ifdef __linux__
void SystemPause()
{
	char magickey;
	fflush(stdin);
	printf("Appuyez sur une touche pour continuer...");
	scanf("%c", &magickey);
	magickey = getc(stdin);
}
#endif
#endif

TPt<TNodeEDatNet<TFlt, TFlt>> GenerateRandomBayesianNetwork(unsigned int minNumNodesPerRank, unsigned int maxNumNodesPerRank, unsigned int minRanks, unsigned int maxRanks, unsigned int probabilityEdge)
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
// before generating DAG
void RandomGraphInitialization(TPt<TNodeEDatNet<TFlt, TFlt>> &pGraph)
{
	srand(time(NULL));
	for (auto EI = pGraph->BegEI(); EI < pGraph->EndEI(); EI++)
		pGraph->SetEDat(EI.GetSrcNId(), EI.GetDstNId(), (double) rand() / RAND_MAX);
	for (auto NI = pGraph->BegNI(); NI < pGraph->EndNI(); NI++)
		pGraph->SetNDat(NI.GetId(), 0.0);
}

// before belief propagation
void InitializationBeforePropagation(TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph)
{

	for (auto NI = pGraph->BegNI(); NI < pGraph->EndNI(); NI++)
		pGraph->SetNDat(NI.GetId(), 0.0);
}


struct Order
{
    inline bool operator()(std::pair<int,double> const& a, std::pair<int,double> const& b) const {return a.second > b.second;}
};

//! Given pGraph with data about edge weights, computes the distance of the shortest paths from sourceNode
//! and returns the result in the nodes of pDAGGraph.
//! Updates the edges if bUpdateEdges is set to true. Default is false. In that case only the node data is updated with the shortest distance to sourceNode.
//! @note Requires initial values for the nodes of pDAGGraph (edges are not needed)
void Dijkstra(const TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, int sourceNode, double dThreshold, TPt<TNodeEDatNet<TFlt, TFlt>>& pDAGGraph, bool bUpdateEdges = false)
{
	double logThreshold = log(dThreshold);
	if(dThreshold==0)
		logThreshold=-DBL_MAX;

	// List of visited nodes
	std::map<int, bool> visitedNodes;
	// Stores the edge vertices to build the final DAG
	std::map<int, int> mapPrevious;
	std::priority_queue<std::pair<int,double>, std::vector<std::pair<int,double>>, Order> nodesToVisit;

	// Distance from source node to itself is 0
	pDAGGraph->SetNDat(sourceNode, 0);
	nodesToVisit.push(std::make_pair(sourceNode,0));

	// Beginning of the loop of Dijkstra algorithm

	while(!nodesToVisit.empty())
	{
		// Find the vertex in queue with the smallest distance and remove it
		int iParentID = -1;
		while (!nodesToVisit.empty() && visitedNodes[iParentID = nodesToVisit.top().first])
			nodesToVisit.pop();
		if (iParentID == -1) break;

		// mark the vertex with the shortest distance
		visitedNodes[iParentID]=true;

		auto parent = pGraph->GetNI(iParentID);
		int numChildren = parent.GetOutDeg();
		for(int i = 0; i < numChildren; ++i)
		{
			int iChildID = parent.GetOutNId(i);
			// Accumulate the shortest distance from source
			double alt = pDAGGraph->GetNDat(iParentID) - log(parent.GetOutEDat(i).Val);
			if(alt >= logThreshold)
			{
				auto it = visitedNodes.find(iChildID);
				if (alt < pDAGGraph->GetNDat(iChildID) && it->second == false)
				{
					//1. update distance
					//2. update the predecessor
					//3. push new shortest rank of chidren nodes
					pDAGGraph->SetNDat(iChildID, alt);
					mapPrevious[iChildID]= iParentID;
					nodesToVisit.push(std::make_pair(iChildID,alt));
				}
			}
		}

	}

	if(bUpdateEdges)
		for(auto it=mapPrevious.begin(); it!= mapPrevious.end(); ++it)
		{
			pDAGGraph->AddEdge(it->second, it->first);
			pDAGGraph->SetEDat(it->second,it->first, pGraph->GetEDat(it->second,it->first));
		}
}

TPt<TNodeEDatNet<TFlt, TFlt>> MIOA(const TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, int sourceNode, double dThreshold)
{
	//////////////////////////////////////////////////////////////
	// Compte the Maximum Influence Out-Arborescence with Dijkstra

	// Copy the nodes of pGraph
	auto pDAGGraph = TNodeEDatNet<TFlt, TFlt>::New();
	for (auto NI = pGraph->BegNI(); NI < pGraph->EndNI(); NI++)
	{
		int NodeID = NI.GetId();
		pDAGGraph->AddNode(NodeID);
		pDAGGraph->SetNDat(NodeID, FLT_MAX);
	}
	Dijkstra(pGraph, sourceNode, dThreshold, pDAGGraph, true);

	// pDAGGraph is the MIOA starting from sourceNode

	return pDAGGraph;
}


///////////
// DAG 1 //
///////////

TPt<TNodeEDatNet<TFlt, TFlt>> GenerateDAG1(const TPt<TNodeEDatNet<TFlt, TFlt>> &pGraph, const std::vector<int>& seedNodes, double threshold)
{
	// Copy pGraph into pGraph_DAG1
	auto pGraph_DAG1 = TNodeEDatNet<TFlt, TFlt>::New();

	for (auto NI = pGraph->BegNI(); NI < pGraph->EndNI(); NI++)
		pGraph_DAG1->AddNode(NI.GetId());
	
	for (auto EI = pGraph->BegEI(); EI < pGraph->EndEI(); EI++)
	{
		pGraph_DAG1->AddEdge(EI.GetSrcNId(),EI.GetDstNId());
		pGraph_DAG1->SetEDat(EI.GetSrcNId(),EI.GetDstNId(), pGraph->GetEDat(EI.GetSrcNId(),EI.GetDstNId()));
	}

	// Create a super root in order to update in one pass all the shortest paths from vSeedIDs nodes
	int superRootID =  pGraph_DAG1->GetMxNId()+1;
	pGraph_DAG1->AddNode(superRootID);

	for(int srcNode: seedNodes)
	{
		pGraph_DAG1->AddEdge(superRootID, srcNode);
		pGraph_DAG1->SetEDat(superRootID, srcNode, 1.0);
	}
	pGraph_DAG1 = MIOA(pGraph_DAG1, superRootID, threshold);
	// Remove the artificial super root node
	pGraph_DAG1->DelNode(superRootID);


	// Add back other edges with the condition r(u)<r(v)
	for (auto EI = pGraph->BegEI(); EI < pGraph->EndEI(); EI++)
	{
		int u = EI.GetSrcNId(), v = EI.GetDstNId();
		if(pGraph_DAG1->GetNDat(u)< pGraph_DAG1->GetNDat(v))
		{
			if (!pGraph_DAG1->IsEdge(u,v))
			{
				pGraph_DAG1->AddEdge(u,v);
				pGraph_DAG1->SetEDat(u,v,EI.GetDat());
			}
		}
	}
	//Reset Node data from the original graph
	for (auto NI = pGraph->BegNI(); NI < pGraph->EndNI(); NI++)
		pGraph_DAG1->SetNDat(NI.GetId(),NI.GetDat().Val);

	return pGraph_DAG1;
}

TPt<TNodeEDatNet<TFlt, TFlt>> GenerateDAG1(const TPt<TNodeEDatNet<TFlt, TFlt>> &pGraph, int sourceNode, double threshold)
{
	std::vector<int> vSeedIDs;
	vSeedIDs.push_back(sourceNode);
	return GenerateDAG1(pGraph, vSeedIDs, threshold);
}

///////////
// DAG 2 //
///////////

//! Graph union between the graphs of vGraphs
//! @note Does not copy edge or node data.
TPt<TNodeEDatNet<TFlt, TFlt>> GraphUnion(const std::vector<TPt<TNodeEDatNet<TFlt, TFlt>>> &vGraphs)
{
	auto pOut = TNodeEDatNet<TFlt, TFlt>::New();
	for(auto it=vGraphs.begin(); it!=vGraphs.end(); ++it)
	{
		for (auto NI = (*it)->BegNI(); NI < (*it)->EndNI(); NI++)
		{
			int iParentID = NI.GetId();
			if(!pOut->IsNode(iParentID))
				pOut->AddNode(iParentID);
			for (int e = 0; e < NI.GetOutDeg(); ++e)
			{
				int iChildID = NI.GetOutNId(e);
				if(!pOut->IsNode(iChildID))
					pOut->AddNode(iChildID);
				pOut->AddEdge(iParentID,iChildID);
			}
		}
	}

	return pOut;
}
//???????
TPt<TNodeEDatNet<TFlt, TFlt>> GenerateDAG2(const TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, const std::vector<int> &vSeedIDs, double dThreshold)
{
	// Vector of MIOA graphs per seed node
	std::vector<TPt<TNodeEDatNet<TFlt, TFlt>>> vMIOAGraphs;

	// Compute the union of MIOA for each node of vSeedIDs
	for(auto it=vSeedIDs.begin(); it!=vSeedIDs.end(); ++it)
		vMIOAGraphs.push_back(MIOA(pGraph, *it, dThreshold));
	auto pOut = GraphUnion(vMIOAGraphs);

	// Set node data
	for (auto NI = pOut->BegNI(); NI < pOut->EndNI(); NI++)
		pOut->SetNDat(NI.GetId(), FLT_MAX);

	// Copy the edge weights from pGraph
	for (auto EI = pOut->BegEI(); EI < pOut->EndEI(); EI++)
		pOut->SetEDat(EI.GetSrcNId(), EI.GetDstNId(), pGraph->GetEDat(EI.GetSrcNId(), EI.GetDstNId()));

	// Create a super root in order to update in one pass all the shortest paths from vSeedIDs nodes
	int superRootID = pGraph->GetMxNId()+1;
	pOut->AddNode(superRootID);
	for(auto it=vSeedIDs.begin(); it!=vSeedIDs.end(); ++it)
	{
		pOut->AddEdge(superRootID, *it);
		pOut->SetEDat(superRootID, *it, 1.0);
	}
	Dijkstra(pOut, superRootID, dThreshold, pOut);
	// Remove the artificial super root node
	pOut->DelNode(superRootID);

	// Traverse the edges and prune the graph
	for (auto EI = pOut->BegEI(); EI < pOut->EndEI(); EI++)
	{
		if(EI.GetDstNDat().Val < EI.GetSrcNDat().Val)
			pOut->DelEdge(EI.GetSrcNId(), EI.GetDstNId());
	}

	//Reset Node data from the original graph
	for (auto NI = pGraph->BegNI(); NI < pGraph->EndNI(); NI++)
		pOut->SetNDat(NI.GetId(),NI.GetDat().Val);

	return pOut;
}

TPt<TNodeEDatNet<TFlt, TFlt>> GenerateDAG2(const TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, int sourceNode, double dThreshold)
{
	std::vector<int> vSeedIDs;
	vSeedIDs.push_back(sourceNode);
	return GenerateDAG2(pGraph, vSeedIDs, dThreshold);
}


std::vector<int> GetPeerSeeds(std::map<int,TPt<TNodeEDatNet<TFlt, TFlt>> > mMIOAs, int nodeID)
{
	std::vector<int> vPeerSeeds;
	for(auto it = mMIOAs.begin(); it!= mMIOAs.end(); it++)
	{
		if(it->first!=nodeID)
		{
			int isOverlapped = false;
			auto pMIOA = it->second;
			for(auto v = mMIOAs[nodeID]->BegNI(); v < mMIOAs[nodeID]->EndNI();v++)
			{
				if (isOverlapped) break;
				for(auto u = pMIOA->BegNI();u < pMIOA->EndNI(); u++)
					if(v.GetId()== u.GetId())
					{
						isOverlapped = true;
						break;
					}
			}
			if(isOverlapped) vPeerSeeds.push_back(it->first);
		}
	}

	return vPeerSeeds;
}

std::vector<int> MaxIncrementalInfluence(TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, int numRounds){

	std::vector<int> vSeedSet;
	tbb::concurrent_unordered_map<int,double> mSpreadIncrement;
	auto pGraph_temp = TNodeEDatNet<TFlt, TFlt>::New();
	double influence = 0.0; int i,chunk = 50;
	static tbb::spin_mutex sMutex;

	//Failure of using PeerSeeds due to insufficient memory
	//std::map<int,std::vector<int> > mPeerSeeds;
	//std::map<int,TPt<TNodeEDatNet<TFlt, TFlt>> > mMIOAs;

	/* Initialization*/
	int numNodes = pGraph->GetMxNId();
	#pragma omp parallel shared(pGraph,chunk,mSpreadIncrement) private(pGraph_temp,i)
	{
		#pragma omp for schedule(dynamic,chunk) nowait
			for (i =0;i<numNodes;++i)
			{
				if(pGraph->IsNode(i))
				{
					pGraph_temp = MIOA(pGraph, i, 0);
					InitializationBeforePropagation(pGraph_temp);
					ParallelBPFromNode_1DPartitioning(pGraph_temp, i);
					mSpreadIncrement[i]=InfluenceSpreadFromSeedNodes(pGraph_temp);
					//mMIOAs.insert(std::make_pair(i,pGraph_v));
				}

			}
	}

/*
	//build PeerSeeds
	//Failure due to the insufficient memory
	for (int v =0; v<pGraph->GetNodes();++v)
		if(pGraph->IsNode(v))
			mPeerSeeds[v]=GetPeerSeeds(mMIOAs,v);
*/

	cout<<"--------------------------Finished Initialization---------------------"<<endl;

	for (int i=0;i<numRounds;++i)
	{
		/* select the i'th seed by finding u = argmax(mSpreadIncrement)*/
		auto it = std::max_element(mSpreadIncrement.begin(),mSpreadIncrement.end(),
							[&](std::pair<int,double> const& a, std::pair<int,double> const& b) {
								 return a.second < b.second;
								 }
							);
		int SeedID = it->first;
		cout << SeedID <<endl;

		/* calculate the current influence spread */
		vSeedSet.push_back(SeedID);
		pGraph = GenerateDAG1(pGraph, vSeedSet, 0.0);
		ParallelBPFromNode_1DPartitioning(pGraph, vSeedSet);
		influence = InfluenceSpreadFromSeedNodes(pGraph);

		/*remove the newly selected node*/
		mSpreadIncrement.unsafe_erase(SeedID);

		/* update incremental influence spread for each round */
		double Delta_MAX = 0.0;
		std::vector<int> vSeedSet_temp = vSeedSet;
		#pragma omp parallel shared(pGraph,chunk,vSeedSet,mSpreadIncrement,Delta_MAX) private(pGraph_temp,vSeedSet_temp,i)
		{
			#pragma omp for schedule(dynamic,chunk) nowait
				for (i =0;i<numNodes;++i)
				{
					/* exclude the nodes in seed set */
					auto result = std::find(vSeedSet.begin(),vSeedSet.end(), i);
					if (result != vSeedSet.end()) continue;

					if(pGraph->IsNode(i) && mSpreadIncrement[i] > Delta_MAX)
					{
						/*different processors use different copied vSeedSet*/
						vSeedSet_temp.push_back(i);

						pGraph_temp = GenerateDAG1(pGraph, vSeedSet_temp, 0);
						ParallelBPFromNode_1DPartitioning(pGraph_temp, vSeedSet_temp);
						mSpreadIncrement[i]=InfluenceSpreadFromSeedNodes(pGraph_temp)-influence;
						if (mSpreadIncrement[i]> Delta_MAX)
						{
							tbb::spin_mutex::scoped_lock lock(sMutex);
							Delta_MAX = mSpreadIncrement[i];
						}
						vSeedSet_temp.pop_back();
					}
				}
		}
	}
	return vSeedSet;
}


