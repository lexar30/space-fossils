#include "space_fossils/file_tree/scan/scanner.hxx"
#include "space_fossils/file_tree/model/tree_pool_bundle.hxx"

#include <filesystem>
#include <memory>

namespace space_fossils::core::file_tree::scan {
	Scanner::Scanner(ScannerConfig config)
		: config(config)
	{
	}

	TreePoolBundle Scanner::CreateBundle() const
	{
		TreePoolBundle bundle;
		bundle.root = nullptr;
		bundle.createdNodesCount = 0;
		bundle.namePool = std::make_unique<NamePool>(config.nameBlockSize);
		bundle.nodePool = std::make_unique<NodePool>(config.nodeBlockSize);
		return bundle;
	}

	TreePoolBundle Scanner::Scan(const ScanInput& input)
	{
		TreePoolBundle bundle = CreateBundle();

		if (bundle.namePool->GetBlockSize() == 0
			|| bundle.nodePool->GetBlockSize() == 0
			) {
			return bundle;
		}

		std::error_code rootEc;
		std::filesystem::directory_entry rootEntry(input.path, rootEc);
		if (rootEc) {
			return bundle;
		}

		Node* rootLastChild = nullptr;
		ScanDirectory(
			bundle,
			rootEntry,
			0,
			input.maxDepth,
			nullptr,
			rootLastChild
		);

		return bundle;
	}

	bool Scanner::ScanDirectory(TreePoolBundle& bundle, const std::filesystem::directory_entry& directoryEntry, std::size_t currentDepth, std::size_t maxDepth, Node* parent, Node*& parentLastChild)
	{
		const auto& path = directoryEntry.path();
		bool isScannedCompletely = true;
		Node* node = bundle.nodePool->Create();
		if (node == nullptr) {
			return false;
		}

		if (bundle.root == nullptr) {
			bundle.root = node;
		}
		else {
			isScannedCompletely &= AddChild(parent, node, parentLastChild);
		}

		const auto name = path.filename().native();
		if (!name.empty()) {
			node->name = bundle.namePool->Store(name);
		}
		else {
			node->name = bundle.namePool->Store(path.native());
		}

		node->parent = parent;

		++bundle.createdNodesCount;

		std::error_code statusEc;
		std::filesystem::file_status status = directoryEntry.symlink_status(statusEc);

		if (statusEc) {
			if (status.type() == std::filesystem::file_type::not_found
				|| statusEc == std::errc::no_such_file_or_directory) {
				node->entryType = EntryType::Unknown;
				node->entryStatus = EntryStatus::NotFound;
				node->scanStatus = EntryScanStatus::Error;
				return false;
			}

			node->entryType = EntryType::Unknown;
			node->entryStatus = EntryStatus::Unknown;
			node->scanStatus = EntryScanStatus::Error;
			return false;
		}

		if (std::filesystem::is_symlink(status)) {
			node->entryType = EntryType::Symlink;
			node->entryStatus = EntryStatus::Accessible;
			node->scanStatus = EntryScanStatus::Complete;
			return true;
		}

		if (!std::filesystem::exists(status)) {
			node->entryType = EntryType::Unknown;
			node->entryStatus = EntryStatus::NotFound;
			node->scanStatus = EntryScanStatus::Error;
			return false;
		}

		if (std::filesystem::is_regular_file(status)) {
			node->entryType = EntryType::File;
			std::error_code sizeEc;
			node->logicalSize = directoryEntry.file_size(sizeEc);
			if (sizeEc) {
				node->logicalSize = DefaultFileSize;
				node->entryStatus = EntryStatus::AccessDenied;
				node->scanStatus = EntryScanStatus::Error;
				return false;
			}

			node->entryStatus = EntryStatus::Accessible;
			node->scanStatus = EntryScanStatus::Complete;

			for (Node* ancestor = parent; ancestor != nullptr; ancestor = ancestor->parent) {
				ancestor->logicalSize += node->logicalSize;
			}

			return true;
		}
		else if (std::filesystem::is_directory(status)) {
			node->entryType = EntryType::Directory;
		}
		else {
			node->entryType = EntryType::Other;
			node->entryStatus = EntryStatus::Accessible;
			node->scanStatus = EntryScanStatus::Complete;
			return true;
		}

		if (currentDepth >= maxDepth) {
			node->entryStatus = EntryStatus::Accessible;
			node->scanStatus = EntryScanStatus::Pending;
			return false;
		}

		std::error_code iterEc;
		std::filesystem::directory_iterator it(
			path
			, iterEc
		);

		if (iterEc) {
			node->entryStatus = EntryStatus::AccessDenied;
			node->scanStatus = EntryScanStatus::Partial;
			return false;
		}

		node->entryStatus = EntryStatus::Accessible;

		Node* childLastChild = nullptr;

		for (const auto& entry : it) {
			isScannedCompletely &= ScanDirectory(bundle, entry, currentDepth + 1, maxDepth, node, childLastChild);
		}

		node->scanStatus = isScannedCompletely ? EntryScanStatus::Complete : EntryScanStatus::Partial;

		return isScannedCompletely;
	}

	bool Scanner::AddChild(Node* parent, Node* child, Node*& lastChild)
	{
		if (parent == nullptr || child == nullptr) {
			return false;
		}

		if (lastChild == nullptr) {
			parent->firstChild = child;
		}
		else {
			lastChild->nextSibling = child;
		}

		lastChild = child;

		return true;
	}
}
