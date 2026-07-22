#include "space_fossils/cli/app_state.hxx"

#include "space_fossils/core/file_tree/model/tree_pool_bundle.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <filesystem>
#include <memory>
#include <optional>
#include <utility>

namespace space_fossils::tests {
	namespace {
		using namespace space_fossils::cli;
		using namespace space_fossils::core::file_tree;

		TreePoolBundle MakeSingleRootBundle()
		{
			TreePoolBundle bundle;
			bundle.namePool = std::make_unique<NamePool>(DefaultNameBlockSize);
			bundle.nodePool = std::make_unique<NodePool>(DefaultNodeBlockSize);
			bundle.root = bundle.nodePool->Create();

			SF_ASSERT_EQ(bundle.root != nullptr, true);
			bundle.root->entryType = EntryType::Directory;
			bundle.root->entryStatus = EntryStatus::Accessible;
			bundle.root->scanStatus = EntryScanStatus::Complete;
			bundle.createdNodesCount = 1;

			return bundle;
		}

		Node* PopulateTree(AppState& state)
		{
			std::optional<AppliedChange> change = state.context->storage.TryAdoptRoot(MakeSingleRootBundle());

			SF_ASSERT_EQ(change.has_value(), true);
			return change->addedRoot;
		}

		void AssertSessionUsesContextStorage(AppState& state)
		{
			SF_ASSERT_EQ(&state.context->session.GetStorage() == &state.context->storage, true);
		}

		TreeMetadata MakeTestMetadata()
		{
			TreeMetadata metadata;
			metadata.scanSourcePath = std::filesystem::path("source") / "root";
			metadata.treeSource = TreeSource::Snapshot;
			metadata.applicationVersion = "test-version";
			metadata.updatedAtUnixSeconds = 123456;
			return metadata;
		}

		void AssertMetadataEquals(const TreeMetadata& actual, const TreeMetadata& expected)
		{
			SF_ASSERT_EQ(actual.scanSourcePath == expected.scanSourcePath, true);
			SF_ASSERT_EQ(actual.treeSource, expected.treeSource);
			SF_ASSERT_EQ(actual.applicationVersion, expected.applicationVersion);
			SF_ASSERT_EQ(actual.updatedAtUnixSeconds, expected.updatedAtUnixSeconds);
		}

		void AssertDefaultMetadata(const TreeMetadata& metadata)
		{
			AssertMetadataEquals(metadata, {});
		}
	}

	SF_TEST(app_state, DefaultStateHasFreshEmptyTreeContext)
	{
		AppState state;

		SF_ASSERT_EQ(state.context != nullptr, true);
		SF_ASSERT_EQ(state.isQuitRequested, false);
		SF_ASSERT_EQ(state.HasActiveTree(), false);
		SF_ASSERT_EQ(state.IsFreshStorage(), true);
		SF_ASSERT_EQ(state.context->session.HasTree(), false);
		AssertDefaultMetadata(state.context->treeMetadata);
		AssertSessionUsesContextStorage(state);
	}

	SF_TEST(app_state, AdoptedRootMakesTreeActiveAndStorageUsed)
	{
		AppState state;
		Node* root = PopulateTree(state);

		SF_ASSERT_EQ(root != nullptr, true);
		SF_ASSERT_EQ(state.HasActiveTree(), true);
		SF_ASSERT_EQ(state.IsFreshStorage(), false);
		SF_ASSERT_EQ(state.context->session.HasTree(), true);
		SF_ASSERT_EQ(state.context->session.GetRoot() == root, true);
		AssertSessionUsesContextStorage(state);
	}

	SF_TEST(app_state, RemovedRootIsInactiveButStorageIsNotFresh)
	{
		AppState state;
		Node* root = PopulateTree(state);

		std::optional<AppliedChange> change = state.context->storage.TryRemoveSubtree(root);

		SF_ASSERT_EQ(change.has_value(), true);
		SF_ASSERT_EQ(state.HasActiveTree(), false);
		SF_ASSERT_EQ(state.IsFreshStorage(), false);
		SF_ASSERT_EQ(state.context->session.HasTree(), false);
		AssertSessionUsesContextStorage(state);
	}

	SF_TEST(app_state, ResetTreeContextCreatesFreshSessionStoragePair)
	{
		AppState state;
		PopulateTree(state);
		state.context->treeMetadata = MakeTestMetadata();
		state.isQuitRequested = true;

		state.ResetTreeContext();

		SF_ASSERT_EQ(state.context != nullptr, true);
		SF_ASSERT_EQ(state.HasActiveTree(), false);
		SF_ASSERT_EQ(state.IsFreshStorage(), true);
		SF_ASSERT_EQ(state.context->session.HasTree(), false);
		AssertDefaultMetadata(state.context->treeMetadata);
		SF_ASSERT_EQ(state.isQuitRequested, true);
		AssertSessionUsesContextStorage(state);
	}

	SF_TEST(app_state, TreeMetadataRemainsIndependentFromStorageChanges)
	{
		AppState state;
		const TreeMetadata expectedMetadata = MakeTestMetadata();
		state.context->treeMetadata = expectedMetadata;

		Node* root = PopulateTree(state);
		SF_ASSERT_EQ(state.context->storage.TryRemoveSubtree(root).has_value(), true);

		AssertMetadataEquals(state.context->treeMetadata, expectedMetadata);
		SF_ASSERT_EQ(state.HasActiveTree(), false);
		SF_ASSERT_EQ(state.IsFreshStorage(), false);
	}

	SF_TEST(app_state, FailedStorageChangeKeepsContextFresh)
	{
		AppState state;

		std::optional<AppliedChange> change = state.context->storage.TryRemoveSubtree(nullptr);

		SF_ASSERT_EQ(change.has_value(), false);
		SF_ASSERT_EQ(state.HasActiveTree(), false);
		SF_ASSERT_EQ(state.IsFreshStorage(), true);
		SF_ASSERT_EQ(state.context->storage.GetVersion(), 0);
		AssertSessionUsesContextStorage(state);
	}

	SF_TEST(app_state, RepeatedResetKeepsContextFreshAndSessionBound)
	{
		AppState state;
		PopulateTree(state);

		state.ResetTreeContext();
		state.ResetTreeContext();
		state.ResetTreeContext();

		SF_ASSERT_EQ(state.context != nullptr, true);
		SF_ASSERT_EQ(state.HasActiveTree(), false);
		SF_ASSERT_EQ(state.IsFreshStorage(), true);
		SF_ASSERT_EQ(state.context->session.HasTree(), false);
		AssertSessionUsesContextStorage(state);
	}

	SF_TEST(app_state, MoveTransfersContextWithoutBreakingSessionBinding)
	{
		AppState source;
		Node* root = PopulateTree(source);
		const TreeMetadata expectedMetadata = MakeTestMetadata();
		source.context->treeMetadata = expectedMetadata;
		source.isQuitRequested = true;
		TreeContext* originalContext = source.context.get();

		AppState destination = std::move(source);

		SF_ASSERT_EQ(source.context == nullptr, true);
		SF_ASSERT_EQ(destination.context.get() == originalContext, true);
		SF_ASSERT_EQ(destination.isQuitRequested, true);
		SF_ASSERT_EQ(destination.HasActiveTree(), true);
		SF_ASSERT_EQ(destination.IsFreshStorage(), false);
		SF_ASSERT_EQ(destination.context->session.GetRoot() == root, true);
		AssertMetadataEquals(destination.context->treeMetadata, expectedMetadata);
		AssertSessionUsesContextStorage(destination);
	}
}
