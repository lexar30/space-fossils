#pragma once

namespace space_fossils::cli {
	enum class CommandType
	{
		Undefined = 0
		, Help
		, Quit

		, Scan

		, Save
		, Load

		, Info

		, ChangeDirectory
		, GoUp
		, GoRoot
		, EnterSubByIndex
		, EnterSubByName

		, SelectSubByIndex
		, SelectSubFirst
		, SelectSubNext
		, SelectSubPrevious
	};
}
