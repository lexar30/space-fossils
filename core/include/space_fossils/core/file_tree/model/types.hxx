#pragma once

#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <string_view>

namespace space_fossils::core::file_tree {
	using NativeChar = std::filesystem::path::value_type;
	using NativeString = std::basic_string<NativeChar>;
	using NativeStringView = std::basic_string_view<NativeChar>;

	using FileSize = std::uintmax_t;
	inline constexpr FileSize DefaultFileSize = 0;

	using StorageVersion = std::uint64_t;

	enum class EntryType {
		Unknown
		, File
		, Directory
		, Symlink
		, Other
	};

	enum class EntryStatus {
		Unknown
		, Accessible
		, AccessDenied
		, NotFound
		, Error
	};

	enum class EntryScanStatus {
		Unknown
		, Pending
		, Complete
		, Partial
		, Error
	};

	struct NameRef {
		const NativeChar* data = nullptr;
		std::uint32_t length = 0;
	};

	inline NativeStringView ToStringView(NameRef ref)
	{
		if (ref.data == nullptr || ref.length == 0) {
			return {};
		}

		return NativeStringView(ref.data, ref.length);
	}
}