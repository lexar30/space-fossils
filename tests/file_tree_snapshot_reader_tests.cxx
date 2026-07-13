#include "space_fossils/file_tree/snapshot/reader.hxx"

#include "space_fossils/file_tree/model/node.hxx"
#include "space_fossils/file_tree/snapshot/writer.hxx"
#include "space_fossils/version.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <cstdint>
#include <cstring>
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
			return Writer::MagicBytes.size();
		}

		std::size_t NativeCharSizeOffset(const std::string& bytes)
		{
			std::size_t offset = Writer::MagicBytes.size();
			offset += sizeof(std::uint32_t);

			const std::uint64_t applicationVersionLength = ReadValueAt<std::uint64_t>(bytes, offset);
			offset += static_cast<std::size_t>(applicationVersionLength);
			offset += sizeof(std::uint64_t);

			return offset;
		}

		std::size_t BodyOffset(const std::string& bytes)
		{
			return NativeCharSizeOffset(bytes) + sizeof(std::uint8_t);
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

		std::string WriteSnapshot(const Node& root)
		{
			Writer writer;
			std::ostringstream out(std::ios::out | std::ios::binary);

			const bool written = writer.TryWriteSnapshot(out, &root);
			SF_ASSERT_EQ(written, true);

			return out.str();
		}

		std::optional<TreePoolBundle> ReadSnapshot(const std::string& bytes)
		{
			Reader reader;
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

	SF_TEST(file_tree_snapshot_reader, RejectsEmptyStream)
	{
		std::optional<TreePoolBundle> bundle = ReadSnapshot({});

		SF_ASSERT_EQ(bundle.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_reader, ReadsWriterSnapshotIntoBundle)
	{
		TestTree tree;
		const std::string bytes = WriteSnapshot(tree.root);

		std::optional<TreePoolBundle> bundle = ReadSnapshot(bytes);

		SF_ASSERT_EQ(bundle.has_value(), true);
		AssertRoundTrippedTree(*bundle);
	}

	SF_TEST(file_tree_snapshot_reader, RejectsInvalidMagicBytes)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		bytes[0] = 'X';

		std::optional<TreePoolBundle> bundle = ReadSnapshot(bytes);

		SF_ASSERT_EQ(bundle.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_reader, RejectsUnsupportedSnapshotVersion)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		const std::uint32_t unsupportedVersion = version::SnapshotFormatVersion + 1;
		WriteValueAt(bytes, SnapshotFormatVersionOffset(), unsupportedVersion);

		std::optional<TreePoolBundle> bundle = ReadSnapshot(bytes);

		SF_ASSERT_EQ(bundle.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_reader, RejectsNativeCharSizeMismatch)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		const std::size_t offset = NativeCharSizeOffset(bytes);
		const std::uint8_t invalidNativeCharSize = static_cast<std::uint8_t>(sizeof(NativeChar) + 1);
		WriteValueAt(bytes, offset, invalidNativeCharSize);

		std::optional<TreePoolBundle> bundle = ReadSnapshot(bytes);

		SF_ASSERT_EQ(bundle.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_reader, RejectsTruncatedSnapshot)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		SF_ASSERT_EQ(bytes.empty(), false);
		bytes.pop_back();

		std::optional<TreePoolBundle> bundle = ReadSnapshot(bytes);

		SF_ASSERT_EQ(bundle.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_reader, RejectsUnalignedNameByteLength)
	{
		if constexpr (sizeof(NativeChar) > 1) {
			TestTree tree;
			std::string bytes = WriteSnapshot(tree.root);
			const std::uint64_t invalidNameByteLength = static_cast<std::uint64_t>(sizeof(NativeChar) + 1);
			SF_ASSERT_EQ(invalidNameByteLength % sizeof(NativeChar) != 0, true);
			WriteValueAt(bytes, BodyOffset(bytes), invalidNameByteLength);

			std::optional<TreePoolBundle> bundle = ReadSnapshot(bytes);

			SF_ASSERT_EQ(bundle.has_value(), false);
		}
		else {
			SF_ASSERT_EQ(sizeof(NativeChar), 1);
		}
	}

	SF_TEST(file_tree_snapshot_reader, RejectsNameByteLengthPastEndOfStream)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		const std::uint64_t nameCharactersPastEnd = static_cast<std::uint64_t>(bytes.size() / sizeof(NativeChar) + 1);
		const std::uint64_t tooLargeNameByteLength = nameCharactersPastEnd * sizeof(NativeChar);
		WriteValueAt(bytes, BodyOffset(bytes), tooLargeNameByteLength);

		std::optional<TreePoolBundle> bundle = ReadSnapshot(bytes);

		SF_ASSERT_EQ(bundle.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_reader, RejectsMissingDeclaredChildRecords)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		const std::uint64_t tooManyChildren = 3;
		WriteValueAt(bytes, RootChildCountOffset(bytes), tooManyChildren);

		std::optional<TreePoolBundle> bundle = ReadSnapshot(bytes);

		SF_ASSERT_EQ(bundle.has_value(), false);
	}

	SF_TEST(file_tree_snapshot_reader, RejectsTrailingBytesAfterTree)
	{
		TestTree tree;
		std::string bytes = WriteSnapshot(tree.root);
		bytes.push_back('\0');

		std::optional<TreePoolBundle> bundle = ReadSnapshot(bytes);

		SF_ASSERT_EQ(bundle.has_value(), false);
	}
}
