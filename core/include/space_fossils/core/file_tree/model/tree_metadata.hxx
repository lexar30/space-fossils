#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace space_fossils::core::file_tree {
	enum class TreeSource
	{
		Unknown
		, Scan
		, Snapshot
	};

	struct TreeMetadata
	{
		std::filesystem::path scanSourcePath = {};
		TreeSource treeSource = TreeSource::Unknown;
		std::string applicationVersion = {};
		std::uint64_t updatedAtUnixSeconds = 0;
	};
}
