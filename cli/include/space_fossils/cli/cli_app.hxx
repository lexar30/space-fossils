#pragma once

#include "space_fossils/cli/app_state.hxx"

#include <iosfwd>

namespace space_fossils::cli {
	class CliApp
	{
	public:
		void Run(std::istream& input, std::ostream& output);

	private:
		AppState appState;
	};
}
