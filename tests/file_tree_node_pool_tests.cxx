#include "space_fossils/file_tree/node_pool.hxx"
#include "space_fossils/file_tree/node.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <utility>

namespace space_fossils::tests {
	namespace {
		using namespace space_fossils::core::file_tree;
	}

	SF_TEST(file_tree_node_pool, StartsEmpty)
	{
		NodePool pool(128);

		SF_ASSERT_EQ(pool.GetBlockSize(), 128);
		SF_ASSERT_EQ(pool.GetLiveNodesCount(), 0);
		SF_ASSERT_EQ(pool.GetBlocksCount(), 0);
		SF_ASSERT_EQ(pool.GetAllocatedBytes(), 0);
		SF_ASSERT_EQ(pool.GetUsedBytes(), 0);
	}

	SF_TEST(file_tree_node_pool, CreateAllocatesDefaultNode)
	{
		NodePool pool(128);

		Node* node = pool.Create();

		SF_ASSERT_EQ(node != nullptr, true);
		SF_ASSERT_EQ(pool.GetLiveNodesCount(), 1);
		SF_ASSERT_EQ(pool.GetBlocksCount(), 1);
		SF_ASSERT_EQ(pool.GetUsedBytes() != 0, true);
		SF_ASSERT_EQ(node->name.data == nullptr, true);
		SF_ASSERT_EQ(node->name.length, 0);
		SF_ASSERT_EQ(node->logicalSize, DefaultFileSize);
		SF_ASSERT_EQ(node->entryType, EntryType::Unknown);
		SF_ASSERT_EQ(node->entryStatus, EntryStatus::Unknown);
		SF_ASSERT_EQ(node->scanStatus, EntryScanStatus::Unknown);
		SF_ASSERT_EQ(node->parent == nullptr, true);
		SF_ASSERT_EQ(node->firstChild == nullptr, true);
		SF_ASSERT_EQ(node->nextSibling == nullptr, true);
	}

	SF_TEST(file_tree_node_pool, CreateCreatesNewBlockWhenCurrentBlockIsFull)
	{
		NodePool pool(sizeof(Node));

		Node* first = pool.Create();
		Node* second = pool.Create();

		SF_ASSERT_EQ(first != nullptr, true);
		SF_ASSERT_EQ(second != nullptr, true);
		SF_ASSERT_EQ(pool.GetLiveNodesCount(), 2);
		SF_ASSERT_EQ(pool.GetBlocksCount(), 2);
	}

	SF_TEST(file_tree_node_pool, DestroyRemovesLiveNodeButKeepsArenaMemory)
	{
		NodePool pool(128);
		Node* node = pool.Create();
		std::size_t usedBytes = pool.GetUsedBytes();

		bool destroyed = pool.Destroy(node);

		SF_ASSERT_EQ(destroyed, true);
		SF_ASSERT_EQ(pool.GetLiveNodesCount(), 0);
		SF_ASSERT_EQ(pool.GetBlocksCount(), 1);
		SF_ASSERT_EQ(pool.GetUsedBytes(), usedBytes);
	}

	SF_TEST(file_tree_node_pool, DestroyRejectsNullAndForeignNodes)
	{
		NodePool pool(128);
		Node foreignNode;

		SF_ASSERT_EQ(pool.Destroy(nullptr), false);
		SF_ASSERT_EQ(pool.Destroy(&foreignNode), false);
		SF_ASSERT_EQ(pool.GetLiveNodesCount(), 0);
		SF_ASSERT_EQ(pool.GetBlocksCount(), 0);
	}

	SF_TEST(file_tree_node_pool, DestroyRejectsSameNodeTwice)
	{
		NodePool pool(128);
		Node* node = pool.Create();

		SF_ASSERT_EQ(pool.Destroy(node), true);
		SF_ASSERT_EQ(pool.Destroy(node), false);
		SF_ASSERT_EQ(pool.GetLiveNodesCount(), 0);
	}

	SF_TEST(file_tree_node_pool, ResetKeepsBlocksAndClearsLiveNodesAndUsedBytes)
	{
		NodePool pool(sizeof(Node));
		pool.Create();
		pool.Create();

		pool.Reset();

		SF_ASSERT_EQ(pool.GetLiveNodesCount(), 0);
		SF_ASSERT_EQ(pool.GetBlocksCount(), 2);
		SF_ASSERT_EQ(pool.GetUsedBytes(), 0);

		Node* node = pool.Create();

		SF_ASSERT_EQ(node != nullptr, true);
		SF_ASSERT_EQ(pool.GetLiveNodesCount(), 1);
		SF_ASSERT_EQ(pool.GetBlocksCount(), 2);
	}

	SF_TEST(file_tree_node_pool, ReleaseClearsBlocksAndLiveNodes)
	{
		NodePool pool(128);
		pool.Create();

		pool.Release();

		SF_ASSERT_EQ(pool.GetLiveNodesCount(), 0);
		SF_ASSERT_EQ(pool.GetBlocksCount(), 0);
		SF_ASSERT_EQ(pool.GetAllocatedBytes(), 0);
		SF_ASSERT_EQ(pool.GetUsedBytes(), 0);
	}

	SF_TEST(file_tree_node_pool, MergeFromMovesLiveNodesAndLeavesSourceEmpty)
	{
		NodePool target(128);
		NodePool source(sizeof(Node));

		Node* node = source.Create();
		SF_ASSERT_EQ(node != nullptr, true);
		node->logicalSize = 7;

		target.MergeFrom(std::move(source));

		SF_ASSERT_EQ(target.GetLiveNodesCount(), 1);
		SF_ASSERT_EQ(target.GetBlocksCount(), 1);
		SF_ASSERT_EQ(source.GetLiveNodesCount(), 0);
		SF_ASSERT_EQ(source.GetBlocksCount(), 0);
		SF_ASSERT_EQ(source.GetUsedBytes(), 0);
		SF_ASSERT_EQ(node->logicalSize, 7);
		SF_ASSERT_EQ(source.Destroy(node), false);
		SF_ASSERT_EQ(target.Destroy(node), true);
		SF_ASSERT_EQ(target.GetLiveNodesCount(), 0);
	}

	SF_TEST(file_tree_node_pool, MergeFromAppendsLiveNodesToNonEmptyTarget)
	{
		NodePool target(sizeof(Node));
		NodePool source(sizeof(Node));

		Node* targetNode = target.Create();
		Node* sourceNode = source.Create();
		SF_ASSERT_EQ(targetNode != nullptr, true);
		SF_ASSERT_EQ(sourceNode != nullptr, true);
		targetNode->logicalSize = 11;
		sourceNode->logicalSize = 22;

		target.MergeFrom(std::move(source));

		SF_ASSERT_EQ(target.GetLiveNodesCount(), 2);
		SF_ASSERT_EQ(target.GetBlocksCount(), 2);
		SF_ASSERT_EQ(source.GetLiveNodesCount(), 0);
		SF_ASSERT_EQ(source.GetBlocksCount(), 0);
		SF_ASSERT_EQ(source.GetUsedBytes(), 0);
		SF_ASSERT_EQ(targetNode->logicalSize, 11);
		SF_ASSERT_EQ(sourceNode->logicalSize, 22);
		SF_ASSERT_EQ(source.Destroy(sourceNode), false);
		SF_ASSERT_EQ(target.Destroy(targetNode), true);
		SF_ASSERT_EQ(target.Destroy(sourceNode), true);
		SF_ASSERT_EQ(target.GetLiveNodesCount(), 0);
	}

	SF_TEST(file_tree_node_pool, ZeroBlockSizeRejectsCreate)
	{
		NodePool pool(0);

		Node* node = pool.Create();

		SF_ASSERT_EQ(node == nullptr, true);
		SF_ASSERT_EQ(pool.GetLiveNodesCount(), 0);
		SF_ASSERT_EQ(pool.GetBlocksCount(), 0);
	}
}
