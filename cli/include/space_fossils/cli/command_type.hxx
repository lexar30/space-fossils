#pragma once

namespace space_fossils::cli {
	enum class CommandType
	{
		Undefined = 0

		, Help
		, Quit

		, SetUnits

		, Scan
		, Rescan

		, SaveSnapshot
		, LoadSnapshot

		, ShowTree
		, ListChildren
		, ShowInfo
		, ShowTop

		, ChangeDirectory
		, PrintWorkingDirectory

		, Reset
	};
}
