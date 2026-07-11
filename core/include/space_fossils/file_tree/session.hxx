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

		bool IsValid() const;

		const Storage& GetStorage() const;
		const Node* GetRoot() const;
		const Node* GetCurrentNode() const;
		NativeString GetCurrentNativePath() const;
		bool ResetToRoot();
		bool TrySelect(const Node* node);
		bool TrySelectChild(NativeStringView name);
		bool TrySelectChild(std::size_t index);
		bool TrySelectFromRoot(NativeStringView nativePath);
		bool TrySelectRelative(NativeStringView nativePath);
		bool TrySelectParent();
		const std::vector<const Node*>& GetAvailableChildren() const;
		bool HasTree() const;
		std::size_t GetFocusedChildIndex() const;
		bool TrySetFocusedChildIndex(std::size_t index);
		void MoveFocusedChildIndex(std::ptrdiff_t delta);

	private:
		void RefreshIfStale() const;
		void SelectKnownNode(const Node* node) const;
		void RefreshAvailableChildren() const;

	private:
		const Storage& storage;
		mutable const Node* currentNode = nullptr;
		mutable std::vector<const Node*> availableChildren;
		mutable NativeString currentNativePath;
		mutable StorageVersion knownStorageVersion = 0;
		mutable std::size_t focusedChildIndex = 0;
	};
}
