#include <cstdlib>
#ifdef SF_TESTS

#include "bitset.hpp"
#include "general_purpose_allocator.hpp"
#include "linear_allocator.hpp"
#include "hashmap.hpp"
#include "dynamic_array.hpp"
#include "logger.hpp"
#include "test_manager.hpp"
#include "fixed_array.hpp"
#include "free_list_allocator.hpp"
#include "stack_allocator.hpp"
#include <string_view>
#include <chrono>
#include <unordered_map>

namespace sf {

void TestManager::add_test(TestFn test) { 
    module_tests.append(test);
}

void TestManager::run_all_tests() const {
    for (const auto& m_test : module_tests) {
        m_test();
    }
}

TestCounter::TestCounter(std::string_view name)
    : test_name{ name }
{
    LOG_TEST("Tests for {} started: ", name);
}

TestCounter::~TestCounter() {
    LOG_TEST("Tests for {} ended:\t\n{} all, passed: {}, failed: {}", test_name, all, passed, failed);
}

auto now() {
    return std::chrono::system_clock::now();
}

Perf::Perf(std::string_view name)
    : name{ name }
    , start_time{ now() } 
{
    LOG_TEST("Performance test starts for: \"{}\"", name);
}

Perf::~Perf()
{
    auto time = now() - start_time;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(time).count();
    LOG_TEST("Performance test ends for: \"{}\"\n\tresult : {}ms", name, ms);
}
// Tests

struct Resource {
    int* ptr;

    Resource(): ptr{nullptr} {}

    Resource(int* ptr) {
        this->ptr = ptr;
    }

    ~Resource() {
        printf("Destructor %p (%d)\n", ptr, ptr ? *ptr : 0);
        if (ptr) {
            delete ptr;
            ptr = nullptr;
        }
    }

    Resource(Resource&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    Resource& operator=(Resource&& other) noexcept {
        if (this != &other) {
            if (ptr) {
                printf("Deleting %p (%d)\n", ptr, ptr ? *ptr : 0);
                delete ptr;
            }
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    friend bool operator==(const Resource& rhs, const Resource& lhs) {
        return rhs.ptr == lhs.ptr;
    }
};

void fixed_array_test() {
    {
        TestCounter counter("Fixed Array 1");
        FixedArray<const char*, 10> arr{ "hello", "world", "crazy", "boy" };

        expect(arr.capacity() == 10u, counter);
        expect(arr.count() == 4u, counter);

        arr.append_emplace("many");
        arr.append_emplace("items");

        expect(arr.capacity() == 10u, counter);
        expect(arr.count() == 6u, counter);

        arr.remove_unordered_at(5);
        arr.remove_unordered_at(4);

        expect(arr.capacity() == 10u, counter);
        expect(arr.count() == 4u, counter);
    }
    {
        TestCounter counter("Fixed Array 2");

        FixedArray<Resource, 10> arr{};
        for (u32 i{0}; i < arr.capacity(); ++i) {
            arr.append_emplace(new int(i));
        }

        arr.remove_at(3);
        arr.remove_at(4);

        expect(arr.count() == arr.capacity() - 2, counter);
        
        arr.remove_unordered_at(3);
        arr.remove_unordered_at(4);

        expect(arr.count() == arr.capacity() - 4, counter);
    }
}

void dyn_array_test() {
    TestCounter counter{"DynamicArray"};
    GeneralPurposeAllocator gpa{};
    DynamicArray<Resource, GeneralPurposeAllocator> arr{&gpa};

    for (u32 i{0}; i < 20; ++i) {
        arr.append_emplace(new int(i));
    }

    expect(*arr[0].ptr == 0, counter);
    expect(*arr[11].ptr == 11, counter);
    expect(*arr[18].ptr == 18, counter);

    arr.remove_unordered_at(5);
    arr.remove_unordered_at(8);
    arr.remove_at(2);
    arr.remove_at(10);

    expect(arr.count() == 20 - 4, counter);

    arr.append(new int(99999));
    arr.append(new int(165666));

    expect(arr.count() == 20 - 4 + 2, counter);
}

void string_test() {
    TestCounter counter{"FixedString"};
    FixedString<100> str{"hello \t\n \n\t "}; 
    str.trim_end();
    str.append_sv("_world!");
    expect(str == "hello_world!", counter);
}

void stack_allocator_test() {
    TestCounter counter("Stack Allocator");
    StackAllocator alloc{500};

    {
        DynamicArray<u8, StackAllocator> arr(&alloc);
        arr.reserve(200);
        expect(alloc.count() >= 200 * sizeof(u8) + sizeof(StackAllocatorHeader), counter);

        DynamicArray<u8, StackAllocator> arr2(&alloc);
        arr2.reserve(200);
        expect(alloc.count() >= 400 * sizeof(u8) + sizeof(StackAllocatorHeader), counter);

        DynamicArray<u8, StackAllocator> arr3(&alloc);
        arr3.reserve(300);
        expect(alloc.count() >= 700 * sizeof(u8) + sizeof(StackAllocatorHeader), counter);
    }
}

void freelist_allocator_test() {
    FreeList alloc{600};

    DynamicArray<u8, FreeList<true>> arr1(512, &alloc);
    for (usize i{0}; i < 256; ++i) {
        arr1.append(i);
    }
    LOG_TEST("random item: {}", arr1[rand() % arr1.count()]);
    arr1.resize(600);

    DynamicArray<u8, FreeList<true>> arr2(512, &alloc);
    for (usize i{256}; i < 512; ++i) {
        arr2.append(i);
    }
    LOG_TEST("random item: {}", arr2[rand() % arr2.count()]);
    arr2.resize(1200);

    arr1.free();
    arr2.free();
}

void linear_allocator_test() {
    TestCounter counter("Linear Allocator");
    LinearAllocator alloc{500};

    {
        DynamicArray<u8, LinearAllocator> arr(&alloc);
        arr.reserve(200);
        expect(alloc.count() >= 200 * sizeof(u8), counter);

        DynamicArray<u8, LinearAllocator> arr2(&alloc);
        arr2.reserve(200);
        expect(alloc.count() >= 400 * sizeof(u8), counter);

        DynamicArray<u8, LinearAllocator> arr3(&alloc);
        arr3.reserve(300);
        expect(alloc.count() >= 700 * sizeof(u8), counter);
    }
}

void hashmap_test() {
    {
        TestCounter counter("HashMap");
        using MapType = HashMap<std::string_view, usize, LinearAllocator>;
        LinearAllocator alloc(1024 * sizeof(MapType::Bucket));
        MapType map{&alloc}; 
        map.set_allocator(&alloc);
        map.reserve(32);

        std::string_view key1 = "kate_age";
        std::string_view key2 = "paul_age";
    
        map.put(key1, 18ul);
        map.put(key2, 20ul);

        auto age1 = map.get(key1);
        auto age2 = map.get(key2);

        expect(age1, counter);
        expect(age2, counter);

        auto del1 = map.remove(key1);
        auto del2 = map.remove(key1);
        auto del3 = map.remove(key2);

        expect(del1, counter);
        expect(!del2, counter);
        expect(del3, counter);

        map.fill(999ul);
        for (auto& item : map) {
            expect(item.value == 999ul, counter);
        }
    }

    {
        TestCounter counter("HashMap 2");
        using MapType = HashMap<usize, Resource, LinearAllocator>;
        LinearAllocator alloc(1024 * sizeof(MapType::Bucket));
        MapType map{&alloc}; 
        map.reserve(20);

        constexpr u32 COUNT{5};

        for (usize i{0}; i < COUNT; ++i) {
            map.put(i, Resource(new int(i)));
        }

        expect(map.count() == COUNT, counter);

        for (usize i{0}; i < 2; ++i) {
            map.remove(i);
        }

        expect(map.count() == COUNT - 2, counter);
    }
}

void hashmap_test_compare_std()
{
    TestCounter counter("HashMap comparison with std");
    constexpr u32 TEST_COUNT = 100'000'000;

    LinearAllocator alloc_buff{TEST_COUNT * sizeof(i32)};

    DynamicArray<int, LinearAllocator> keys{TEST_COUNT, &alloc_buff};
    for (int i = 0; i < TEST_COUNT;++i) {
        keys.append(rand());
    }

    std::unordered_map<i32, i32> mapstd;
    HashMap<i32, i32> map{};

// Putting
    {
        Perf perf{ "My map put" };
        for (int i = 0; i < TEST_COUNT;++i) {
            map.put(keys[i], i);
        }
    }

    {
        Perf perf{ "STD map put" };
        for (int i = 0; i < TEST_COUNT;++i) {
            mapstd[keys[i]] = keys[i];
        }
    }

// Getting
    {
        Perf perf{ "My map get" };
        for (int i = 0; i < TEST_COUNT;++i) {
            i32* j = map.get(keys[i]);
            if (j && (*j == -999'999'999)) {
                LOG_TEST("not important");
            }
        }
    }

    {
        Perf perf{ "STD map get" };
        for (int i = 0; i < TEST_COUNT;++i) {
            i32& j = mapstd.at(keys[i]);
            if (j == -999'999'999) {
                LOG_TEST("not important");
            }
        }
    }
    
// Removing
    {
        Perf perf{ "My map remove" };
        for (int i = 0; i < TEST_COUNT;++i) {
            map.remove(keys[i]);
        }
    }

    {
        Perf perf{ "STD map remove" };
        for (int i = 0; i < TEST_COUNT;++i) {
            mapstd.erase(keys[i]);
        }
    }

    printf("Capacity my: %d\n", map.count());
    printf("Capacity std: %zu\n", mapstd.size());
    printf("End\n");
}


static constexpr u32 MAX_STR_LEN = 32;

void gen_str(FixedString<MAX_STR_LEN>& out_s) {
    out_s.resize_to_capacity();
    for (u32 i = 0; i < out_s.count(); ++i) {
        out_s[i] = static_cast<char>(rand() % ('z' - 'a') + 'a');
    }
}

FixedString<MAX_STR_LEN>& random_key(DynamicArray<FixedString<MAX_STR_LEN>, LinearAllocator>& keys) {
    return keys[rand() % keys.count()];
}

void hashmap_test_strings()
{
    TestCounter counter("HashMap string test");
    constexpr u32 KEY_COUNT = 50'000'000;

    LinearAllocator alloc_buff{KEY_COUNT * sizeof(FixedString<MAX_STR_LEN>)};

    DynamicArray<FixedString<MAX_STR_LEN>, LinearAllocator> keys{KEY_COUNT, &alloc_buff};
    keys.resize_to_capacity();

    for (int i = 0; i < KEY_COUNT;++i) {
        gen_str(keys[i]);
    }

    HashMap<FixedString<MAX_STR_LEN>, int> map{};

// Putting
    {
        Perf perf{ "My map put" };
        for (int i = 0; i < KEY_COUNT;++i) {
            map.put(keys[i], i);
        }
    }

// Getting
    {
        Perf perf{ "My map get" };
        for (int i = 0; i < KEY_COUNT;++i) {
            i32* val = map.get(random_key(keys));
            if (val && *val == 999'999'999) {
                LOG_TEST("not important");
            }
        }
    }

// Removing
    {
        Perf perf{ "My map remove" };
        for (int i = 0; i < KEY_COUNT;++i) {
            map.remove(random_key(keys));
        }
    }

    printf("Capacity my: %d\n", map.count());
    printf("End Hashmap string test\n");
}

void bitset_test() {
    TestCounter counter{"BitSet"};
    BitSet<256> bitset{};

    bitset.set_bit(2);
    bitset.set_bit(18);
    bitset.set_bit(34);
    bitset.set_bit(56);
    bitset.set_bit(112);
    bitset.set_bit(213);

    expect(bitset.is_bit(2), counter);
    expect(bitset.is_bit(18), counter);
    expect(bitset.is_bit(34), counter);
    expect(bitset.is_bit(56), counter);
    expect(bitset.is_bit(112), counter);
    expect(bitset.is_bit(213), counter);
    expect(bitset.is_bit(118) == false, counter);
    expect(bitset.is_bit(35) == false, counter);
    expect(bitset.is_bit(218) == false, counter);
    expect(bitset.is_bit(59) == false, counter);

    bitset.unset_bit(2);
    bitset.unset_bit(18);
    bitset.unset_bit(34);
    bitset.unset_bit(56);
    bitset.unset_bit(112);
    bitset.unset_bit(213);

    expect(bitset.is_bit(2) == false, counter);
    expect(bitset.is_bit(18) == false, counter);
    expect(bitset.is_bit(34) == false, counter);
    expect(bitset.is_bit(56) == false, counter);
    expect(bitset.is_bit(112) == false, counter);
    expect(bitset.is_bit(213) == false, counter);

    bitset.toggle_bit(2);
    bitset.toggle_bit(18);
    bitset.toggle_bit(34);
    bitset.toggle_bit(56);
    bitset.toggle_bit(112);
    bitset.toggle_bit(213);

    expect(bitset.is_bit(2), counter);
    expect(bitset.is_bit(18), counter);
    expect(bitset.is_bit(34), counter);
    expect(bitset.is_bit(56), counter);
    expect(bitset.is_bit(112), counter);
    expect(bitset.is_bit(213), counter);
}

void TestManager::collect_all_tests() {
    module_tests.append(fixed_array_test);
    module_tests.append(dyn_array_test);
    module_tests.append(hashmap_test);
    module_tests.append(hashmap_test_compare_std);
    module_tests.append(hashmap_test_strings);
    module_tests.append(string_test);
    module_tests.append(linear_allocator_test);
    module_tests.append(stack_allocator_test);
    module_tests.append(bitset_test);
    // module_tests.append(freelist_allocator_test);
}

} // sf

#endif
