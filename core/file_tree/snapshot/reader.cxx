#include "space_fossils/core/file_tree/snapshot/reader.hxx"

#include "space_fossils/core/file_tree/model/default_constants.hxx"
#include "space_fossils/core/file_tree/memory/name_pool.hxx"
#include "space_fossils/core/file_tree/model/node.hxx"
#include "space_fossils/core/file_tree/memory/node_pool.hxx"
#include "space_fossils/core/file_tree/model/types.hxx"
#include "space_fossils/core/file_tree/model/tree_pool_bundle.hxx"
#include "space_fossils/core/file_tree/snapshot/writer.hxx"
#include "space_fossils/core/version.hxx"

#include <array>
#include <cstdint>
#include <ios>
#include <limits>
#include <memory>
#include <vector>

namespace space_fossils::core::file_tree::snapshot {
	namespace {
		struct ParentFrame
		{
			Node* parent = nullptr;
			std::uint64_t remainingChildCount = 0;
			Node* lastChild = nullptr;
		};
	}

	std::optional<TreePoolBundle> Reader::TryReadSnapshot(std::istream& in) const
	{
		OperationTimer timer;

		timer.Start();
		TreePoolBundle bundle;

		bundle.root = nullptr;
		bundle.createdNodesCount = 0;
		bundle.namePool = std::make_unique<NamePool>(DefaultNameBlockSize);
		bundle.nodePool = std::make_unique<NodePool>(DefaultNodeBlockSize);

		if (!TryReadAndCheckMetadata(in)) {
			timer.Stop();
			readElapsedTime = timer.Elapsed();
			return std::nullopt;
		}

		if (!TryReadBody(in, bundle)) {
			timer.Stop();
			readElapsedTime = timer.Elapsed();
			return std::nullopt;
		}

		if (in.peek() != std::char_traits<char>::eof()) {
			timer.Stop();
			readElapsedTime = timer.Elapsed();
			return std::nullopt;
		}
		timer.Stop();
		readElapsedTime = timer.Elapsed();

		return bundle;
	}

	MetricsDuration Reader::GetReadElapsedTime() const
	{
		return readElapsedTime;
	}

	bool Reader::TryReadAndCheckMetadata(std::istream& in) const
	{
		// TODO: use metadata later & split read/check(?)
		std::array<char, 4> magicBytes{};
		if (!TryReadBytes(in, magicBytes.data(), Writer::MagicBytes.size())) {
			return false;
		}
		if (magicBytes != Writer::MagicBytes) {
			return false;
		}

		std::uint32_t snapshotFormatVersion = 0;
		if (!TryReadValue(in, snapshotFormatVersion)) {
			return false;
		}
		if (snapshotFormatVersion != version::SnapshotFormatVersion) {
			return false;
		}

		std::uint64_t applicationVersionLength = 0;
		if (!TryReadValue(in, applicationVersionLength)) {
			return false;
		}

		if (!TrySkipBytes(in, applicationVersionLength)) {
			return false;
		}

		std::uint64_t secondsSinceEpoch = 0;
		if (!TryReadValue(in, secondsSinceEpoch)) {
			return false;
		}

		std::uint8_t nativeCharSize = 0;
		if (!TryReadValue(in, nativeCharSize)) {
			return false;
		}
		if (nativeCharSize != sizeof(NativeChar)) {
			return false;
		}

		return in.good();
	}

	bool Reader::TryReadBody(std::istream& in, TreePoolBundle& bundle) const
	{
		Node* root = nullptr;
		std::uint64_t rootChildCount = 0;
		if (!TryReadNodeRecord(in, bundle, root, rootChildCount)) {
			return false;
		}

		bundle.root = root;

		std::vector<ParentFrame> stack;
		stack.push_back(ParentFrame{
			root,
			rootChildCount,
			nullptr
			});

		while (!stack.empty()) {
			ParentFrame& frame = stack.back();
			if (frame.remainingChildCount == 0) {
				stack.pop_back();
				continue;
			}

			Node* child = nullptr;
			std::uint64_t childChildrenCount = 0;
			if (!TryReadNodeRecord(in, bundle, child, childChildrenCount)) {
				return false;
			}

			if (child == nullptr) {
				return false;
			}

			child->parent = frame.parent;
			if (frame.lastChild == nullptr) {
				frame.parent->firstChild = child;
			}
			else {
				frame.lastChild->nextSibling = child;
			}

			frame.lastChild = child;
			--frame.remainingChildCount;

			if (childChildrenCount > 0) {
				stack.push_back(ParentFrame{
					child,
					childChildrenCount,
					nullptr
					});
			}
		}

		return in.good();
	}

	bool Reader::TryReadNodeRecord(std::istream& in, TreePoolBundle& bundle, Node*& node, std::uint64_t& childCount) const
	{
		node = bundle.nodePool->Create();
		if (node == nullptr) {
			return false;
		}
		++bundle.createdNodesCount;

		std::uint64_t nameByteLength = 0;
		if (!TryReadValue(in, nameByteLength)) {
			return false;
		}

		if (nameByteLength % sizeof(NativeChar) != 0) {
			return false;
		}

		if (nameByteLength > 0) {
			NativeString name;
			const std::uint64_t characterCount =
				nameByteLength / sizeof(NativeChar);

			if (characterCount >
				std::numeric_limits<std::uint32_t>::max()) {
				return false;
			}

			name.resize(static_cast<std::size_t>(nameByteLength / sizeof(NativeChar)));

			if (!TryReadBytes(in, name.data(), nameByteLength)) {
				return false;
			}
			node->name = bundle.namePool->Store(name);

			if (!name.empty() && node->name.data == nullptr) {
				return false;
			}
		}

		std::uint64_t logicalSize = 0;
		if (!TryReadValue(in, logicalSize)) {
			return false;
		}
		node->logicalSize = static_cast<FileSize>(logicalSize);

		std::uint8_t entryType = 0;
		if (!TryReadValue(in, entryType)) {
			return false;
		}
		if (entryType > static_cast<std::uint8_t>(EntryType::Other)) {
			return false;
		}
		node->entryType = static_cast<EntryType>(entryType);

		std::uint8_t entryStatus = 0;
		if (!TryReadValue(in, entryStatus)) {
			return false;
		}
		if (entryStatus >
			static_cast<std::uint8_t>(EntryStatus::Error)) {
			return false;
		}
		node->entryStatus = static_cast<EntryStatus>(entryStatus);

		std::uint8_t scanStatus = 0;
		if (!TryReadValue(in, scanStatus)) {
			return false;
		}
		if (scanStatus >
			static_cast<std::uint8_t>(EntryScanStatus::Error)) {
			return false;
		}
		node->scanStatus = static_cast<EntryScanStatus>(scanStatus);

		childCount = 0;
		if (!TryReadValue(in, childCount)) {
			return false;
		}

		return true;
	}

	bool Reader::TryReadBytes(std::istream& in, void* data, std::uint64_t size) const
	{
		if (data == nullptr && size != 0) {
			return false;
		}

		if (size > static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max())) {
			return false;
		}

		in.read(static_cast<char*>(data), static_cast<std::streamsize>(size));

		return in.good();
	}

	bool Reader::TrySkipBytes(std::istream& in, std::uint64_t size) const
	{
		std::array<char, 256> buffer{};
		std::uint64_t remainingBytes = size;

		while (remainingBytes > 0) {
			const std::uint64_t chunkSize = remainingBytes < buffer.size()
				? remainingBytes
				: static_cast<std::uint64_t>(buffer.size());

			if (!TryReadBytes(in, buffer.data(), chunkSize)) {
				return false;
			}

			remainingBytes -= chunkSize;
		}

		return true;
	}

	template<typename T>
	bool Reader::TryReadValue(std::istream& in, T& value) const
	{
		return TryReadBytes(in, &value, sizeof(T));
	}
}
