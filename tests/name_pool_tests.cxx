#include "space_fossils/core/file_tree/memory/name_pool.hxx"

#include "space_fossils_tests/micro_test_framework.hxx"

#include <utility>

namespace space_fossils::tests {
	namespace {
		using namespace space_fossils::core::file_tree;

		NativeString MakeNativeString(const char* value)
		{
			NativeString result;
			while (*value != '\0') {
				result.push_back(static_cast<NativeChar>(*value));
				++value;
			}

			return result;
		}
	}

	SF_TEST(name_pool, StartsEmpty)
	{
		NamePool pool(64);

		SF_ASSERT_EQ(pool.GetBlockSize(), 64);
		SF_ASSERT_EQ(pool.GetBlocksCount(), 0);
		SF_ASSERT_EQ(pool.GetAllocatedBytes(), 0);
		SF_ASSERT_EQ(pool.GetUsedBytes(), 0);
	}

	SF_TEST(name_pool, StoresAndReadsName)
	{
		NamePool pool(64);
		NativeString name = MakeNativeString("alpha.txt");

		NameRef ref = pool.Store(name);
		NativeStringView storedName = ToStringView(ref);

		SF_ASSERT_EQ(ref.data != nullptr, true);
		SF_ASSERT_EQ(ref.length, name.size());
		SF_ASSERT_EQ(storedName.size(), name.size());
		SF_ASSERT_EQ(storedName[0] == name[0], true);
		SF_ASSERT_EQ(storedName[5] == name[5], true);
		SF_ASSERT_EQ(storedName[8] == name[8], true);
	}

	SF_TEST(name_pool, StoresNameInOwnedMemory)
	{
		NamePool pool(64);
		NativeString name = MakeNativeString("old");

		NameRef ref = pool.Store(name);
		name[0] = static_cast<NativeChar>('n');

		NativeStringView storedName = ToStringView(ref);

		SF_ASSERT_EQ(storedName[0] == static_cast<NativeChar>('o'), true);
		SF_ASSERT_EQ(storedName[1] == static_cast<NativeChar>('l'), true);
		SF_ASSERT_EQ(storedName[2] == static_cast<NativeChar>('d'), true);
	}

	SF_TEST(name_pool, EmptyNameDoesNotAllocate)
	{
		NamePool pool(64);

		NameRef ref = pool.Store({});

		SF_ASSERT_EQ(ref.data == nullptr, true);
		SF_ASSERT_EQ(ref.length, 0);
		SF_ASSERT_EQ(ToStringView(ref).size(), 0);
		SF_ASSERT_EQ(pool.GetBlocksCount(), 0);
	}

	SF_TEST(name_pool, CreatesNewBlockWhenCurrentBlockIsFull)
	{
		NamePool pool(sizeof(NativeChar) * 2);
		NativeString first = MakeNativeString("ab");
		NativeString second = MakeNativeString("c");

		NameRef firstRef = pool.Store(first);
		NameRef secondRef = pool.Store(second);

		SF_ASSERT_EQ(firstRef.data != nullptr, true);
		SF_ASSERT_EQ(secondRef.data != nullptr, true);
		SF_ASSERT_EQ(pool.GetBlocksCount(), 2);
		SF_ASSERT_EQ(ToStringView(firstRef).size(), 2);
		SF_ASSERT_EQ(ToStringView(secondRef).size(), 1);
	}

	SF_TEST(name_pool, ResetKeepsBlocksAndClearsUsedBytes)
	{
		NamePool pool(sizeof(NativeChar) * 2);
		pool.Store(MakeNativeString("ab"));
		pool.Store(MakeNativeString("c"));

		pool.Reset();

		SF_ASSERT_EQ(pool.GetBlocksCount(), 2);
		SF_ASSERT_EQ(pool.GetUsedBytes(), 0);

		NameRef ref = pool.Store(MakeNativeString("d"));

		SF_ASSERT_EQ(ref.data != nullptr, true);
		SF_ASSERT_EQ(pool.GetBlocksCount(), 2);
		SF_ASSERT_EQ(pool.GetUsedBytes(), sizeof(NativeChar));
	}

	SF_TEST(name_pool, ReleaseClearsBlocks)
	{
		NamePool pool(64);
		pool.Store(MakeNativeString("alpha"));

		pool.Release();

		SF_ASSERT_EQ(pool.GetBlocksCount(), 0);
		SF_ASSERT_EQ(pool.GetAllocatedBytes(), 0);
		SF_ASSERT_EQ(pool.GetUsedBytes(), 0);
	}

	SF_TEST(name_pool, MergeFromMovesStoredNamesAndLeavesSourceEmpty)
	{
		NamePool target(64);
		NamePool source(16);
		NativeString name = MakeNativeString("merged");

		NameRef ref = source.Store(name);

		target.MergeFrom(std::move(source));

		SF_ASSERT_EQ(target.GetBlocksCount(), 1);
		SF_ASSERT_EQ(target.GetAllocatedBytes(), 16);
		// MergeFrom defines the moved-from source as empty.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 26800)
#endif
		SF_ASSERT_EQ(source.GetBlocksCount(), 0);
		SF_ASSERT_EQ(source.GetAllocatedBytes(), 0);
		SF_ASSERT_EQ(source.GetUsedBytes(), 0);
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
		SF_ASSERT_EQ(ToStringView(ref)[0] == static_cast<NativeChar>('m'), true);
		SF_ASSERT_EQ(ToStringView(ref).size(), name.size());
	}

	SF_TEST(name_pool, MergeFromAppendsStoredNamesToNonEmptyTarget)
	{
		NamePool target(16);
		NamePool source(16);
		NativeString targetName = MakeNativeString("target");
		NativeString sourceName = MakeNativeString("source");

		NameRef targetRef = target.Store(targetName);
		NameRef sourceRef = source.Store(sourceName);

		target.MergeFrom(std::move(source));

		SF_ASSERT_EQ(target.GetBlocksCount(), 2);
		// MergeFrom defines the moved-from source as empty.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 26800)
#endif
		SF_ASSERT_EQ(source.GetBlocksCount(), 0);
		SF_ASSERT_EQ(source.GetAllocatedBytes(), 0);
		SF_ASSERT_EQ(source.GetUsedBytes(), 0);
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
		SF_ASSERT_EQ(ToStringView(targetRef).size(), targetName.size());
		SF_ASSERT_EQ(ToStringView(sourceRef).size(), sourceName.size());
		SF_ASSERT_EQ(ToStringView(targetRef)[0] == static_cast<NativeChar>('t'), true);
		SF_ASSERT_EQ(ToStringView(sourceRef)[0] == static_cast<NativeChar>('s'), true);
	}
}
