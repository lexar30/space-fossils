#include "space_fossils/file_scanner.hxx"
#include "space_fossils/file_tree.hxx"
#include "space_fossils/file_tree_node.hxx"

#include <cassert>

namespace space_fossils::core {

	void FileScanner::ScanThrough(FileTreeNode& fileTreeNode, const std::filesystem::path& path)
	{
		for (const auto& it : std::filesystem::directory_iterator(path)) {
			if (it.is_regular_file()) {
				fileTreeNode.AddFile(it.path().filename().string(), it.file_size());
			}
			else if(it.is_directory()) {
				FileTreeNode& dir = fileTreeNode.AddDirectory(it.path().filename().string());
				ScanThrough(dir, it.path());
			}
		}
	}

	FileTree FileScanner::Scan(const std::filesystem::path& path)
	{
		FileTree fileTree;

		if (!std::filesystem::exists(path)) {
			return fileTree;
		}
		else if (std::filesystem::is_regular_file(path)) {
			fileTree.CreateRootFile(path.filename().string(), std::filesystem::file_size(path));
		}
		else if(std::filesystem::is_directory(path))
		{
			FileTreeNode& root = fileTree.CreateRootDirectory(path.filename().string());
			ScanThrough(root, path);
		}

		fileTree.RecalculateSizeRecursive();

		return fileTree;
	}

}