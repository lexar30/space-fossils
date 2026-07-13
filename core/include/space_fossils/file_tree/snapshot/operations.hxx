#pragma once

namespace space_fossils::core::file_tree {
	class Storage;
}

namespace space_fossils::core::file_tree::snapshot {
	class Operations
	{
	public:
		bool TryMakeSnapshot(Storage& storage);
		bool TryLoadSnapshot(Storage& storage);
	};
}
