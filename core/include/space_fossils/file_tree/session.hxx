#pragma once

#include "space_fossils/file_tree/types.hxx"

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

		const Storage& GetStorage() const;
		const Node* GetRoot() const;
		const Node* GetCurrentNode();
		NativeString GetCurrentNativePath();
		bool ResetToRoot();
		bool TrySelect(const Node* node);
		bool TrySelectChild(NativeStringView name);
		bool TrySelectChild(std::size_t index);
		bool TrySelectFromRoot(NativeStringView nativePath);
		bool TrySelectRelative(NativeStringView nativePath);
		bool TrySelectParent();
		const std::vector<const Node*>& GetAvailableChildren();
		bool HasTree();
		std::size_t GetFocusedChildIndex();
		bool TrySetFocusedChildIndex(std::size_t index);
		void MoveFocusedChildIndex(std::ptrdiff_t delta);

	private:
		void RefreshIfStale();
		void SelectKnownNode(const Node* node);
		void RefreshAvailableChildren();

	private:
		const Storage& storage;
		const Node* currentNode = nullptr;
		std::vector<const Node*> availableChildren;
		NativeString currentNativePath;
		StorageVersion knownStorageVersion = 0;
		std::size_t focusedChildIndex = 0;
	};
}
