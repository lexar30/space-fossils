#include "cli.hxx"

#if defined(_WIN32)
int wmain(int argc, wchar_t* argv[])
#else
int main(int argc, char* argv[])
#endif
{
	return space_fossils::app::RunCli(argc, argv);
}
