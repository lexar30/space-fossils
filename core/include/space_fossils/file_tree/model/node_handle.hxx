#pragma once

#include "space_fossils/file_tree/model/types.hxx"

namespace space_fossils::core::file_tree {
	struct Node;

	struct NodeHandle
	{
		const Node* cachedNode = nullptr;
		NativeString nativePath;
		StorageVersion storageVersion = 0;
	};
}
