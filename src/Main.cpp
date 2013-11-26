#include <iostream>
#include <vector>
#include "tbb/tick_count.h"
#include "BP.h"
#include "Utilities.h"
#include "Snap.h"

using namespace std;

//#define _TEST_RANDOM_Bayesian_Network
//#define _TEST_DAG1_GRAPH
//#define _TEST_DAG2
//#define _TEST_GRAPH
//#define _TEST_Email_EuAll
//#define _TEST_p2p_Gnutella04
//#define _LOAD_FROM_FILE
//#define _SAVE_TO_FILE
//#define _TEST_Amazon0302
//#define _TEST_Slashdot0902
#define _CREATE_DATA_SETS
//#define _TEST_EDGE_SAVE



// Discussion about variable declaration and loops
// http://stackoverflow.com/questions/982963/is-there-any-overhead-to-declaring-a-variable-within-a-loop-c
void TestGraph(TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, std::vector<int> vSeedIDs, int numIterations)
{

	cout << "Starting BP from nodeID {";
	for(int sourceNode : vSeedIDs)
		cout << sourceNode << " ";
	cout << "}\nThe input graph has " << pGraph->GetNodes() << " nodes and " << pGraph->GetEdges() << " edges.\n";

	auto pGraph_test = CopyGraph(pGraph);
	// Start traversing the graph
	tbb::tick_count tic = tbb::tick_count::now();
	for(int i = 0; i<numIterations; ++i)
		ParallelBPFromNode(pGraph_test, vSeedIDs);
	double dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for parallel BP: " << dElapsedTime/numIterations << " seconds\n";
	cout<<"The influence spread speed is: "<<InfluenceSpreadFromSeedNodes(pGraph_test)<<endl;

	pGraph_test = CopyGraph(pGraph);
	tic = tbb::tick_count::now();
	for(int i = 0; i<numIterations; ++i)
		ParallelBPFromNode_LevelSynchronous(pGraph_test, vSeedIDs);
	dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for parallel level synchronous BP: " << dElapsedTime/numIterations << " seconds\n";
	cout<<"The influence spread speed is: "<<InfluenceSpreadFromSeedNodes(pGraph_test)<<endl;

	pGraph_test = CopyGraph(pGraph);
	tic = tbb::tick_count::now();
	for(int i = 0; i<numIterations; ++i)
		PropagateFromNode(pGraph_test, vSeedIDs);
	dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for serial BP: " << dElapsedTime/numIterations << " seconds\n";
	cout<<"The influence spread speed is: "<<InfluenceSpreadFromSeedNodes(pGraph_test)<<endl;

	pGraph_test = CopyGraph(pGraph);
	std::vector<int> vResult;
	CalculateRankFromSource(pGraph, vSeedIDs, vResult);
	tic = tbb::tick_count::now();
	for(int i = 0; i<numIterations; ++i)
		ParallelBPFromNode_SingleNodeUpdate(pGraph_test, vResult, vSeedIDs);
	dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for ParallelBPFromNode_SingleNodeUpdate: " << dElapsedTime/numIterations << " seconds\n";
	cout<<"The influence spread speed is: "<<InfluenceSpreadFromSeedNodes(pGraph_test)<<endl;
}

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
		ParallelBPFromNode_LevelSynchronous(pGraph, sourceNode);
	dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for parallel level synchronous BP: " << dElapsedTime/numIterations << " seconds\n";

	tic = tbb::tick_count::now();
	for(int i = 0; i<numIterations; ++i)
		PropagateFromNode(pGraph, sourceNode);
	dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for serial BP: " << dElapsedTime/numIterations << " seconds\n";

	std::vector<int> vResult;
	CalculateRankFromSource(pGraph, sourceNode, vResult);
	tic = tbb::tick_count::now();
	for(int i = 0; i<numIterations; ++i)
		ParallelBPFromNode_SingleNodeUpdate(pGraph, vResult, sourceNode);
	dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for ParallelBPFromNode_SingleNodeUpdate: " << dElapsedTime/numIterations << " seconds\n";
}

void TestGraphDAG(TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, std::vector<int> vSeedIDs, int numIterations)
{
	cout << "Starting BP from nodeID {";
	for(int sourceNode : vSeedIDs)
		cout << sourceNode << " ";
	cout << "}\nThe input graph has " << pGraph->GetNodes() << " nodes and " << pGraph->GetEdges() << " edges.\n";

	// Start traversing the graph
	tbb::tick_count tic = tbb::tick_count::now();
	for(int i = 0; i<numIterations; ++i)
		ParallelBPFromNodeDAG(pGraph, vSeedIDs);
	double dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for parallel BP: " << dElapsedTime/numIterations << " seconds\n";

	tic = tbb::tick_count::now();
	for(int i = 0; i<numIterations; ++i)
		ParallelBPFromNodeDAG_LevelSynchronous(pGraph, vSeedIDs);
	dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for parallel level synchronous BP: " << dElapsedTime/numIterations << " seconds\n";

	tic = tbb::tick_count::now();
	for(int i = 0; i<numIterations; ++i)
		PropagateFromNodeDAG(pGraph, vSeedIDs);
	dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for serial BP: " << dElapsedTime/numIterations << " seconds\n";

	std::vector<int> vResult;
	CalculateRankFromSource(pGraph, vSeedIDs, vResult);
	tic = tbb::tick_count::now();
	for(int i = 0; i<numIterations; ++i)
		ParallelBPFromNode_SingleNodeUpdate(pGraph, vResult, vSeedIDs);
	dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for ParallelBPFromNode_SingleNodeUpdate: " << dElapsedTime/numIterations << " seconds\n";
}

void TestGraphDAG(TPt<TNodeEDatNet<TFlt, TFlt>>& pGraph, int sourceNode, int numIterations)
{
	// Start traversing the graph
	cout << "Starting BP from nodeID " << sourceNode << ". The input graph has " << pGraph->GetNodes() << " nodes and " << pGraph->GetEdges() << " edges.\n";
	tbb::tick_count tic = tbb::tick_count::now();
	for(int i = 0; i<numIterations; ++i)
		ParallelBPFromNodeDAG(pGraph, sourceNode);
	double dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for parallel BP: " << dElapsedTime/numIterations << " seconds\n";

	tic = tbb::tick_count::now();
	for(int i = 0; i<numIterations; ++i)
		ParallelBPFromNodeDAG_LevelSynchronous(pGraph, sourceNode);
	dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for parallel level synchronous BP: " << dElapsedTime/numIterations << " seconds\n";

	tic = tbb::tick_count::now();
	for(int i = 0; i<numIterations; ++i)
		PropagateFromNodeDAG(pGraph, sourceNode);
	dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for serial BP: " << dElapsedTime/numIterations << " seconds\n";

	//auto rankGraph = CalculateRankFromSource(pGraph, sourceNode);
	std::vector<int> vResult;
	CalculateRankFromSource(pGraph, sourceNode, vResult);
	tic = tbb::tick_count::now();
	for(int i = 0; i<numIterations; ++i)
		ParallelBPFromNode_SingleNodeUpdate(pGraph, vResult, sourceNode);
	dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for ParallelBPFromNode_SingleNodeUpdate: " << dElapsedTime/numIterations << " seconds\n";
}

int main(int argc, char* argv[])
{
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
	tbb::tick_count tic = tbb::tick_count::now();
	//PropagateFromNode(pGraph, 0);
	//ParallelBPFromNode(pGraph, 0);
	ParallelBPFromNode_LevelSynchronous(pGraph, 0);
	double dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	std:: cout << pGraph->GetNDat(0).Val << " " << pGraph->GetNDat(1).Val << " " << pGraph->GetNDat(2).Val << std::endl;
#endif

#ifdef _TEST_DAG2
	auto pGraph = TNodeEDatNet<TFlt, TFlt>::New();
	for(int i=0;i<5;++i)
		pGraph->AddNode(i);

	pGraph->AddEdge(0,2);
	pGraph->AddEdge(0,3);
	pGraph->AddEdge(1,2);
	pGraph->AddEdge(1,3);
	pGraph->AddEdge(2,3);
	pGraph->AddEdge(2,4);
	pGraph->AddEdge(3,4);

	pGraph->SetEDat(0,2,0.5);
	pGraph->SetEDat(0,3,0.4);
	pGraph->SetEDat(1,2,0.3);
	pGraph->SetEDat(1,3,0.4);
	pGraph->SetEDat(2,3,0.5);
	pGraph->SetEDat(2,4,0.4);
	pGraph->SetEDat(3,4,0.5);

	std::vector<int> vSeedNodeIDs;
	vSeedNodeIDs.push_back(0);
	vSeedNodeIDs.push_back(1);
	pGraph = GenerateDAG2(pGraph, vSeedNodeIDs, 0);
	//pGraph = MIOA(pGraph, 0, 0);
	TSnap::SaveGViz(pGraph, "test.gv", "Test DAG", true);
#endif

#ifdef _LOAD_FROM_FILE
	TFIn FIn("test.graph");
	auto pGraph = TNodeEDatNet<TFlt, TFlt>::Load(FIn);

	//for (auto EI = pGraph->BegEI(); EI < pGraph->EndEI(); EI++)
	//std::cout << EI.GetSrcNId() << " " << EI.GetDstNId() << " " << EI.GetDat() << std::endl;

	/*
		std::cout << "ExactBP_Marginalization(pGraph, 3)\n";
		ExactBP_Marginalization(pGraph, 3);
		auto ExactBPGraph = CopyGraph(pGraph);
		for (auto NI = pGraph->BegNI(); NI < pGraph->EndNI(); NI++)
			std::cout << NI.GetId() << " " << NI.GetDat() << std::endl;

		ResetGraphBelief(pGraph);
		std::cout << "PropagateFromNode(pGraph, 3)\n";
	 */

	std::vector<int> mapRanks;
	CalculateRankFromSource(pGraph, 3, mapRanks);

	ParallelBPFromNode_SingleNodeUpdate(pGraph, mapRanks, 3);
	for (auto NI = pGraph->BegNI(); NI < pGraph->EndNI(); NI++)
		std::cout << NI.GetId() << " " << NI.GetDat() << std::endl;

	//std::cout << "Error : " << BPError(ExactBPGraph, pGraph, [] (double a, double b) -> double { return abs(a-b); })  << std::endl;

	/*
		std::vector<int> vSeedNodes;
		vSeedNodes.push_back(0);
		vSeedNodes.push_back(3);
		pGraph = GenerateDAG2(pGraph, vSeedNodes);
		TSnap::SaveGViz(pGraph, "testDAG.gv", "Test DAG", true);
	 */
#endif

#ifdef _SAVE_TO_FILE

	auto pGraph = GenerateRandomBayesianNetwork(1, 5, 5, 7, 30);
	TFOut FOut("test.graph");
	pGraph->Save(FOut);
	TSnap::SaveGViz(pGraph, "test.gv", "Test DAG", true);
#endif

#ifdef _TEST_p2p_Gnutella04
	auto pGraph = TSnap::LoadEdgeList<TPt<TNodeEDatNet<TFlt, TFlt>>>("datasets/p2p-Gnutella04.txt", 0, 1);
	RandomGraphInitialization(pGraph,0.01,0.1);
	std::vector<int> vSeedNodeIDs;
	vSeedNodeIDs.push_back(3109);
	vSeedNodeIDs.push_back(5617);
	vSeedNodeIDs.push_back(9134);
	//pGraph = GenerateDAG1(pGraph, vSeedNodeIDs, 0);
	//ParallelBPFromNodeDAG_LevelSynchronous(pGraph, vSeedNodeIDs);
	//cout<<"The influence spread speed is: "<<InfluenceSpreadFromSeedNodes(pGraph)<<endl;
	//pGraph = GenerateDAG2(pGraph, vSeedNodeIDs, 0);

	/*
		std::cout << "ExactBP_Marginalization(pGraph, vSeedNodeIDs)\n";	// Crash out of memory
		ExactBP_Marginalization(pGraph, vSeedNodeIDs);
		auto ExactBPGraph = CopyGraph(pGraph);

		ResetGraphBelief(pGraph);
		std::cout << "PropagateFromNode(pGraph, vSeedNodeIDs)\n";
		PropagateFromNode(pGraph, vSeedNodeIDs);

		std::cout << "Error : " << BPError(ExactBPGraph, pGraph, [] (double a, double b) -> double { return abs(a-b); })  << std::endl;
	 */

	//pGraph = GenerateDAG2(pGraph, 0, 0);
	//TestGraph(pGraph, vSeedNodeIDs, 100);


	auto pGraph_test = CopyGraph(pGraph);
	std::vector<int> vSeedSet;
/*	ParallelGreedyCELF(pGraph_test , 10, vSeedSet);
	pGraph_test = GenerateDAG1(pGraph_test , vSeedSet, 0);
	ParallelBPFromNodeDAG_LevelSynchronous(pGraph_test , vSeedSet);
	cout<<"The influence spread speed is: "<<InfluenceSpreadFromSeedNodes(pGraph_test)<<endl;
*/
/*	pGraph_test = CopyGraph(pGraph);
	vSeedSet.clear();
	tbb::tick_count tic = tbb::tick_count::now();
	ParallelNewGreedIC(pGraph_test,10,1000, vSeedSet);
	double dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for Old: " << dElapsedTime << " seconds\n";
//	pGraph_test = GenerateDAG1(pGraph_test, vSeedSet, 0);
//	ParallelBPFromNodeDAG_LevelSynchronous(pGraph_test, vSeedSet);
//	cout<<"The influence spread speed is: "<<InfluenceSpreadFromSeedNodes(pGraph_test)<<endl;



	vSeedSet.clear();
	//std::vector<int> vSeedSet;
	tic = tbb::tick_count::now();
	ParallelNewGreedIC_Nested(pGraph,10,1000, vSeedSet);
	dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for NEW: " << dElapsedTime << " seconds\n";
	pGraph = GenerateDAG1(pGraph, vSeedSet, 0);
	ParallelBPFromNodeDAG_LevelSynchronous(pGraph, vSeedSet);
	cout<<"The influence spread speed is: "<<InfluenceSpreadFromSeedNodes(pGraph)<<endl;
*/



#endif

#ifdef _TEST_Email_EuAll
	auto pGraph = TSnap::LoadEdgeList<TPt<TNodeEDatNet<TFlt, TFlt>>>("datasets/Email-EuAll.txt", 0, 1);
	/*
	std::vector<int> vSeedNodeIDs;
	vSeedNodeIDs.push_back(0);
	vSeedNodeIDs.push_back(2);
	vSeedNodeIDs.push_back(6);
    */
	RandomGraphInitialization(pGraph);
	std::vector<int> vSeedSet = MaxIncrementalInfluence(pGraph,5);
	//tbb::tick_count tic = tbb::tick_count::now();
	//pGraph = GenerateDAG1(pGraph, vSeedNodeIDs, 0);
	//pGraph = GenerateDAG2(pGraph, 0, 0);
	//double dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	//cout << "Time elapsed for DAG2 computation: " << dElapsedTime << " seconds\n";

	//TestGraph(pGraph, 0, 100);
#endif

#ifdef _TEST_Amazon0302
	auto pGraph = TSnap::LoadEdgeList<TPt<TNodeEDatNet<TFlt, TFlt>>>("datasets/Amazon0302.txt", 0, 1);
	RandomGraphInitialization(pGraph);
	std::vector<int> vSeedSet;
	MaxIncrementalInfluence(pGraph, 5, vSeedSet);
#endif

#ifdef _TEST_Slashdot0902
	auto pGraph = TSnap::LoadEdgeList<TPt<TNodeEDatNet<TFlt, TFlt>>>("datasets/Slashdot0902.txt", 0, 1);
	RandomGraphInitialization(pGraph);
	tbb::tick_count tic = tbb::tick_count::now();
	std::vector<int> vSeedSet;
	MaxIncrementalInfluence(pGraph, 5, vSeedSet);
	double dElapsedTime = (tbb::tick_count::now() - tic).seconds();
	cout << "Time elapsed for MaxInfluence computation: " << dElapsedTime << " seconds\n";
#endif
#ifdef _TEST_EDGE_SAVE
	 SaveEdgeWeightsToFile(pGraph, "BLABLA.txt");

	 //LoadEdgeWeightsFromFile(pGraph, "BLABLA.txt");
#endif
#ifdef _CREATE_DATA_SETS

	auto pGraph = TSnap::LoadEdgeList<TPt<TNodeEDatNet<TFlt, TFlt>>>("datasets/Email-EuAll.txt", 0, 1);
	RandomGraphInitialization(pGraph,0.01,0.1);
	SaveEdgeWeightsToFile(pGraph, "Email-EuAll-Network.txt");

	pGraph = TSnap::LoadEdgeList<TPt<TNodeEDatNet<TFlt, TFlt>>>("datasets/p2p-Gnutella04.txt", 0, 1);
	RandomGraphInitialization(pGraph,0.01,0.1);
	SaveEdgeWeightsToFile(pGraph, "p2p-Gnutella04-Network.txt");

	pGraph = TSnap::LoadEdgeList<TPt<TNodeEDatNet<TFlt, TFlt>>>("datasets/Amazon0302.txt", 0, 1);
	RandomGraphInitialization(pGraph,0.01,0.1);
	SaveEdgeWeightsToFile(pGraph, "Amazon0302-Network.txt");

	pGraph = TSnap::LoadEdgeList<TPt<TNodeEDatNet<TFlt, TFlt>>>("datasets/Slashdot0902.txt", 0, 1);
	RandomGraphInitialization(pGraph,0.01,0.1);
	SaveEdgeWeightsToFile(pGraph, "Slashdot0902-Network.txt");

	pGraph = TSnap::LoadEdgeList<TPt<TNodeEDatNet<TFlt, TFlt>>>("datasets/web-Stanford.txt", 0, 1);
	RandomGraphInitialization(pGraph,0.01,0.1);
	SaveEdgeWeightsToFile(pGraph, "web-Stanford-Network.txt");

#endif


#ifdef _WIN32
	system("pause");
#endif
#ifdef __linux__
	SystemPause();
#endif


	return 0;
}


