#include "space_fossils/core/file_tree/session/session_operations.hxx"

#include "space_fossils/core/file_tree/model/node.hxx"
#include "space_fossils/core/file_tree/query/tree_query.hxx"
#include "space_fossils/core/file_tree/session/session.hxx"

namespace space_fossils::core::file_tree {
	bool SessionOperations::TryResetToRoot(Session& session)
	{
		return session.TrySetCurrentNode(session.GetRoot());
	}

	bool SessionOperations::TryChangeDirectory(Session& session, const Node* target)
	{
		if (target == nullptr) {
			return false;
		}

		if (!TreeQuery::ContainsInSubtree(session.GetRoot(), target)) {
			return false;
		}

		if (target->entryType != EntryType::Directory) {
			return false;
		}

		return session.TrySetCurrentNode(target);
	}

	bool SessionOperations::TryChangeNode(Session& session, const Node* target)
	{
		if (target == nullptr) {
			return false;
		}

		if (!TreeQuery::ContainsInSubtree(session.GetRoot(), target)) {
			return false;
		}

		return session.TrySetCurrentNode(target);
	}

	bool SessionOperations::TrySelectParent(Session& session)
	{
		session.Sync();

		const Node* currentNode = session.GetCurrentNode();
		if (currentNode == nullptr || currentNode->parent == nullptr) {
			return false;
		}

		return session.TrySetCurrentNode(currentNode->parent);
	}

	bool SessionOperations::TrySelectChild(Session& session, std::size_t index)
	{
		session.Sync();

		const auto children = session.GetAvailableChildren();
		if (index >= children.size()) {
			return false;
		}

		return session.TrySetCurrentNode(children[index]);
	}

	bool SessionOperations::TrySelectChild(Session& session, NativeStringView name)
	{
		session.Sync();

		if (name.empty()) {
			return false;
		}

		const Node* foundChild = TreeQuery::FindChildByName(session.GetCurrentNode(), name);
		if (foundChild == nullptr) {
			return false;
		}

		return session.TrySetCurrentNode(foundChild);
	}

	bool SessionOperations::TryFocusChildByIndex(Session& session, std::size_t index)
	{
		return session.TrySetFocusedChildIndex(index);
	}

	bool SessionOperations::TryFocusChildByName(Session& session, NativeStringView name)
	{
		if (name.empty()) {
			return false;
		}

		const auto& children = session.GetAvailableChildren();
		for (std::size_t childIndex = 0; childIndex < children.size(); ++childIndex) {
			if (TreeQuery::NodeNameEquals(children[childIndex], name)) {
				return session.TrySetFocusedChildIndex(childIndex);
			}
		}

		return false;
	}

	bool SessionOperations::TryFocusNextChild(Session& session)
	{
		return TryMoveFocusedChildIndexByDelta(session, 1);
	}

	bool SessionOperations::TryFocusPreviousChild(Session& session)
	{
		return TryMoveFocusedChildIndexByDelta(session, -1);
	}

	bool SessionOperations::TryMoveFocusedChildIndexByDelta(Session& session, std::ptrdiff_t delta)
	{
		session.Sync();

		const auto& children = session.GetAvailableChildren();
		if (delta == 0 || children.empty()) {
			return false;
		}

		const std::size_t limit = children.size();

		const std::size_t focusedIndex = session.GetFocusedChildIndex();
		if (delta > 0) {
			const std::size_t shift = static_cast<std::size_t>(delta) % limit;
			if (shift >= limit - focusedIndex) {
				const std::size_t focusedChildIndex = shift - (limit - focusedIndex);
				return session.TrySetFocusedChildIndex(focusedChildIndex);
			}
			else {
				const std::size_t focusedChildIndex = focusedIndex + shift;
				return session.TrySetFocusedChildIndex(focusedChildIndex);
			}
		}
		else {
			const std::size_t shift = (static_cast<std::size_t>(-(delta + 1)) + 1) % limit;
			if (shift > focusedIndex) {
				const std::size_t focusedChildIndex = limit - (shift - focusedIndex);
				return session.TrySetFocusedChildIndex(focusedChildIndex);
			}
			else {
				const std::size_t focusedChildIndex = focusedIndex - shift;
				return session.TrySetFocusedChildIndex(focusedChildIndex);
			}
		}
	}
}
