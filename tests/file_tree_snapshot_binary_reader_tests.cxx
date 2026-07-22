#include "space_fossils/core/file_tree/snapshot/binary_reader.hxx"

#include "space_fossils/core/file_tree/model/node.hxx"
#include "space_fossils/core/file_tree/model/tree_metadata.hxx"
#include "space_fossils/core/file_tree/snapshot/binary_writer.hxx"
#include "space_fossils/core/file_tree/snapshot/constants.hxx"
#include "space_fossils/core/version.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

namespace space_fossils::tests {
	namespace {
		using namespace space_fossils::core;
		using namespace space_fossils::core::file_tree;
		using namespace space_fossils::core::file_tree::snapshot;

		NativeString MakeNativeString(const char* value)
		{
			NativeString result;
			while (*value != '\0') {
				result.push_back(static_cast<NativeChar>(*value));
				++value;
			}

			return result;
		}

		NameRef MakeNameRef(const NativeString& value)
		{
			return NameRef{
				value.data(),
				static_cast<std::uint32_t>(value.size())
			};
		}

		void AssertNameEquals(const Node& node, const char* expectedValue)
		{
			NativeString expectedName = MakeNativeString(expectedValue);
			NativeStringView actualName = ToStringView(node.name);

			SF_ASSERT_EQ(actualName.size(), expectedName.size());
			for (std::size_t index = 0; index < expectedName.size(); ++index) {
				SF_ASSERT_EQ(actualName[index] == expectedName[index], true);
			}
		}

		template<typename T>
		T ReadValueAt(const std::string& data, std::size_t& offset)
		{
			SF_ASSERT_EQ(offset + sizeof(T) <= data.size(), true);

			T value{};
			std::memcpy(&value, data.data() + offset, sizeof(T));
			offset += sizeof(T);

			return value;
		}

		template<typename T>
		void WriteValueAt(std::string& data, std::size_t offset, T value)
		{
			SF_ASSERT_EQ(offset + sizeof(T) <= data.size(), true);
			std::memcpy(data.data() + offset, &value, sizeof(T));
		}

		std::size_t SnapshotFormatVersionOffset()
		{
			return MagicBytes.size();
		}

		std::size_t ApplicationVersionLengthOffset()
		{
			return SnapshotFormatVersionOffset() + sizeof(std::uint32_t);
		}

		std::size_t ApplicationVersionDataOffset()
		{
			return ApplicationVersionLengthOffset() + sizeof(std::uint64_t);
		}

		std::size_t NativeCharSizeOffset(const std::string& bytes)
		{
			std::size_t offset = ApplicationVersionLengthOffset();
			const std::uint64_t applicationVersionLength = ReadValueAt<std::uint64_t>(bytes, offset);
			offset += static_cast<std::size_t>(applicationVersionLength);
			return offset;
		}

		std::size_t UpdatedAtOffset(const std::string& bytes)
		{
			return NativeCharSizeOffset(bytes) + sizeof(std::uint8_t);
		}

		std::size_t SourcePathLengthOffset(const std::string& bytes)
		{
			return UpdatedAtOffset(bytes) + sizeof(std::uint64_t);
		}

		std::size_t SourcePathDataOffset(const std::string& bytes)
		{
			return SourcePathLengthOffset(bytes) + sizeof(std::uint64_t);
		}

		std::size_t BodyOffset(const std::string& bytes)
		{
			std::size_t offset = SourcePathLengthOffset(bytes);
			const std::uint64_t sourcePathBytesLength = ReadValueAt<std::uint64_t>(bytes, offset);
			offset += static_cast<std::size_t>(sourcePathBytesLength);

			return offset;
		}

		std::size_t RootEntryTypeOffset(const std::string& bytes)
		{
			std::size_t offset = BodyOffset(bytes);
			const std::uint64_t nameByteLength = ReadValueAt<std::uint64_t>(bytes, offset);
			offset += static_cast<std::size_t>(nameByteLength);
			offset += sizeof(std::uint64_t);

			return offset;
		}

		std::size_t RootEntryStatusOffset(const std::string& bytes)
		{
			return RootEntryTypeOffset(bytes) + sizeof(std::uint8_t);
		}

		std::size_t RootScanStatusOffset(const std::string& bytes)
		{
			return RootEntryStatusOffset(bytes) + sizeof(std::uint8_t);
		}

		std::size_t RootChildCountOffset(const std::string& bytes)
		{
			std::size_t offset = BodyOffset(bytes);

			const std::uint64_t nameByteLength = ReadValueAt<std::uint64_t>(bytes, offset);
			offset += static_cast<std::size_t>(nameByteLength);
			offset += sizeof(std::uint64_t);
			offset += sizeof(std::uint8_t);
			offset += sizeof(std::uint8_t);
			offset += sizeof(std::uint8_t);

			return offset;
		}

		struct TestTree
		{
			NativeString rootName = MakeNativeString("root");
			NativeString firstName = MakeNativeString("folder-a");
			NativeString nestedName = MakeNativeString("inside.txt");
			NativeString secondName = MakeNativeString("file-b.bin");

			Node root;
			Node first;
			Node nested;
			Node second;

			TestTree()
			{
				root.name = MakeNameRef(rootName);
				root.logicalSize = 60;
				root.entryType = EntryType::Directory;
				root.entryStatus = EntryStatus::Accessible;
				root.scanStatus = EntryScanStatus::Complete;

				first.name = MakeNameRef(firstName);
				first.logicalSize = 10;
				first.entryType = EntryType::Directory;
				first.entryStatus = EntryStatus::Accessible;
				first.scanStatus = EntryScanStatus::Partial;
				first.parent = &root;

				nested.name = MakeNameRef(nestedName);
				nested.logicalSize = 4;
				nested.entryType = EntryType::File;
				nested.entryStatus = EntryStatus::Accessible;
				nested.scanStatus = EntryScanStatus::Complete;
				nested.parent = &first;

				second.name = MakeNameRef(secondName);
				second.logicalSize = 50;
				second.entryType = EntryType::Other;
				second.entryStatus = EntryStatus::Error;
				second.scanStatus = EntryScanStatus::Error;
				second.parent = &root;

				root.firstChild = &first;
				first.firstChild = &nested;
				first.nextSibling = &second;
			}
		};

		std::string WriteSnapshot(const Node& root, const TreeMetadata& treeMetadata = {})
		{
			BinaryWriter writer;
			std::ostringstream out(std::ios::out | std::ios::binary);

			const bool written = writer.TryWriteSnapshot(out, &root, treeMetadata);
			SF_ASSERT_EQ(written, true);

			return out.str();
		}

		std::optional<LoadedSnapshot> ReadSnapshot(const std::string& bytes)
		{
			BinaryReader reader;
			std::istringstream in(bytes, std::ios::in | std::ios::binary);

			return reader.TryReadSnapshot(in);
		}

		void AssertRoundTrippedTree(const TreePoolBundle& bundle)
		{
			SF_ASSERT_EQ(bundle.root != nullptr, true);
			SF_ASSERT_EQ(bundle.createdNodesCount, 4);

			const Node* root = bundle.root;
			AssertNameEquals(*root, "root");
			SF_ASSERT_EQ(root->logicalSize, 60);
			SF_ASSERT_EQ(root->entryType, EntryType::Directory);
			SF_ASSERT_EQ(root->entryStatus, EntryStatus::Accessible);
			SF_ASSERT_EQ(root->scanStatus, EntryScanStatus::Complete);
			SF_ASSERT_EQ(root->parent == nullptr, true);
			SF_ASSERT_EQ(root->nextSibling == nullptr, true);

			const Node* first = root->firstChild;
			SF_ASSERT_EQ(first != nullptr, true);
			AssertNameEquals(*first, "folder-a");
			SF_ASSERT_EQ(first->logicalSize, 10);
			SF_ASSERT_EQ(first->entryType, EntryType::Directory);
			SF_ASSERT_EQ(first->entryStatus, EntryStatus::Accessible);
			SF_ASSERT_EQ(first->scanStatus, EntryScanStatus::Partial);
			SF_ASSERT_EQ(first->parent == root, true);

			const Node* nested = first->firstChild;
			SF_ASSERT_EQ(nested != nullptr, true);
			AssertNameEquals(*nested, "inside.txt");
			SF_ASSERT_EQ(nested->logicalSize, 4);
			SF_ASSERT_EQ(nested->entryType, EntryType::File);
			SF_ASSERT_EQ(nested->entryStatus, EntryStatus::Accessible);
			SF_ASSERT_EQ(nested->scanStatus, EntryScanStatus::Complete);
			SF_ASSERT_EQ(nested->parent == first, true);
			SF_ASSERT_EQ(nested->firstChild == nullptr, true);
			SF_ASSERT_EQ(nested->nextSibling == nullptr, true);

			const Node* second = first->nextSibling;
			SF_ASSERT_EQ(second != nullptr, true);
			AssertNameEquals(*second, "file-b.bin");
			SF_ASSERT_EQ(second->logicalSize, 50);
			SF_ASSERT_EQ(second->entryType, EntryType::Other);
			SF_ASSERT_EQ(second->entryStatus, EntryStatus::Error);
			SF_ASSERT_EQ(second->scanStatus, EntryScanStatus::Error);
			SF_ASSERT_EQ(second->parent == root, true);
			SF_ASSERT_EQ(second->firstChild == nullptr, true);
			SF_ASSERT_EQ(second->nextSibling == nullptr, true);
		}
	}

	SF_TEST(file_tree_snapshot_binary_reader, RejectsEmptyStream)
	{
		BinaryReader reader;
		std::istringstream in({}, std::ios::in | std::ios::binary);

		std::optional<LoadedSnapshot> snapshot = reader.TryReadSnapshot(in);

		SF_ASSERT_EQ(snapshot.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_binary_reader, ReadsWriterSnapshotIntoLoadedSnapshot)
	{
		TestTree tree;
		TreeMetadata treeMetadata;
		treeMetadata.scanSourcePath = std::filesystem::path("source") / "root";
		treeMetadata.treeSource = TreeSource::Scan;
		treeMetadata.applicationVersion = "ignored-writer-version";
		treeMetadata.updatedAtUnixSeconds = 123456;
		const std::string bytes = WriteSnapshot(tree.root, treeMetadata);
		BinaryReader reader;
		std::istringstream in(bytes, std::ios::in | std::ios::binary);
		SF_ASSERT_EQ(reader.GetReadElapsedTime().count(), 0);

		std::optional<LoadedSnapshot> snapshot = reader.TryReadSnapshot(in);

		SF_ASSERT_EQ(snapshot.has_value(), true);
		AssertRoundTrippedTree(snapshot->poolBundle);
		SF_ASSERT_EQ(snapshot->treeMetadata.scanSourcePath == treeMetadata.scanSourcePath, true);
		SF_ASSERT_EQ(snapshot->treeMetadata.treeSource, TreeSource::Snapshot);
		SF_ASSERT_EQ(snapshot->treeMetadata.applicationVersion, std::string(version::ApplicationVersion));
		SF_ASSERT_EQ(snapshot->treeMetadata.updatedAtUnixSeconds, treeMetadata.updatedAtUnixSeconds);
	}

	SF_TEST(file_tree_snapshot_binary_reader, RejectsInvalidMagicBytes)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		bytes[0] = 'X';

		std::optional<LoadedSnapshot> snapshot = ReadSnapshot(bytes);

		SF_ASSERT_EQ(snapshot.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_binary_reader, RejectsUnsupportedSnapshotVersion)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		const std::uint32_t unsupportedVersion = version::BinarySnapshotFormatVersion + 1;
		WriteValueAt(bytes, SnapshotFormatVersionOffset(), unsupportedVersion);

		std::optional<LoadedSnapshot> snapshot = ReadSnapshot(bytes);

		SF_ASSERT_EQ(snapshot.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_binary_reader, PreservesApplicationVersionWhenFormatVersionIsSupported)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		std::size_t lengthOffset = ApplicationVersionLengthOffset();
		const std::uint64_t applicationVersionLength = ReadValueAt<std::uint64_t>(bytes, lengthOffset);
		const std::string expectedVersion(static_cast<std::size_t>(applicationVersionLength), 'x');
		std::memcpy(bytes.data() + ApplicationVersionDataOffset(), expectedVersion.data(), expectedVersion.size());

		std::optional<LoadedSnapshot> snapshot = ReadSnapshot(bytes);

		SF_ASSERT_EQ(snapshot.has_value(), true);
		SF_ASSERT_EQ(snapshot->treeMetadata.applicationVersion, expectedVersion);
	}

	SF_TEST(file_tree_snapshot_binary_reader, RejectsApplicationVersionLengthAboveLimit)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		WriteValueAt(bytes, ApplicationVersionLengthOffset(), ApplicationVersionLengthLimit + 1);

		std::optional<LoadedSnapshot> snapshot = ReadSnapshot(bytes);

		SF_ASSERT_EQ(snapshot.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_binary_reader, RejectsTruncatedApplicationVersion)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		std::size_t lengthOffset = ApplicationVersionLengthOffset();
		const std::uint64_t applicationVersionLength = ReadValueAt<std::uint64_t>(bytes, lengthOffset);
		SF_ASSERT_EQ(applicationVersionLength > 0, true);
		bytes.resize(ApplicationVersionDataOffset() + static_cast<std::size_t>(applicationVersionLength) - 1);

		std::optional<LoadedSnapshot> snapshot = ReadSnapshot(bytes);

		SF_ASSERT_EQ(snapshot.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_binary_reader, RejectsNativeCharSizeMismatch)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		const std::size_t offset = NativeCharSizeOffset(bytes);
		const std::uint8_t invalidNativeCharSize = static_cast<std::uint8_t>(sizeof(NativeChar) + 1);
		WriteValueAt(bytes, offset, invalidNativeCharSize);

		std::optional<LoadedSnapshot> snapshot = ReadSnapshot(bytes);

		SF_ASSERT_EQ(snapshot.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_binary_reader, ReadsEmptySourcePath)
	{
		TestTree tree;
		std::optional<LoadedSnapshot> snapshot = ReadSnapshot(WriteSnapshot(tree.root));

		SF_ASSERT_EQ(snapshot.has_value(), true);
		SF_ASSERT_EQ(snapshot->treeMetadata.scanSourcePath.empty(), true);
		SF_ASSERT_EQ(snapshot->treeMetadata.updatedAtUnixSeconds, 0);
	}

	SF_TEST(file_tree_snapshot_binary_reader, ReadsNonAsciiSourcePath)
	{
		TestTree tree;
		TreeMetadata treeMetadata;
		treeMetadata.scanSourcePath = std::filesystem::path(std::u8string(u8"source/данные"));

		std::optional<LoadedSnapshot> snapshot = ReadSnapshot(WriteSnapshot(tree.root, treeMetadata));

		SF_ASSERT_EQ(snapshot.has_value(), true);
		SF_ASSERT_EQ(snapshot->treeMetadata.scanSourcePath == treeMetadata.scanSourcePath, true);
	}

	SF_TEST(file_tree_snapshot_binary_reader, ReadsEmptyNodeName)
	{
		Node root;

		std::optional<LoadedSnapshot> snapshot = ReadSnapshot(WriteSnapshot(root));

		SF_ASSERT_EQ(snapshot.has_value(), true);
		SF_ASSERT_EQ(snapshot->poolBundle.root != nullptr, true);
		SF_ASSERT_EQ(snapshot->poolBundle.root->name.length, 0);
		SF_ASSERT_EQ(snapshot->poolBundle.root->firstChild == nullptr, true);
	}

	SF_TEST(file_tree_snapshot_binary_reader, RejectsSourcePathLengthAboveLimit)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		const std::uint64_t invalidLength = SourcePathBytesLengthLimit + sizeof(NativeChar);
		WriteValueAt(bytes, SourcePathLengthOffset(bytes), invalidLength);

		std::optional<LoadedSnapshot> snapshot = ReadSnapshot(bytes);

		SF_ASSERT_EQ(snapshot.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_binary_reader, RejectsUnalignedSourcePathByteLength)
	{
		if constexpr (sizeof(NativeChar) > 1) {
			TestTree tree;
			std::string bytes = WriteSnapshot(tree.root);
			WriteValueAt(bytes, SourcePathLengthOffset(bytes), static_cast<std::uint64_t>(1));

			std::optional<LoadedSnapshot> snapshot = ReadSnapshot(bytes);

			SF_ASSERT_EQ(snapshot.has_value(), false);
		}
		else {
			SF_ASSERT_EQ(sizeof(NativeChar), 1);
		}
	}

	SF_TEST(file_tree_snapshot_binary_reader, RejectsTruncatedSourcePath)
	{
		TestTree tree;
		TreeMetadata treeMetadata;
		treeMetadata.scanSourcePath = std::filesystem::path("source") / "root";
		std::string bytes = WriteSnapshot(tree.root, treeMetadata);
		std::size_t lengthOffset = SourcePathLengthOffset(bytes);
		const std::uint64_t sourcePathBytesLength = ReadValueAt<std::uint64_t>(bytes, lengthOffset);
		SF_ASSERT_EQ(sourcePathBytesLength > 0, true);
		bytes.resize(SourcePathDataOffset(bytes) + static_cast<std::size_t>(sourcePathBytesLength) - 1);

		std::optional<LoadedSnapshot> snapshot = ReadSnapshot(bytes);

		SF_ASSERT_EQ(snapshot.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_binary_reader, RejectsTruncatedSnapshot)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		SF_ASSERT_EQ(bytes.empty(), false);
		bytes.pop_back();

		std::optional<LoadedSnapshot> snapshot = ReadSnapshot(bytes);

		SF_ASSERT_EQ(snapshot.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_binary_reader, RejectsEveryTruncatedSnapshotPrefix)
	{
		TestTree tree;
		const std::string bytes = WriteSnapshot(tree.root);

		for (std::size_t prefixLength = 0; prefixLength < bytes.size(); ++prefixLength) {
			const std::string truncatedBytes = bytes.substr(0, prefixLength);
			std::optional<LoadedSnapshot> snapshot = ReadSnapshot(truncatedBytes);

			SF_ASSERT_EQ(snapshot.has_value(), false);
		}
	}

	SF_TEST(file_tree_snapshot_binary_reader, RejectsUnalignedNameByteLength)
	{
		if constexpr (sizeof(NativeChar) > 1) {
			TestTree tree;
			std::string bytes = WriteSnapshot(tree.root);
			const std::uint64_t invalidNameByteLength = static_cast<std::uint64_t>(sizeof(NativeChar) + 1);
			SF_ASSERT_EQ(invalidNameByteLength % sizeof(NativeChar) != 0, true);
			WriteValueAt(bytes, BodyOffset(bytes), invalidNameByteLength);

			std::optional<LoadedSnapshot> snapshot = ReadSnapshot(bytes);

			SF_ASSERT_EQ(snapshot.has_value(), false);
		}
		else {
			SF_ASSERT_EQ(sizeof(NativeChar), 1);
		}
	}

	SF_TEST(file_tree_snapshot_binary_reader, RejectsNameByteLengthPastEndOfStream)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		const std::uint64_t nameCharactersPastEnd = static_cast<std::uint64_t>(bytes.size() / sizeof(NativeChar) + 1);
		const std::uint64_t tooLargeNameByteLength = nameCharactersPastEnd * sizeof(NativeChar);
		WriteValueAt(bytes, BodyOffset(bytes), tooLargeNameByteLength);

		std::optional<LoadedSnapshot> snapshot = ReadSnapshot(bytes);

		SF_ASSERT_EQ(snapshot.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_binary_reader, RejectsNameLengthPastNameRefCapacity)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		constexpr std::uint64_t invalidCharacterCount = static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()) + 1;
		const std::uint64_t invalidNameByteLength = invalidCharacterCount * sizeof(NativeChar);
		WriteValueAt(bytes, BodyOffset(bytes), invalidNameByteLength);

		std::optional<LoadedSnapshot> snapshot = ReadSnapshot(bytes);

		SF_ASSERT_EQ(snapshot.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_binary_reader, RejectsUnknownEntryType)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		constexpr std::uint8_t invalidEntryType = std::numeric_limits<std::uint8_t>::max();
		WriteValueAt(bytes, RootEntryTypeOffset(bytes), invalidEntryType);

		std::optional<LoadedSnapshot> snapshot = ReadSnapshot(bytes);

		SF_ASSERT_EQ(snapshot.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_binary_reader, RejectsUnknownEntryStatus)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		constexpr std::uint8_t invalidEntryStatus = std::numeric_limits<std::uint8_t>::max();
		WriteValueAt(bytes, RootEntryStatusOffset(bytes), invalidEntryStatus);

		std::optional<LoadedSnapshot> snapshot = ReadSnapshot(bytes);

		SF_ASSERT_EQ(snapshot.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_binary_reader, RejectsUnknownEntryScanStatus)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		constexpr std::uint8_t invalidScanStatus = std::numeric_limits<std::uint8_t>::max();
		WriteValueAt(bytes, RootScanStatusOffset(bytes), invalidScanStatus);

		std::optional<LoadedSnapshot> snapshot = ReadSnapshot(bytes);

		SF_ASSERT_EQ(snapshot.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_binary_reader, RejectsMissingDeclaredChildRecords)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		const std::uint64_t tooManyChildren = 3;
		WriteValueAt(bytes, RootChildCountOffset(bytes), tooManyChildren);

		std::optional<LoadedSnapshot> snapshot = ReadSnapshot(bytes);

		SF_ASSERT_EQ(snapshot.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_binary_reader, RejectsTrailingBytesAfterTree)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		bytes.push_back('\0');

		std::optional<LoadedSnapshot> snapshot = ReadSnapshot(bytes);

		SF_ASSERT_EQ(snapshot.has_value(), false);
	}
}
