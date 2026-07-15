#include "space_fossils/core/file_tree/memory/name_pool.hxx"

#include <algorithm>
#include <limits>
#include <utility>

namespace space_fossils::core::file_tree {
	NamePool::NamePool(std::size_t blockSize)
		: arena(blockSize)
	{
	}

	NameRef NamePool::Store(NativeStringView name)
	{
		if (name.empty()) {
			return {};
		}

		if (name.size() > std::numeric_limits<std::uint32_t>::max()) {
			return {};
		}

		constexpr std::size_t charSize = sizeof(NativeChar);
		if (name.size() > std::numeric_limits<std::size_t>::max() / charSize) {
			return {};
		}

		std::size_t requiredBytes = name.size() * charSize;
		void* allocation = arena.Allocate(requiredBytes, alignof(NativeChar));
		if (allocation == nullptr) {
			return {};
		}

		NativeChar* storedName = static_cast<NativeChar*>(allocation);
		std::copy(name.begin(), name.end(), storedName);

		return NameRef {
			storedName,
			static_cast<std::uint32_t>(name.size())
		};
	}

	void NamePool::MergeFrom(NamePool&& other)
	{
		if (this == &other) {
			return;
		}

		arena.MergeFrom(std::move(other.arena));
	}

	void NamePool::Reset()
	{
		arena.Reset();
	}

	void NamePool::Release()
	{
		arena.Release();
	}

	std::size_t NamePool::GetAllocatedBytes() const
	{
		return arena.GetAllocatedBytes();
	}

	std::size_t NamePool::GetUsedBytes() const
	{
		return arena.GetUsedSize();
	}

	std::size_t NamePool::GetBlocksCount() const
	{
		return arena.GetBlocksCount();
	}

	std::size_t NamePool::GetBlockSize() const
	{
		return arena.GetBlockSize();
	}
}
