#pragma once

#include "space_fossils/core/file_tree/model/types.hxx"

namespace space_fossils::core::file_tree {
	struct Node
	{
		NameRef name{};
		FileSize logicalSize = DefaultFileSize;

		EntryType entryType = EntryType::Unknown;
		EntryStatus entryStatus = EntryStatus::Unknown;
		EntryScanStatus scanStatus = EntryScanStatus::Unknown;

		Node* parent = nullptr;
		Node* firstChild = nullptr;
		Node* nextSibling = nullptr;
	};
}
