#pragma once

#include "space_fossils/file_tree/model/default_constants.hxx"
#include "space_fossils/file_tree/scan/scan_input.hxx"
#include "space_fossils/file_tree/model/tree_pool_bundle.hxx"

#include <filesystem>

// Symlinks/junctions are represented as leaf nodes and are not traversed.
// Directories stopped by maxDepth become Pending.
// Inaccessible entries remain visible with AccessDenied and non-complete scan status.

namespace space_fossils::core::file_tree::scan {
	struct ScannerConfig
	{
		std::size_t nameBlockSize = DefaultNameBlockSize;
		std::size_t nodeBlockSize = DefaultNodeBlockSize;
	};

	class Scanner
	{
	public:
		explicit Scanner(ScannerConfig config = {});

		Scanner(const Scanner&) = delete;
		Scanner& operator=(const Scanner&) = delete;

		Scanner(Scanner&&) = delete;
		Scanner& operator=(Scanner&&) = delete;

		TreePoolBundle Scan(const ScanInput& input);

	private:
		TreePoolBundle CreateBundle() const;

		bool ScanDirectory(
			TreePoolBundle& bundle
			, const std::filesystem::directory_entry& directoryEntry
			, std::size_t currentDepth
			, std::size_t maxDepth
			, Node* parent
			, Node*& parentLastChild
		);

		bool AddChild(Node* parent, Node* child, Node*& lastChild);

	private:
		ScannerConfig config;
	};
}
