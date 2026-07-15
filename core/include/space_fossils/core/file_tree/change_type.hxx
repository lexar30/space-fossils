#pragma once

namespace space_fossils::core::file_tree {
	enum class ChangeType
	{
		Unknown,
		AdoptRoot,
		Attach,
		Replace,
		Remove
	};
}
