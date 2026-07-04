#pragma once

#include <cstddef>
#include <memory>
#include <vector>

namespace space_fossils::core::memory {
	class MemoryArena
	{
	private:
		struct MemoryBlock
		{
			std::unique_ptr<std::byte[]> memory;
			std::size_t capacity;
			std::size_t used;
		};

	public:
		explicit MemoryArena(std::size_t blockSize);

		MemoryArena(const MemoryArena&) = delete;
		MemoryArena& operator=(const MemoryArena&) = delete;

		void* Allocate(std::size_t requiredSize, std::size_t alignment);
		void MergeFrom(MemoryArena&& other);

		void Reset();
		void Release();

		std::size_t GetAllocatedBytes() const;
		std::size_t GetUsedSize() const;
		std::size_t GetBlocksCount() const;
		std::size_t GetBlockSize() const;

	private:
		void* TryAllocateInBlock(MemoryBlock& block, std::size_t requiredSize, std::size_t alignment);
		MemoryBlock* TryCreateNewBlock(std::size_t requiredSize, std::size_t alignment);

	private:
		const std::size_t blockSize;
		std::vector<MemoryBlock> blocks;
	};
}
