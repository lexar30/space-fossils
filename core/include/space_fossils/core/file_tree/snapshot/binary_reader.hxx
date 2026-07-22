#pragma once

#include "space_fossils/core/operation_timer.hxx"
#include "space_fossils/core/file_tree/model/tree_pool_bundle.hxx"
#include "space_fossils/core/file_tree/model/tree_metadata.hxx"

#include <cstdint>
#include <istream>
#include <optional>

namespace space_fossils::core::file_tree {
	struct Node;
}

namespace space_fossils::core::file_tree::snapshot {
	struct LoadedSnapshot
	{
		TreePoolBundle poolBundle;
		TreeMetadata treeMetadata;
	};

	class BinaryReader
	{
	public:
		std::optional<LoadedSnapshot> TryReadSnapshot(std::istream& in) const;
		MetricsDuration GetReadElapsedTime() const;

	private:
		bool TryReadAndCheckMetadata(std::istream& in, TreeMetadata& treeMetadata) const;
		bool TryReadBody(std::istream& in, TreePoolBundle& bundle) const;
		bool TryReadNodeRecord(std::istream& in, TreePoolBundle& bundle, Node*& node, std::uint64_t& childCount) const;

		bool TryReadBytes(std::istream& in, void* data, std::uint64_t size) const;
		bool TrySkipBytes(std::istream& in, std::uint64_t size) const;

		template<typename T>
		bool TryReadValue(std::istream& in, T& value) const;

	private:
		mutable MetricsDuration readElapsedTime = {};
	};
}
