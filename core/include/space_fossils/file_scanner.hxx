#pragma once

#include <filesystem>
#include <memory>

#include "space_fossils/file_tree.hxx"


namespace space_fossils::core {

	class FileTree;

	class FileScanner
	{
	public:
		FileTree Scan(const std::filesystem::path& path);
		void ScanThrough(FileTreeNode& fileTreeNode, const std::filesystem::path& path);


	};
}
