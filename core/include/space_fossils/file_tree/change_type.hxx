#pragma once

namespace space_fossils::core::file_tree {
	enum class IncomingChangeType
	{
		Unknown,
		AdoptRoot,
		Attach,
		Replace,
		Remove
	};
}
