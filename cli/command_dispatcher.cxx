#include "space_fossils/cli/command_dispatcher.hxx"

#include "space_fossils/cli/command_type.hxx"
#include "space_fossils/cli/command_spec.hxx"
#include "space_fossils/core/file_tree/model/node.hxx"
#include "space_fossils/core/file_tree/query/tree_query.hxx"
#include "space_fossils/core/file_tree/report/text_writer.hxx"
#include "space_fossils/core/file_tree/scan/coordinator.hxx"
#include "space_fossils/core/file_tree/scan/operations.hxx"
#include "space_fossils/core/file_tree/scan/scan_input.hxx"
#include "space_fossils/core/file_tree/session/session_operations.hxx"
#include "space_fossils/core/file_tree/snapshot/operations.hxx"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string_view>

namespace space_fossils::cli {
	// TODO: temp
	namespace {
		using namespace space_fossils::core;
		using namespace space_fossils::core::file_tree;

		std::filesystem::path MakePath(std::string_view value)
		{
			return std::filesystem::u8path(value.begin(), value.end());
		}

		std::string ToUtf8(NativeStringView value)
		{
			const std::u8string utf8 = std::filesystem::path(value).u8string();
			return std::string(reinterpret_cast<const char*>(utf8.data()), utf8.size());
		}

		std::string ToUtf8(NameRef value)
		{
			return ToUtf8(ToStringView(value));
		}

		long long ToMilliseconds(MetricsDuration value)
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(value).count();
		}

		std::size_t GetTextWidth(std::string_view value)
		{
			std::size_t width = 0;
			for (unsigned char symbol : value) {
				if ((symbol & 0xc0) != 0x80) {
					++width;
				}
			}
			return width;
		}

		void WritePaddedRight(std::ostream& output, std::string_view value, std::size_t width)
		{
			output << value;
			const std::size_t valueWidth = GetTextWidth(value);
			if (valueWidth < width) {
				output << std::string(width - valueWidth, ' ');
			}
		}

		void WritePaddedLeft(std::ostream& output, std::string_view value, std::size_t width)
		{
			const std::size_t valueWidth = GetTextWidth(value);
			if (valueWidth < width) {
				output << std::string(width - valueWidth, ' ');
			}
			output << value;
		}

		std::string MakeCommandLabel(const CommandSpec& spec)
		{
			std::string label(spec.usage);
			if (!spec.shortName.empty()) {
				label += " (";
				label += spec.shortName;
				label += ')';
			}
			return label;
		}

		struct NodeLine
		{
			std::string name;
			std::string size;
			std::string_view type;
		};

		std::string MakeNodeTable(
			const std::vector<const Node*>& nodes,
			FileSizeUnitSystem unitsType)
		{
			std::vector<NodeLine> lines;
			lines.reserve(nodes.size());
			std::size_t nameWidth = 0;
			std::size_t sizeWidth = 0;

			for (const Node* node : nodes) {
				NodeLine line{
					ToUtf8(node->name),
					FormatFileSize(node->logicalSize, unitsType),
					ToString(node->entryType)
				};
				nameWidth = std::max(nameWidth, GetTextWidth(line.name));
				sizeWidth = std::max(sizeWidth, GetTextWidth(line.size));
				lines.push_back(std::move(line));
			}

			std::ostringstream output;
			for (const NodeLine& line : lines) {
				WritePaddedRight(output, line.name, nameWidth);
				output << "  ";
				WritePaddedLeft(output, line.size, sizeWidth);
				output << "  " << line.type << '\n';
			}
			return output.str();
		}

		void WriteInfoLine(std::ostream& output, std::string_view label, std::string_view value)
		{
			constexpr std::size_t LabelWidth = std::string_view("Scan status").size();
			WritePaddedRight(output, label, LabelWidth);
			output << ": " << value << '\n';
		}
	}

	CommandResult CommandDispatcher::Dispatch(const ParsedCommand& parsedCommand, AppState& appState)
	{
		if (parsedCommand.type == CommandType::Undefined) {
			return ExecuteUndefined();
		}

		const CommandSpec* spec = TryFindCommandSpecByType(parsedCommand.type);
		if (spec == nullptr) {
			return ExecuteUndefined();
		}

		if (parsedCommand.arguments.size() != spec->argsCount) {
			return {
				CommandStatus::InvalidArgs,
				"Unexpected arguments count"
			};
		}

		switch (parsedCommand.type)
		{
		case CommandType::Help:
			return ExecuteHelp(appState);

		case CommandType::Quit:
			return ExecuteQuit(appState);

		case CommandType::SetUnits:
			return ExecuteSetUnits(parsedCommand.arguments, appState);

		case CommandType::LoadSnapshot:
			return ExecuteLoadSnapshot(parsedCommand.arguments, appState);

		case CommandType::Reset:
			return ExecuteReset(appState);

		default:
			break;
		}

		if (parsedCommand.type == CommandType::Scan) {
			if (!appState.IsFreshStorage()) {
				CommandResult result;

				result.status = CommandStatus::InvalidState;
				result.message = spec->name;
				result.message += " requires empty tree. Execute reset first";

				return result;
			}
			else {
				return ExecuteScan(parsedCommand.arguments, appState);
			}
		}

		if (!appState.HasActiveTree()) {
			CommandResult result;

			result.status = CommandStatus::InvalidState;
			result.message = spec->name;
			result.message += " requires filled tree. Execute scan <path> first";

			return result;
		}

		switch (parsedCommand.type)
		{
		case CommandType::Rescan:
			return ExecuteRescan(appState);

		case CommandType::SaveSnapshot:
			return ExecuteSaveSnapshot(parsedCommand.arguments, appState);

		case CommandType::ShowTree:
			return ExecuteShowTree(appState);

		case CommandType::ListChildren:
			return ExecuteListChildren(appState);

		case CommandType::ShowInfo:
			return ExecuteShowInfo(appState);

		case CommandType::ShowTop:
			return ExecuteShowTop(appState);

		case CommandType::ChangeDirectory:
			return ExecuteChangeDirectory(parsedCommand.arguments, appState);

		case CommandType::PrintWorkingDirectory:
			return ExecutePrintWorkingDirectory(appState);

		default:
			break;
		}

		return ExecuteUndefined();
	}

	CommandResult CommandDispatcher::ExecuteUndefined()
	{
		CommandResult result;

		result.status = CommandStatus::ExecutionFailed;
		result.message = UndefinedCommandTypeMessage;

		return result;
	}

	CommandResult CommandDispatcher::ExecuteHelp(AppState&)
	{
		std::vector<std::string> labels;
		labels.reserve(CommandSpecs.size());
		std::size_t labelWidth = 0;
		for (const CommandSpec& spec : CommandSpecs) {
			std::string label(spec.usage);
			if (!spec.shortName.empty()) {
				label += " (";
				label += spec.shortName;
				label += ')';
			}

			labelWidth = std::max(labelWidth, GetTextWidth(label));
			labels.push_back(std::move(label));
		}

		std::ostringstream output;
		for (std::size_t index = 0; index < CommandSpecs.size(); ++index) {
			WritePaddedRight(output, labels[index], labelWidth);
			output << "  -  " << CommandSpecs[index].description << '\n';
		}

		return { CommandStatus::Successful, output.str() };
	}

	CommandResult CommandDispatcher::ExecuteQuit(AppState& appState)
	{
		CommandResult result;

		if (!appState.isQuitRequested) {
			appState.isQuitRequested = true;
			result.status = CommandStatus::Successful;
		}
		else {
			result.status = CommandStatus::ExecutionFailed;
		}

		result.message = QuittingMessage;

		return result;
	}

	CommandResult CommandDispatcher::ExecuteSetUnits(const std::vector<std::string>& arguments, AppState& appState)
	{
		CommandResult result;

		const bool isDecimalArg = arguments.front() == "decimal";
		const bool isBinaryArg = arguments.front() == "binary";

		if (!isDecimalArg && !isBinaryArg) {
			return {
				CommandStatus::InvalidArgs
				, "Expected: units <binary|decimal>"
			};
		}

		appState.unitsType = isDecimalArg ? FileSizeUnitSystem::Decimal : FileSizeUnitSystem::Binary;

		result.status = CommandStatus::Successful;
		result.message = "Units set to ";
		result.message += isDecimalArg ? "decimal" : "binary";

		return result;
	}

	CommandResult CommandDispatcher::ExecuteScan(const std::vector<std::string>& arguments, AppState& appState)
	{
		scan::ScanInput input;
		input.path = MakePath(arguments.front());
		if (input.path.empty()) {
			return { CommandStatus::InvalidArgs, "Scan path is empty" };
		}
		std::error_code scanPathError;
		if (!std::filesystem::is_directory(input.path, scanPathError) || scanPathError) {
			return { CommandStatus::ExecutionFailed, "Scan path is not an accessible directory" };
		}

		std::filesystem::path scanRootPath = input.path;
		std::error_code pathError;
		const std::filesystem::path absolutePath = std::filesystem::absolute(scanRootPath, pathError);
		if (!pathError) {
			scanRootPath = absolutePath.lexically_normal();
		}

		scan::Coordinator coordinator(appState.context->storage);
		scan::Operations operations;
		if (!operations.TryScheduleRootScan(coordinator, std::move(input))) {
			return { CommandStatus::ExecutionFailed, "Failed to schedule scan" };
		}

		const scan::Summary summary = operations.RunScanning(coordinator);
		if (!appState.HasActiveTree() || summary.scanJobStatistics.appliedJobCount == 0) {
			return { CommandStatus::ExecutionFailed, "Scan failed" };
		}
		appState.context->scanRootPath = std::move(scanRootPath);

		std::ostringstream message;
		message << "Scan completed: " << summary.storedNodesCount << " nodes, "
			<< FormatFileSize(summary.totalLogicalSize, appState.unitsType)
			<< ", " << summary.scanJobStatistics.appliedJobCount << " jobs applied"
			<< ", " << summary.scanJobStatistics.rejectedJobCount << " rejected"
			<< ", pending peak " << summary.pendingJobsPeakCount
			<< ", " << ToMilliseconds(summary.totalScanElapsedTime) << " ms";

		return { CommandStatus::Successful, message.str() };
	}

	CommandResult CommandDispatcher::ExecuteRescan(AppState& appState)
	{
		const Node* currentNode = appState.context->session.GetCurrentNode();
		if (currentNode == nullptr) {
			return { CommandStatus::ExecutionFailed, "Current directory is unavailable" };
		}

		if (appState.context->scanRootPath.empty()) {
			return { CommandStatus::ExecutionFailed, "Scan source path is unavailable" };
		}

		std::filesystem::path scanPath = appState.context->scanRootPath;
		const std::vector<NativeStringView> components = TreeQuery::CollectPathComponents(currentNode);
		for (std::size_t index = 1; index < components.size(); ++index) {
			scanPath /= components[index];
		}

		scan::ScanInput input;
		input.path = std::move(scanPath);
		input.maxDepth = UnlimitedScanDepth;

		scan::Coordinator coordinator(appState.context->storage);
		scan::Operations operations;
		if (!operations.TryScheduleReplaceScan(
			coordinator,
			std::move(input),
			const_cast<Node*>(currentNode))) {
			return { CommandStatus::ExecutionFailed, "Rescan failed" };
		}

		const scan::Summary summary = operations.RunScanning(coordinator);
		if (summary.scanJobStatistics.appliedJobCount == 0) {
			return { CommandStatus::ExecutionFailed, "Rescan failed" };
		}

		return { CommandStatus::Successful, "Rescan completed" };
	}

	CommandResult CommandDispatcher::ExecuteSaveSnapshot(const std::vector<std::string>& arguments, AppState& appState)
	{
		const std::filesystem::path path = MakePath(arguments.front());
		if (path.empty()) {
			return { CommandStatus::InvalidArgs, "Snapshot path is empty" };
		}

		snapshot::Operations operations;
		const snapshot::SavedSnapshotSummary summary =
			operations.TrySaveSnapshot(path, appState.context->storage);
		if (!summary.isSuccessful) {
			return { CommandStatus::ExecutionFailed, "Failed to save snapshot" };
		}

		return {
			CommandStatus::Successful,
			"Snapshot saved in " + std::to_string(ToMilliseconds(summary.saveDuration)) + " ms"
		};
	}

	CommandResult CommandDispatcher::ExecuteLoadSnapshot(const std::vector<std::string>& arguments, AppState& appState)
	{
		const std::filesystem::path path = MakePath(arguments.front());
		if (path.empty()) {
			return { CommandStatus::InvalidArgs, "Snapshot path is empty" };
		}

		auto candidate = std::make_unique<TreeContext>();
		snapshot::Operations operations;
		const snapshot::LoadedSnapshotSummary summary =
			operations.TryLoadSnapshot(path, candidate->storage);
		if (!summary.isSuccessful) {
			return { CommandStatus::ExecutionFailed, "Failed to load snapshot" };
		}

		appState.context = std::move(candidate);
		return {
			CommandStatus::Successful,
			"Snapshot loaded in " + std::to_string(ToMilliseconds(summary.loadDuration)) + " ms"
		};
	}

	CommandResult CommandDispatcher::ExecuteShowTree(AppState& appState)
	{
		const Node* currentNode = appState.context->session.GetCurrentNode();
		if (currentNode == nullptr) {
			return { CommandStatus::ExecutionFailed, "Current entry is unavailable" };
		}

		std::ostringstream output;
		report::TextWriter writer;
		if (!writer.WriteTreeReport(output, *currentNode)) {
			return { CommandStatus::ExecutionFailed, "Failed to build tree report" };
		}

		return { CommandStatus::Successful, output.str() };
	}

	CommandResult CommandDispatcher::ExecuteListChildren(AppState& appState)
	{
		const std::vector<const Node*>& children = appState.context->session.GetAvailableChildren();
		if (children.empty()) {
			return { CommandStatus::Successful, "No children" };
		}

		return { CommandStatus::Successful, MakeNodeTable(children, appState.unitsType) };
	}

	CommandResult CommandDispatcher::ExecuteShowInfo(AppState& appState)
	{
		const Node* currentNode = appState.context->session.GetCurrentNode();
		if (currentNode == nullptr) {
			return { CommandStatus::ExecutionFailed, "Current entry is unavailable" };
		}

		std::ostringstream output;
		WriteInfoLine(output, "Path", ToUtf8(TreeQuery::BuildNativePath(currentNode)));
		WriteInfoLine(output, "Type", ToString(currentNode->entryType));
		WriteInfoLine(output, "Status", ToString(currentNode->entryStatus));
		WriteInfoLine(output, "Scan status", ToString(currentNode->scanStatus));
		WriteInfoLine(output, "Size", FormatFileSize(currentNode->logicalSize, appState.unitsType));
		WriteInfoLine(output, "Children", std::to_string(TreeQuery::CollectChildren(currentNode).size()));

		return { CommandStatus::Successful, output.str() };
	}

	CommandResult CommandDispatcher::ExecuteShowTop(AppState& appState)
	{
		std::vector<const Node*> children = appState.context->session.GetAvailableChildren();
		if (children.empty()) {
			return { CommandStatus::Successful, "No children" };
		}

		std::ranges::sort(children, [](const Node* left, const Node* right) {
			return left->logicalSize > right->logicalSize;
			});

		return { CommandStatus::Successful, MakeNodeTable(children, appState.unitsType) };
	}

	CommandResult CommandDispatcher::ExecuteChangeDirectory(const std::vector<std::string>& arguments, AppState& appState)
	{
		const NativeString nativePath = MakePath(arguments.front()).native();
		if (nativePath.empty()) {
			return { CommandStatus::InvalidArgs, "Directory path is empty" };
		}

		Session& session = appState.context->session;
		SessionOperations operations;
		const bool isRootPath = nativePath == NativeString(1, static_cast<NativeChar>('/'))
			|| nativePath == NativeString(1, static_cast<NativeChar>('\\'));
		const Node* target = isRootPath
			? session.GetRoot()
			: TreeQuery::FindNodeByPath(session.GetCurrentNode(), nativePath);
		if (target == nullptr && session.GetCurrentNode() != session.GetRoot()) {
			target = TreeQuery::FindNodeByPath(session.GetRoot(), nativePath);
		}

		if (!operations.TryChangeDirectory(session, target)) {
			return { CommandStatus::ExecutionFailed, "Directory not found" };
		}

		return { CommandStatus::Successful, ToUtf8(session.GetCurrentNativePath()) };
	}

	CommandResult CommandDispatcher::ExecutePrintWorkingDirectory(AppState& appState)
	{
		const NativeString path = appState.context->session.GetCurrentNativePath();
		if (path.empty()) {
			return { CommandStatus::ExecutionFailed, "Current directory is unavailable" };
		}

		return { CommandStatus::Successful, ToUtf8(path) };
	}

	CommandResult CommandDispatcher::ExecuteReset(AppState& appState)
	{
		CommandResult result;

		appState.ResetTreeContext();
		result.status = CommandStatus::Successful;
		result.message = "Tree reset";

		return result;
	}
}
