#pragma once

#include "space_fossils/core/file_tree/model/types.hxx"

#include <cstddef>
#include <vector>

namespace space_fossils::core::file_tree {
	struct Node;
	class Storage;

	class Session
	{
	public:
		explicit Session(const Storage& storage);

		bool IsValid();
		void Sync();

		const Storage& GetStorage() const;
		const Node* GetRoot() const;
		const Node* GetCurrentNode();
		NativeString GetCurrentNativePath();
		bool TrySetCurrentNode(const Node* node);
		const std::vector<const Node*>& GetAvailableChildren();
		bool HasTree();
		std::size_t GetFocusedChildIndex();
		bool TrySetFocusedChildIndex(std::size_t index);

	private:
		void SelectKnownNode(const Node* node);
		void RefreshAvailableChildren();

	private:
		const Storage& storage;
		const Node* currentNode = nullptr;
		NativeString currentNativePath;
		StorageVersion knownStorageVersion = 0;
		std::vector<const Node*> availableChildren;
		std::size_t focusedChildIndex = 0;
	};
}
