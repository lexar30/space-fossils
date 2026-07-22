#pragma once

#include "space_fossils/core/operation_timer.hxx"

#include <cstdint>
#include <ostream>

namespace space_fossils::core::file_tree {
	struct Node;
	struct TreeMetadata;
}

namespace space_fossils::core::file_tree::snapshot {
	class BinaryWriter
	{
	public:
		bool TryWriteSnapshot(std::ostream& out, const Node* root, const TreeMetadata& treeMetadata) const;
		MetricsDuration GetWriteElapsedTime() const;

	private:
		bool TryWriteMetadata(std::ostream& out, const TreeMetadata& treeMetadata) const;
		bool TryWriteBody(std::ostream& out, const Node* root) const;
		bool TryWriteNode(std::ostream& out, const Node* root) const;

		bool TryWriteBytes(std::ostream& out, const void* data, std::uint64_t size) const;

		template<typename T>
		bool TryWriteValue(std::ostream& out, T value) const;

	private:
		mutable MetricsDuration writeElapsedTime = {};
	};
}
