#include "space_fossils/core/file_tree/snapshot/writer.hxx"

#include "space_fossils/core/file_tree/model/node.hxx"
#include "space_fossils/core/version.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <streambuf>
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

		void AssertNameEquals(const NativeString& actualName, const char* expectedValue)
		{
			NativeString expectedName = MakeNativeString(expectedValue);

			SF_ASSERT_EQ(actualName.size(), expectedName.size());
			for (std::size_t index = 0; index < expectedName.size(); ++index) {
				SF_ASSERT_EQ(actualName[index] == expectedName[index], true);
			}
		}

		std::uint64_t CurrentUnixSeconds()
		{
			const auto epochDuration = std::chrono::system_clock::now().time_since_epoch();
			return std::chrono::duration_cast<std::chrono::seconds>(epochDuration).count();
		}

		template<typename T>
		T ReadValue(const std::string& data, std::size_t& offset)
		{
			SF_ASSERT_EQ(offset + sizeof(T) <= data.size(), true);

			T value{};
			std::memcpy(&value, data.data() + offset, sizeof(T));
			offset += sizeof(T);

			return value;
		}

		std::string ReadBytes(const std::string& data, std::size_t& offset, std::uint64_t size)
		{
			SF_ASSERT_EQ(offset + size <= data.size(), true);

			std::string bytes(data.data() + offset, static_cast<std::size_t>(size));
			offset += static_cast<std::size_t>(size);

			return bytes;
		}

		void AssertSnapshotMetadata(
			const std::string& bytes,
			std::size_t& offset,
			std::uint64_t timestampBefore,
			std::uint64_t timestampAfter)
		{
			const std::string magic = ReadBytes(bytes, offset, Writer::MagicBytes.size());
			SF_ASSERT_EQ(magic.size(), Writer::MagicBytes.size());
			for (std::size_t index = 0; index < Writer::MagicBytes.size(); ++index) {
				SF_ASSERT_EQ(magic[index], Writer::MagicBytes[index]);
			}

			const std::uint32_t snapshotFormatVersion = ReadValue<std::uint32_t>(bytes, offset);
			SF_ASSERT_EQ(snapshotFormatVersion, version::SnapshotFormatVersion);

			const std::uint64_t applicationVersionLength = ReadValue<std::uint64_t>(bytes, offset);
			const std::string applicationVersion = ReadBytes(bytes, offset, applicationVersionLength);
			SF_ASSERT_EQ(applicationVersion, std::string(version::ApplicationVersion));

			const std::uint64_t createdUnixSeconds = ReadValue<std::uint64_t>(bytes, offset);
			SF_ASSERT_EQ(createdUnixSeconds >= timestampBefore, true);
			SF_ASSERT_EQ(createdUnixSeconds <= timestampAfter, true);

			const std::uint8_t nativeCharSize = ReadValue<std::uint8_t>(bytes, offset);
			SF_ASSERT_EQ(nativeCharSize, static_cast<std::uint8_t>(sizeof(NativeChar)));
		}

		NativeString ReadNativeName(const std::string& data, std::size_t& offset)
		{
			const std::uint64_t nameByteLength = ReadValue<std::uint64_t>(data, offset);
			SF_ASSERT_EQ(nameByteLength % sizeof(NativeChar), 0);

			NativeString name;
			name.resize(static_cast<std::size_t>(nameByteLength / sizeof(NativeChar)));
			if (nameByteLength > 0) {
				SF_ASSERT_EQ(offset + nameByteLength <= data.size(), true);
				std::memcpy(name.data(), data.data() + offset, static_cast<std::size_t>(nameByteLength));
			}

			offset += static_cast<std::size_t>(nameByteLength);
			return name;
		}

		struct SnapshotNodeRecord
		{
			NativeString name;
			std::uint64_t logicalSize = 0;
			std::uint8_t entryType = 0;
			std::uint8_t entryStatus = 0;
			std::uint8_t scanStatus = 0;
			std::uint64_t childCount = 0;
		};

		SnapshotNodeRecord ReadNodeRecord(const std::string& data, std::size_t& offset)
		{
			SnapshotNodeRecord record;
			record.name = ReadNativeName(data, offset);
			record.logicalSize = ReadValue<std::uint64_t>(data, offset);
			record.entryType = ReadValue<std::uint8_t>(data, offset);
			record.entryStatus = ReadValue<std::uint8_t>(data, offset);
			record.scanStatus = ReadValue<std::uint8_t>(data, offset);
			record.childCount = ReadValue<std::uint64_t>(data, offset);

			return record;
		}

		class FailingAfterBytesBuffer : public std::streambuf
		{
		public:
			explicit FailingAfterBytesBuffer(std::size_t bytesBeforeFailure)
				: remainingBytes(bytesBeforeFailure)
			{
			}

		protected:
			std::streamsize xsputn(const char*, std::streamsize count) override
			{
				if (remainingBytes == 0) {
					return 0;
				}

				const std::size_t requestedBytes = static_cast<std::size_t>(count);
				const std::size_t writtenBytes = requestedBytes < remainingBytes ? requestedBytes : remainingBytes;
				remainingBytes -= writtenBytes;
				return static_cast<std::streamsize>(writtenBytes);
			}

			int_type overflow(int_type ch) override
			{
				if (traits_type::eq_int_type(ch, traits_type::eof())) {
					return traits_type::not_eof(ch);
				}

				if (remainingBytes == 0) {
					return traits_type::eof();
				}

				--remainingBytes;
				return ch;
			}

		private:
			std::size_t remainingBytes = 0;
		};

		void AssertNodeRecord(
			const SnapshotNodeRecord& record,
			const char* expectedName,
			std::uint64_t expectedLogicalSize,
			EntryType expectedEntryType,
			EntryStatus expectedEntryStatus,
			EntryScanStatus expectedScanStatus,
			std::uint64_t expectedChildCount)
		{
			AssertNameEquals(record.name, expectedName);
			SF_ASSERT_EQ(record.logicalSize, expectedLogicalSize);
			SF_ASSERT_EQ(record.entryType, static_cast<std::uint8_t>(expectedEntryType));
			SF_ASSERT_EQ(record.entryStatus, static_cast<std::uint8_t>(expectedEntryStatus));
			SF_ASSERT_EQ(record.scanStatus, static_cast<std::uint8_t>(expectedScanStatus));
			SF_ASSERT_EQ(record.childCount, expectedChildCount);
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
	}

	SF_TEST(file_tree_snapshot_writer, RejectsNullRoot)
	{
		Writer writer;
		std::ostringstream out(std::ios::out | std::ios::binary);

		const bool written = writer.TryWriteSnapshot(out, nullptr);

		SF_ASSERT_EQ(written, false);
		SF_ASSERT_EQ(out.str().empty(), true);
	}

	SF_TEST(file_tree_snapshot_writer, WritesSnapshotHeaderAndNodesInDfsPreorder)
	{
		TestTree tree;
		Writer writer;
		std::ostringstream out(std::ios::out | std::ios::binary);

		const std::uint64_t timestampBefore = CurrentUnixSeconds();
		const bool written = writer.TryWriteSnapshot(out, &tree.root);
		const std::uint64_t timestampAfter = CurrentUnixSeconds();

		SF_ASSERT_EQ(written, true);

		const std::string bytes = out.str();
		std::size_t offset = 0;

		AssertSnapshotMetadata(bytes, offset, timestampBefore, timestampAfter);

		const SnapshotNodeRecord rootRecord = ReadNodeRecord(bytes, offset);
		AssertNodeRecord(rootRecord, "root", 60, EntryType::Directory, EntryStatus::Accessible, EntryScanStatus::Complete, 2);

		const SnapshotNodeRecord firstRecord = ReadNodeRecord(bytes, offset);
		AssertNodeRecord(firstRecord, "folder-a", 10, EntryType::Directory, EntryStatus::Accessible, EntryScanStatus::Partial, 1);

		const SnapshotNodeRecord nestedRecord = ReadNodeRecord(bytes, offset);
		AssertNodeRecord(nestedRecord, "inside.txt", 4, EntryType::File, EntryStatus::Accessible, EntryScanStatus::Complete, 0);

		const SnapshotNodeRecord secondRecord = ReadNodeRecord(bytes, offset);
		AssertNodeRecord(secondRecord, "file-b.bin", 50, EntryType::Other, EntryStatus::Error, EntryScanStatus::Error, 0);

		SF_ASSERT_EQ(offset, bytes.size());
	}

	SF_TEST(file_tree_snapshot_writer, WritesEmptyNameAsZeroLengthName)
	{
		Node root;
		root.logicalSize = 42;
		root.entryType = EntryType::Unknown;
		root.entryStatus = EntryStatus::Unknown;
		root.scanStatus = EntryScanStatus::Unknown;

		Writer writer;
		std::ostringstream out(std::ios::out | std::ios::binary);

		const std::uint64_t timestampBefore = CurrentUnixSeconds();
		const bool written = writer.TryWriteSnapshot(out, &root);
		const std::uint64_t timestampAfter = CurrentUnixSeconds();

		SF_ASSERT_EQ(written, true);

		const std::string bytes = out.str();
		std::size_t offset = 0;

		AssertSnapshotMetadata(bytes, offset, timestampBefore, timestampAfter);

		const SnapshotNodeRecord rootRecord = ReadNodeRecord(bytes, offset);
		AssertNodeRecord(rootRecord, "", 42, EntryType::Unknown, EntryStatus::Unknown, EntryScanStatus::Unknown, 0);

		SF_ASSERT_EQ(offset, bytes.size());
	}

	SF_TEST(file_tree_snapshot_writer, RejectsNameWithLengthButNoData)
	{
		Node root;
		root.name = NameRef{
			nullptr,
			1
		};

		Writer writer;
		std::ostringstream out(std::ios::out | std::ios::binary);

		const bool written = writer.TryWriteSnapshot(out, &root);

		SF_ASSERT_EQ(written, false);
	}

	SF_TEST(file_tree_snapshot_writer, ReturnsFalseWhenStreamFailsDuringWrite)
	{
		TestTree tree;
		Writer writer;
		FailingAfterBytesBuffer buffer(1);
		std::ostream out(&buffer);

		const bool written = writer.TryWriteSnapshot(out, &tree.root);

		SF_ASSERT_EQ(written, false);
		SF_ASSERT_EQ(out.good(), false);
	}

	SF_TEST(file_tree_snapshot_writer, ReturnsFalseWhenStreamAlreadyFailed)
	{
		TestTree tree;
		Writer writer;
		std::ostringstream out(std::ios::out | std::ios::binary);
		out.setstate(std::ios::badbit);

		const bool written = writer.TryWriteSnapshot(out, &tree.root);

		SF_ASSERT_EQ(written, false);
	}
}
