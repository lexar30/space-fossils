#include "space_fossils/core/file_tree/snapshot/writer.hxx"

#include "space_fossils/core/file_tree/model/node.hxx"
#include "space_fossils/core/file_tree/model/types.hxx"
#include "space_fossils/core/version.hxx"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <limits>
#include <ranges>
#include <stack>
#include <vector>

namespace space_fossils::core::file_tree::snapshot {
	bool Writer::TryWriteSnapshot(std::ostream& out, const Node* root) const
	{
		OperationTimer timer;

		timer.Start();
		if (root == nullptr) {
			timer.Stop();
			writeElapsedTime = timer.Elapsed();
			return false;
		}

		const bool isSuccessful = TryWriteMetadata(out) && TryWriteBody(out, root);
		timer.Stop();
		writeElapsedTime = timer.Elapsed();

		return isSuccessful;
	}

	MetricsDuration Writer::GetWriteElapsedTime() const
	{
		return writeElapsedTime;
	}

	bool Writer::TryWriteBody(std::ostream& out, const Node* root) const
	{
		return TryWriteNode(out, root);
	}

	bool Writer::TryWriteMetadata(std::ostream& out) const
	{
		const auto epochDuration = std::chrono::system_clock::now().time_since_epoch();
		const std::uint64_t secondsSinceEpoch = std::chrono::duration_cast<std::chrono::seconds>(epochDuration).count();

		const std::uint64_t applicationVersionLength = std::strlen(version::ApplicationVersion);
		const std::uint8_t nativeCharSize = sizeof(NativeChar);

		return TryWriteBytes(out, MagicBytes.data(), MagicBytes.size())
			&& TryWriteValue(out, version::SnapshotFormatVersion)
			&& TryWriteValue(out, applicationVersionLength)
			&& TryWriteBytes(out, version::ApplicationVersion, applicationVersionLength)
			&& TryWriteValue(out, secondsSinceEpoch)
			&& TryWriteValue(out, nativeCharSize)
			;
	}

	bool Writer::TryWriteNode(std::ostream& out, const Node* root) const
	{
		std::stack<const Node*> nodes;
		std::vector<const Node*> children;
		children.reserve(32);

		nodes.push(root);

		while (!nodes.empty()) {
			const Node* node = nodes.top();
			nodes.pop();

			if (node == nullptr) {
				return false;
			}

			const std::uint64_t nameByteLength = static_cast<std::uint64_t>(node->name.length) * sizeof(NativeChar);
			if (nameByteLength > 0 && node->name.data == nullptr) {
				return false;
			}

			if (!TryWriteValue(out, nameByteLength)) {
				return false;
			}
			if (nameByteLength > 0) {
				if (!TryWriteBytes(out, node->name.data, nameByteLength)) {
					return false;
				}
			}
			if (!TryWriteValue(out, static_cast<std::uint64_t>(node->logicalSize))) {
				return false;
			}
			if (!TryWriteValue(out, static_cast<std::uint8_t>(node->entryType))) {
				return false;
			}
			if (!TryWriteValue(out, static_cast<std::uint8_t>(node->entryStatus))) {
				return false;
			}
			if (!TryWriteValue(out, static_cast<std::uint8_t>(node->scanStatus))) {
				return false;
			}

			std::uint64_t childCount = 0;
			Node* child = node->firstChild;
			while (child != nullptr) {
				++childCount;
				children.push_back(child);
				child = child->nextSibling;
			}

			if (!TryWriteValue(out, childCount)) {
				return false;
			}

			for (const auto child : children | std::views::reverse) {
				nodes.push(child);
			}

			children.clear();
		}

		return out.good();
	}

	bool Writer::TryWriteBytes(std::ostream& out, const void* data, std::uint64_t size) const
	{
		if (data == nullptr && size != 0) {
			return false;
		}

		if (size > static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max())) {
			return false;
		}

		out.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));

		return out.good();
	}

	template<typename T>
	bool Writer::TryWriteValue(std::ostream& out, T value) const
	{
		return TryWriteBytes(out, &value, static_cast<std::uint64_t>(sizeof(T)));
	}
}