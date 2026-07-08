#pragma once

#include "space_fossils/file_tree/tree_pool_bundle.hxx"

#include <cstdint>
#include <istream>
#include <optional>

namespace space_fossils::core::file_tree {
	struct Node;

	class SnapshotReader
	{
	public:
		std::optional<TreePoolBundle> TryReadSnapshot(std::istream& in) const;

	private:
		bool TryReadAndCheckMetadata(std::istream& in) const;
		bool TryReadBody(std::istream& in, TreePoolBundle& bundle) const;
		bool TryReadNodeRecord(std::istream& in, TreePoolBundle& bundle, Node*& node, std::uint64_t& childCount) const;

		bool TryReadBytes(std::istream& in, void* data, std::uint64_t size) const;
		bool TrySkipBytes(std::istream& in, std::uint64_t size) const;

		template<typename T>
		bool TryReadValue(std::istream& in, T& value) const;
	};
}