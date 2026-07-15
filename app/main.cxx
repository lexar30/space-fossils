#include <space_fossils/cli/cli_app.hxx>

#include <iostream>

#if defined(_WIN32)
int wmain(int argc, wchar_t* argv[])
#else
int main(int argc, char* argv[])
#endif
{
	space_fossils::cli::CliApp cliApp;
	cliApp.Run(std::cin, std::cout);

	return 0;
}
