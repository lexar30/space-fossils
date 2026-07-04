#include "space_fossils/memory/memory_arena.hxx"
#include "space_fossils_tests/micro_test_framework.hxx"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

namespace space_fossils::tests {
	namespace {
		using namespace space_fossils::core::memory;

		bool IsAligned(const void* ptr, std::size_t alignment)
		{
			return reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0;
		}
	}

	SF_TEST(memory_arena, StartsEmpty)
	{
		MemoryArena arena(64);

		SF_ASSERT_EQ(arena.GetBlockSize(), 64);
		SF_ASSERT_EQ(arena.GetBlocksCount(), 0);
		SF_ASSERT_EQ(arena.GetAllocatedBytes(), 0);
		SF_ASSERT_EQ(arena.GetUsedSize(), 0);
	}

	SF_TEST(memory_arena, AllocateCreatesBlockAndTracksUsedBytes)
	{
		MemoryArena arena(64);

		void* allocation = arena.Allocate(8, 1);

		SF_ASSERT_EQ(allocation != nullptr, true);
		SF_ASSERT_EQ(arena.GetBlocksCount(), 1);
		SF_ASSERT_EQ(arena.GetAllocatedBytes(), 64);
		SF_ASSERT_EQ(arena.GetUsedSize(), 8);
	}

	SF_TEST(memory_arena, AllocateHonorsAlignment)
	{
		MemoryArena arena(128);

		void* allocation = arena.Allocate(1, 64);

		SF_ASSERT_EQ(allocation != nullptr, true);
		SF_ASSERT_EQ(IsAligned(allocation, 64), true);
	}

	SF_TEST(memory_arena, AllocateCreatesNewBlockWhenCurrentBlockIsFull)
	{
		MemoryArena arena(16);

		void* first = arena.Allocate(16, 1);
		void* second = arena.Allocate(1, 1);

		SF_ASSERT_EQ(first != nullptr, true);
		SF_ASSERT_EQ(second != nullptr, true);
		SF_ASSERT_EQ(arena.GetBlocksCount(), 2);
		SF_ASSERT_EQ(arena.GetAllocatedBytes(), 32);
		SF_ASSERT_EQ(arena.GetUsedSize(), 17);
	}

	SF_TEST(memory_arena, ResetKeepsBlocksAndClearsUsedBytes)
	{
		MemoryArena arena(16);

		arena.Allocate(16, 1);
		arena.Allocate(1, 1);

		arena.Reset();

		SF_ASSERT_EQ(arena.GetBlocksCount(), 2);
		SF_ASSERT_EQ(arena.GetAllocatedBytes(), 32);
		SF_ASSERT_EQ(arena.GetUsedSize(), 0);

		void* firstReusedBlockAllocation = arena.Allocate(16, 1);
		void* secondReusedBlockAllocation = arena.Allocate(16, 1);

		SF_ASSERT_EQ(firstReusedBlockAllocation != nullptr, true);
		SF_ASSERT_EQ(secondReusedBlockAllocation != nullptr, true);
		SF_ASSERT_EQ(arena.GetBlocksCount(), 2);
		SF_ASSERT_EQ(arena.GetUsedSize(), 32);

		void* newBlockAllocation = arena.Allocate(1, 1);

		SF_ASSERT_EQ(newBlockAllocation != nullptr, true);
		SF_ASSERT_EQ(arena.GetBlocksCount(), 3);
		SF_ASSERT_EQ(arena.GetUsedSize(), 33);
	}

	SF_TEST(memory_arena, ReleaseClearsBlocks)
	{
		MemoryArena arena(16);

		arena.Allocate(16, 1);
		arena.Allocate(1, 1);

		arena.Release();

		SF_ASSERT_EQ(arena.GetBlocksCount(), 0);
		SF_ASSERT_EQ(arena.GetAllocatedBytes(), 0);
		SF_ASSERT_EQ(arena.GetUsedSize(), 0);
	}

	SF_TEST(memory_arena, MergeFromMovesBlocksAndLeavesSourceEmpty)
	{
		MemoryArena target(64);
		MemoryArena source(16);

		std::byte* storedByte = static_cast<std::byte*>(source.Allocate(8, alignof(std::byte)));
		SF_ASSERT_EQ(storedByte != nullptr, true);
		storedByte[0] = std::byte { 42 };

		target.MergeFrom(std::move(source));

		SF_ASSERT_EQ(target.GetBlocksCount(), 1);
		SF_ASSERT_EQ(target.GetAllocatedBytes(), 16);
		SF_ASSERT_EQ(target.GetUsedSize(), 8);
		SF_ASSERT_EQ(source.GetBlocksCount(), 0);
		SF_ASSERT_EQ(source.GetAllocatedBytes(), 0);
		SF_ASSERT_EQ(source.GetUsedSize(), 0);
		SF_ASSERT_EQ(storedByte[0] == std::byte { 42 }, true);
	}

	SF_TEST(memory_arena, InvalidRequestsReturnNullWithoutAllocatingBlocks)
	{
		MemoryArena arena(64);

		SF_ASSERT_EQ(arena.Allocate(0, 1), nullptr);
		SF_ASSERT_EQ(arena.Allocate(1, 0), nullptr);
		SF_ASSERT_EQ(arena.Allocate(1, 3), nullptr);
		SF_ASSERT_EQ(arena.GetBlocksCount(), 0);
	}

	SF_TEST(memory_arena, OverflowingBlockCapacityReturnsNullWithoutAllocatingBlocks)
	{
		MemoryArena arena(64);

		void* allocation = arena.Allocate(std::numeric_limits<std::size_t>::max(), 2);

		SF_ASSERT_EQ(allocation, nullptr);
		SF_ASSERT_EQ(arena.GetBlocksCount(), 0);
	}

	SF_TEST(memory_arena, FailedLargeAllocationDoesNotAdvanceCurrentBlock)
	{
		MemoryArena arena(16);

		void* firstAllocation = arena.Allocate(8, 1);
		void* failedAllocation = arena.Allocate(std::numeric_limits<std::size_t>::max(), 2);
		void* secondAllocation = arena.Allocate(8, 1);

		SF_ASSERT_EQ(firstAllocation != nullptr, true);
		SF_ASSERT_EQ(failedAllocation, nullptr);
		SF_ASSERT_EQ(secondAllocation != nullptr, true);
		SF_ASSERT_EQ(arena.GetBlocksCount(), 1);
		SF_ASSERT_EQ(arena.GetUsedSize(), 16);
	}
}
