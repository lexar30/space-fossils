#pragma once

#include "space_fossils/core/file_tree/model/types.hxx"
#include "space_fossils/core/memory/memory_arena.hxx"

#include <cstddef>

namespace space_fossils::core::file_tree {
	class NamePool
	{
	public:
		explicit NamePool(std::size_t blockSize);

		NamePool(const NamePool&) = delete;
		NamePool& operator=(const NamePool&) = delete;

		NameRef Store(NativeStringView name);
		void MergeFrom(NamePool&& other);

		void Reset();
		void Release();

		std::size_t GetAllocatedBytes() const;
		std::size_t GetUsedBytes() const;
		std::size_t GetBlocksCount() const;
		std::size_t GetBlockSize() const;

	private:
		memory::MemoryArena arena;
	};
}
