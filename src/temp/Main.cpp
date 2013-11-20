#include <iostream>
#include <queue>
#include <map>

#include "tbb/tick_count.h"

#include "BP.h"
#include "DAG.h"
#include "Utilities.h"
#include "Snap.h"

using namespace std;


//#define _TEST_RANDOM_Bayesian_Network
//#define _TEST_DAG1_GRAPH
//#define _TEST_GRAPH
//#define _TEST_Email_EuAll
#define _TEST_p2p_Gnutella04
//#define _LOAD_FROM_FILE
//#define _SAVE_TO_FILE
//#define  _TEST_soc-LiveJournal1
//#define _TEST_web-BerkStan

void TestGraph(TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, int sourceNode, int numIterations)
{
	// Start traversing the graph
	cout << "Starting BP from nodeID " << sourceNode << ". The input graph has " << pGraph->GetNodes() << " nodes and " << pGraph->GetEdges() << " edges.\n";
	tbb::tick_count tic = tbb::tick_count::now();
	for(int i = 0; i<numIterations; ++i)
	ParallelBPFromNode(pGraph, sourceNode);
	double dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for parallel BP: " << dElapsedTime/numIterations << " seconds\n";

	tic = tbb::tick_count::now();
	for(int i = 0; i<numIterations; ++i)
	ParallelBPFromNode_1DPartitioning(pGraph, sourceNode);
	dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for parallel 1D partitioning BP: " << dElapsedTime/numIterations << " seconds\n";

	tic = tbb::tick_count::now();
	for(int i = 0; i<numIterations; ++i)
	ParallelBPFromNode_1DPartitioning_Hybrid(pGraph, sourceNode);
	dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for parallel 1D partitioning hybrid BP: " << dElapsedTime/numIterations << " seconds\n";

	tic = tbb::tick_count::now();
	for(int i = 0; i<numIterations; ++i)
	PropagateFromNode(pGraph, sourceNode);
	dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for serial BP: " << dElapsedTime/numIterations << " seconds\n";


}

void TestDAG1(TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, double threshold, std::vector<int> seedNodes)
{
	cout << "The input graph has " << pGraph->GetNodes() << " nodes and " << pGraph->GetEdges() << " edges.\n";
	tbb::tick_count tic = tbb::tick_count::now();
	auto pGraph_DAG1 = GenerateDAG1(pGraph,threshold,seedNodes);
	double dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout <<"The output DAG1 graph has " << pGraph_DAG1->GetNodes() << " nodes and " << pGraph_DAG1->GetEdges() << " edges.\n";
	cout << "Time elapsed for Parallel DAG1: " << dElapsedTime << " seconds\n";
}


int main(int argc, char* argv[])
{

#ifdef _TEST_RANDOM_Bayesian_Network
	auto pGraph = GenerateRandomBayesianNetwork (3,5,4,5,30);
	//TestGraph(pGraph,0,10);
	TestDAG(pGraph,0);

#endif

#ifdef _TEST_DAG1_GRAPH
	auto pGraph = TNodeEDatNet<TFlt, TFlt>::New();
	pGraph->AddNode(0);
	pGraph->SetNDat(0,1);
	pGraph->AddNode(1);
	pGraph->SetNDat(1,1);
	pGraph->AddNode(2);
	pGraph->SetNDat(2,1);
	pGraph->AddNode(3);
	pGraph->SetNDat(3,1);
	pGraph->AddEdge(0,1);
	pGraph->SetEDat(0,1, 0.9);
	pGraph->AddEdge(0,2);
	pGraph->SetEDat(0,2, 0.1);
	pGraph->AddEdge(1,2);
	pGraph->SetEDat(1,2, 0.9);
	pGraph->AddEdge(1,3);
	pGraph->SetEDat(1,3, 0.4);
	pGraph->AddEdge(2,3);
	pGraph->SetEDat(2,3, 0.1);
	pGraph->AddEdge(0,3);
	pGraph->SetEDat(0,3, 0.8);
	auto pGraph_DAG1 =  GenerateDAG1(pGraph,0.0,0);
#endif


#ifdef _TEST_GRAPH
	auto pGraph = TNodeEDatNet<TFlt, TFlt>::New();
	pGraph->AddNode(0);
	pGraph->SetNDat(0, 1);
	pGraph->AddNode(1);
	pGraph->SetNDat(1, 0);
	pGraph->AddNode(2);
	pGraph->SetNDat(2, 0);
	pGraph->AddEdge(0,1);
	pGraph->SetEDat(0,1, 0.5);
	pGraph->AddEdge(0,2);
	pGraph->SetEDat(0,2, 0.5);
	pGraph->AddEdge(1,2);
	pGraph->SetEDat(1,2, 0.5);
	TestGraph(pGraph,0,1);
#endif

#ifdef _LOAD_FROM_FILE
	TFIn FIn("test.graph");
	auto pGraph = TNodeEDatNet<TFlt, TFlt>::Load(FIn);
	TestGraph(pGraph, 0, 100);
#endif

#ifdef _SAVE_TO_FILE
	auto pGraph = GenerateRandomBayesianNetwork(1, 5, 5, 7, 30);
	TFOut FOut("test.graph");
	pGraph->Save(FOut);
	TSnap::SaveGViz(pGraph, "test.gv", "Test DAG", true);
	//dot -Tpng test.gv > test.png
#endif

#ifdef _TEST_p2p_Gnutella04
	auto pGraph = TSnap::LoadEdgeList<TPt<TNodeEDatNet<TFlt, TFlt>>>("datasets/p2p-Gnutella04.txt", 0, 1);
	//TestGraph(pGraph, 0, 1);
	std::vector<int> seedNodes;
	seedNodes.push_back(0);
	//seedNodes.push_back(26);
	//seedNodes.push_back(38);
	TestDAG1(pGraph,0.0, seedNodes);

#endif

#ifdef _TEST_Email_EuAll
	auto pGraph = TSnap::LoadEdgeList<TPt<TNodeEDatNet<TFlt, TFlt>>>("datasets/Email-EuAll.txt", 0, 1);
	//TestGraph(pGraph, 0, 100);
	std::vector<int> seedNodes;
	seedNodes.push_back(0);
	seedNodes.push_back(4);
	//seedNodes.push_back(38);
	TestDAG1(pGraph,0.0, seedNodes);

#endif

#ifdef _TEST_web-BerkStan
	auto pGraph = TSnap::LoadConnList<TPt<TNodeEDatNet<TFlt, TFlt>>>("datasets/web-BerkStan.txt");
	//TestGraph(pGraph, 1, 1);

#endif

#ifdef _TEST_soc_pokec_relationships
	auto pGraph = TSnap::LoadConnList<TPt<TNodeEDatNet<TFlt, TFlt>>>("soc-pokec-relationships.txt");
	TestGraph(pGraph, 1, 1);
#endif

#ifdef _TEST_soc-LiveJournal1
	auto pGraph = TSnap::LoadConnList<TPt<TNodeEDatNet<TFlt, TFlt>>>("datasets/soc-LiveJournal1.txt");
	//TestGraph(pGraph, 1, 1);
#endif


	return 0;
}


