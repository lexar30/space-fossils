#include "space_fossils/core/memory/memory_arena.hxx"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

namespace space_fossils::core::memory {
	MemoryArena::MemoryArena(std::size_t blockSize)
		: blockSize(blockSize)
	{
	}

	void* MemoryArena::Allocate(std::size_t requiredSize, std::size_t alignment)
	{
		if (blockSize == 0 || requiredSize == 0 || alignment == 0) {
			return nullptr;
		}

		if ((alignment & (alignment - 1)) != 0) {
			return nullptr;
		}

		std::size_t candidateBlockIndex = currentBlockIndex;
		while (candidateBlockIndex < blocks.size()) {
			void* ptr = TryAllocateInBlock(blocks[candidateBlockIndex], requiredSize, alignment);
			if (ptr != nullptr) {
				currentBlockIndex = candidateBlockIndex;
				return ptr;
			}

			++candidateBlockIndex;
		}

		MemoryBlock* newBlock = TryCreateNewBlock(requiredSize, alignment);
		if (newBlock == nullptr) {
			return nullptr;
		}

		currentBlockIndex = blocks.size() - 1;
		return TryAllocateInBlock(*newBlock, requiredSize, alignment);
	}

	void MemoryArena::MergeFrom(MemoryArena&& other)
	{
		if (this == &other) {
			return;
		}

		for (auto& blockIt : other.blocks) {
			blocks.push_back(std::move(blockIt));
		}

		other.blocks.clear();
		other.currentBlockIndex = 0;
	}

	void MemoryArena::Reset()
	{
		for (auto& blockIt : blocks) {
			blockIt.used = 0;
		}

		currentBlockIndex = 0;
	}

	void MemoryArena::Release()
	{
		blocks.clear();
		currentBlockIndex = 0;
	}

	std::size_t MemoryArena::GetAllocatedBytes() const
	{
		std::size_t totalSize = 0;
		for (const auto& blockIt : blocks) {
			totalSize += blockIt.capacity;
		}

		return totalSize;
	}

	std::size_t MemoryArena::GetUsedSize() const
	{
		std::size_t totalSize = 0;
		for (const auto& blockIt : blocks) {
			totalSize += blockIt.used;
		}

		return totalSize;
	}

	std::size_t MemoryArena::GetBlocksCount() const
	{
		return blocks.size();
	}

	std::size_t MemoryArena::GetBlockSize() const
	{
		return blockSize;
	}

	void* MemoryArena::TryAllocateInBlock(MemoryBlock& block, std::size_t requiredSize, std::size_t alignment)
	{
		void* currentFreePtr = block.memory.get() + block.used;

		std::size_t spaceLeft = block.capacity - block.used;
		if (spaceLeft < requiredSize) {
			return nullptr;
		}

		void* alignedPtr = std::align(alignment, requiredSize, currentFreePtr, spaceLeft);
		if (alignedPtr == nullptr) {
			return nullptr;
		}

		std::size_t bytesUsedFromStart = static_cast<std::byte*>(alignedPtr) - block.memory.get();
		block.used = bytesUsedFromStart + requiredSize;

		return alignedPtr;
	}

	MemoryArena::MemoryBlock* MemoryArena::TryCreateNewBlock(std::size_t requiredSize, std::size_t alignment)
	{
		if (requiredSize > std::numeric_limits<std::size_t>::max() - (alignment - 1)) {
			return nullptr;
		}

		MemoryBlock newBlock;

		std::size_t requiredCapacity = std::max(blockSize, requiredSize + alignment - 1);
		newBlock.memory = std::unique_ptr<std::byte[]>(new std::byte[requiredCapacity]);
		newBlock.capacity = requiredCapacity;
		newBlock.used = 0;

		blocks.push_back(std::move(newBlock));

		return &blocks.back();
	}
}
