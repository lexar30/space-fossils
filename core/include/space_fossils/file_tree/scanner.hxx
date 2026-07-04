#pragma once

#include "space_fossils/file_tree/tree_pool_bundle.hxx"
#include "space_fossils/file_tree/scan_request.hxx"

#include <filesystem>

namespace space_fossils::core::file_tree {
	struct ScannerConfig
	{
		std::size_t nameBlockSize = sizeof(NativeChar) * 4096;
		std::size_t nodeBlockSize = sizeof(Node) * 1024;
	};

	class Scanner
	{
	public:
		explicit Scanner(ScannerConfig config = {});

		Scanner(const Scanner&) = delete;
		Scanner& operator=(const Scanner&) = delete;

		Scanner(Scanner&&) = delete;
		Scanner& operator=(Scanner&&) = delete;

		TreePoolBundle Scan(const ScanRequest& request);

	private:
		TreePoolBundle CreateBundle() const;

		bool ScanDirectory(
			TreePoolBundle& bundle
			, const std::filesystem::path& path
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