#pragma once

#include "space_fossils/core/file_tree/model/types.hxx"

#include <cstddef>

namespace space_fossils::core::file_tree {
	class Session;
	struct Node;

	class SessionOperations
	{
	public:
		bool TryResetToRoot(Session& session);
		bool TryChangeDirectory(Session& session, const Node* target);
		bool TryChangeNode(Session& session, const Node* target);
		bool TrySelectParent(Session& session);
		bool TrySelectChild(Session& session, std::size_t index);
		bool TrySelectChild(Session& session, NativeStringView name);

		bool TryFocusChildByIndex(Session& session, std::size_t index);
		bool TryFocusChildByName(Session& session, NativeStringView name);
		bool TryFocusNextChild(Session& session);
		bool TryFocusPreviousChild(Session& session);
		bool TryMoveFocusedChildIndexByDelta(Session& session, std::ptrdiff_t delta);
	};
}
