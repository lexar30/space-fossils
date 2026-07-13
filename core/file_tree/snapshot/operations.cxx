#include "space_fossils/file_tree/snapshot/operations.hxx"

#include "space_fossils/file_tree/snapshot/reader.hxx"
#include "space_fossils/file_tree/snapshot/writer.hxx"
#include "space_fossils/file_tree/storage/storage.hxx"
#include "space_fossils/file_tree/model/tree_pool_bundle.hxx"

#include <fstream>
#include <optional>
#include <utility>

namespace space_fossils::core::file_tree::snapshot {
	SavedSnapshotSummary Operations::TrySaveSnapshot(const std::filesystem::path& outPath, const Storage& storage)
	{
		// TODO: may save unfinished file
		SavedSnapshotSummary summary;
		snapshot::Writer writer;

		std::ofstream out(outPath, std::ios::binary);

		const bool written = writer.TryWriteSnapshot(out, storage.GetRoot());
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

		snapshot::Reader reader;

		std::ifstream in(inPath, std::ios::binary);

		std::optional<TreePoolBundle> bundle = reader.TryReadSnapshot(in);
		summary.loadDuration = reader.GetReadElapsedTime();
		if (!bundle.has_value()) {
			return summary;
		}

		std::optional<AppliedChange> change = storage.TryAdoptRoot(std::move(bundle.value()));
		if (!change.has_value()) {
			return summary;
		}

		summary.isSuccessful = true;

		return summary;
	}
}
