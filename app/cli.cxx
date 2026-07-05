#include "cli.hxx"

#include "space_fossils/file_tree/node.hxx"
#include "space_fossils/file_tree/scan_coordinator.hxx"
#include "space_fossils/file_tree/storage.hxx"
#include "space_fossils/file_tree/text_report_writer.hxx"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace space_fossils::app {
	namespace file_tree = space_fossils::core::file_tree;
	using CliString = std::basic_string<CliChar>;
	using CliStringView = std::basic_string_view<CliChar>;

	struct ScanCommandOptions
	{
		std::filesystem::path inputPath;
		std::filesystem::path treeOutputPath;
		std::size_t depth = 1;
	};

	static void PrintUsage(std::ostream& out)
	{
		out << "Usage:\n";
		out << "  space_fossils_app scan --input <path> [--depth <N>] --tree <output_path>\n";
	}

	static CliStringView MakeCliStringView(const CliChar* value)
	{
		if (value == nullptr) {
			return {};
		}

		return CliStringView(value);
	}

	static bool EqualsAscii(CliStringView value, const char* ascii)
	{
		std::size_t index = 0;
		for (; index < value.size() && ascii[index] != '\0'; ++index) {
			if (value[index] != static_cast<CliChar>(ascii[index])) {
				return false;
			}
		}

		return index == value.size() && ascii[index] == '\0';
	}

	static void WriteCliValue(std::ostream& out, CliStringView value)
	{
		const std::filesystem::path path{ CliString(value) };
		const auto utf8 = path.u8string();

		out.write(
			reinterpret_cast<const char*>(utf8.data()),
			static_cast<std::streamsize>(utf8.size())
		);
	}

	static bool ParseDepth(CliStringView value, std::size_t& depth)
	{
		if (value.empty()) {
			return false;
		}

		std::size_t parsedDepth = 0;
		for (CliChar ch : value) {
			if (ch < static_cast<CliChar>('0') || ch > static_cast<CliChar>('9')) {
				return false;
			}

			const std::size_t digit = static_cast<std::size_t>(ch - static_cast<CliChar>('0'));
			if (parsedDepth > (std::numeric_limits<std::size_t>::max() - digit) / 10) {
				return false;
			}

			parsedDepth = parsedDepth * 10 + digit;
		}

		depth = parsedDepth;
		return true;
	}

	static bool ReadOptionValue(int argc, CliChar* argv[], int& index, const char* option, CliStringView& value)
	{
		if (index + 1 >= argc) {
			std::cerr << "Missing value for " << option << ".\n";
			return false;
		}

		++index;
		value = MakeCliStringView(argv[index]);
		return true;
	}

	static bool ParseScanCommand(int argc, CliChar* argv[], ScanCommandOptions& options)
	{
		for (int index = 2; index < argc; ++index) {
			const CliStringView argument = MakeCliStringView(argv[index]);
			CliStringView value;

			if (EqualsAscii(argument, "--input")) {
				if (!ReadOptionValue(argc, argv, index, "--input", value)) {
					return false;
				}

				options.inputPath = std::filesystem::path(CliString(value));
			}
			else if (EqualsAscii(argument, "--tree")) {
				if (!ReadOptionValue(argc, argv, index, "--tree", value)) {
					return false;
				}

				options.treeOutputPath = std::filesystem::path(CliString(value));
			}
			else if (EqualsAscii(argument, "--depth")) {
				if (!ReadOptionValue(argc, argv, index, "--depth", value)) {
					return false;
				}

				if (!ParseDepth(value, options.depth)) {
					std::cerr << "Invalid depth: ";
					WriteCliValue(std::cerr, value);
					std::cerr << ".\n";
					return false;
				}
			}
			else {
				std::cerr << "Unknown argument: ";
				WriteCliValue(std::cerr, argument);
				std::cerr << ".\n";
				return false;
			}
		}

		if (options.inputPath.empty()) {
			std::cerr << "Missing required option --input.\n";
			return false;
		}

		if (options.treeOutputPath.empty()) {
			std::cerr << "Missing required option --tree.\n";
			return false;
		}

		return true;
	}

	static int RunScanCommand(const ScanCommandOptions& options)
	{
		file_tree::Storage storage;

		file_tree::ScanCoordinatorConfig config;
		config.rootPath = options.inputPath;
		config.defaultScanDepth = options.depth;

		file_tree::ScanCoordinator coordinator(storage, std::move(config));
		coordinator.ScheduleRootScan(options.depth);

		while (coordinator.ProcessNext().has_value()) {
		}

		const file_tree::Node* root = storage.GetRoot();
		if (root == nullptr) {
			std::cerr << "Scan did not produce a root node.\n";
			return 2;
		}

		std::ofstream treeFile(options.treeOutputPath, std::ios::binary);
		if (!treeFile) {
			std::cerr << "Failed to open tree output file: " << options.treeOutputPath << ".\n";
			return 3;
		}

		file_tree::TextReportWriter writer;
		if (!writer.WriteTreeReport(treeFile, *root)) {
			std::cerr << "Failed to write tree report: " << options.treeOutputPath << ".\n";
			return 4;
		}

		return 0;
	}

	int RunCli(int argc, CliChar* argv[])
	{
		if (argc < 2) {
			PrintUsage(std::cerr);
			return 1;
		}

		const CliStringView command = MakeCliStringView(argv[1]);
		if (!EqualsAscii(command, "scan")) {
			std::cerr << "Unknown command: ";
			WriteCliValue(std::cerr, command);
			std::cerr << ".\n";
			PrintUsage(std::cerr);
			return 1;
		}

		ScanCommandOptions options;
		if (!ParseScanCommand(argc, argv, options)) {
			PrintUsage(std::cerr);
			return 1;
		}

		return RunScanCommand(options);
	}
}