#include "space_fossils/core/file_tree/snapshot/operations.hxx"

#include "space_fossils/core/file_tree/snapshot/binary_reader.hxx"
#include "space_fossils/core/file_tree/snapshot/binary_writer.hxx"
#include "space_fossils/core/file_tree/storage/storage.hxx"
#include "space_fossils/core/file_tree/model/tree_pool_bundle.hxx"

#include <fstream>
#include <optional>
#include <utility>

namespace space_fossils::core::file_tree::snapshot {
	SavedSnapshotSummary Operations::TrySaveSnapshot(const std::filesystem::path& outPath, const Storage& storage, const TreeMetadata& treeMetadata)
	{
		// TODO: may save unfinished file
		SavedSnapshotSummary summary;
		snapshot::BinaryWriter writer;

		std::ofstream out(outPath, std::ios::binary);

		const bool written = writer.TryWriteSnapshot(out, storage.GetRoot(), treeMetadata);
		summary.saveDuration = writer.GetWriteElapsedTime();
		out.close();

		summary.isSuccessful = written && !out.fail();
		return summary;
	}

	LoadedSnapshotSummary Operations::TryLoadSnapshot(const std::filesystem::path& inPath, Storage& storage)
	{
		LoadedSnapshotSummary summary;

		if (storage.GetRoot() != nullptr || storage.GetVersion() != 0) {
			return summary;
		}

		snapshot::BinaryReader reader;

		std::ifstream in(inPath, std::ios::binary);

		std::optional<LoadedSnapshot> snapshotData = reader.TryReadSnapshot(in);
		summary.loadDuration = reader.GetReadElapsedTime();
		if (!snapshotData.has_value()) {
			return summary;
		}

		std::optional<AppliedChange> change = storage.TryAdoptRoot(std::move(snapshotData.value().poolBundle));
		if (!change.has_value()) {
			return summary;
		}

		summary.treeMetadata = std::move(snapshotData.value().treeMetadata);
		summary.isSuccessful = true;

		return summary;
	}
}
