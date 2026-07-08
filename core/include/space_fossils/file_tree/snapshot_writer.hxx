#pragma once

#include <array>
#include <cstdint>
#include <ostream>

namespace space_fossils::core::file_tree {
	struct Node;

	class SnapshotWriter
	{
	public:
		static constexpr std::array<char, 4> MagicBytes{ 'S', 'F', 'V', 'B' };

	public:
		bool TryWriteSnapshot(std::ostream& out, const Node* root) const;

	private:
		bool TryWriteMetadata(std::ostream& out) const;
		bool TryWriteBody(std::ostream& out, const Node* root) const;
		bool TryWriteNode(std::ostream& out, const Node* root) const;

		bool TryWriteBytes(std::ostream& out, const void* data, std::uint64_t size) const;

		template<typename T>
		bool TryWriteValue(std::ostream& out, T value) const;
	};
}