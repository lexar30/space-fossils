#pragma once

#include "space_fossils/operation_timer.hxx"

#include <filesystem>

namespace space_fossils::core::file_tree {
	class Storage;
}

namespace space_fossils::core::file_tree::snapshot {
	struct LoadedSnapshotSummary
	{
		MetricsDuration loadDuration = {};
		bool isSuccessful = false;
	};

	struct SavedSnapshotSummary
	{
		MetricsDuration saveDuration = {};
		bool isSuccessful = false;
	};

	class Operations
	{
	public:
		SavedSnapshotSummary TrySaveSnapshot(const std::filesystem::path& outPath, const Storage& storage);
		LoadedSnapshotSummary TryLoadSnapshot(const std::filesystem::path& inPath, Storage& storage);
	};
}
