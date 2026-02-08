#pragma once

#include "asserts_sf.hpp"
#include "general_purpose_allocator.hpp"
#include "traits.hpp"
#include "constants.hpp"
#include "defines.hpp"
#include "memory_sf.hpp"
#include "utility.hpp"
#include "iterator.hpp"
#include "optional.hpp"
#include <cstring>
#include <string_view>
#include <type_traits>
#include <utility>

namespace sf {

template<typename K>
using HashFn = u64(*)(ConstLRefOrValType<K>);

template<typename K>
using EqualFn = bool(*)(ConstLRefOrValType<K>, ConstLRefOrValType<K>);

template<typename K>
u64 hashfn_default(ConstLRefOrValType<K> key) noexcept;

template<typename K>
bool equal_fn_default(ConstLRefOrValType<K> first, ConstLRefOrValType<K> second) {
    return first == second;
};

// fnv1a hash function
static constexpr u64 PRIME = 1099511628211ull;
static constexpr u64 OFFSET_BASIS = 14695981039346656037ull;

template<typename K>
u64 hashfn_default(ConstLRefOrValType<K> key) noexcept {
    u64 hash = OFFSET_BASIS;

    u8 bytes[sizeof(K)];
    sf_mem_copy(bytes, &key, sizeof(K));

    for (u32 i{0}; i < sizeof(K); ++i) {
        hash ^= bytes[i];
        hash *= PRIME;
    }

    return hash;
}

template<>
inline u64 hashfn_default<std::string_view>(ConstLRefOrValType<std::string_view> key) noexcept {
    u64 hash = OFFSET_BASIS;

    for (u32 i{0}; i < key.size(); ++i) {
        hash ^= key[i];
        hash *= PRIME;
    }

    return hash;
}

template<>
inline u64 hashfn_default<const char*>(ConstLRefOrValType<const char*> key) noexcept {
    u64 hash = OFFSET_BASIS;

    u32 s_len{ static_cast<u32>(strlen(key)) };

    for (u32 i{0}; i < s_len; ++i) {
        hash ^= key[i];
        hash *= PRIME;
    }

    return hash;
}

template<typename K>
struct HashMapConfig {
    HashFn<K>   hash_fn;
    EqualFn<K>  equal_fn;
    f32         load_factor;
    f32         grow_factor;

    HashMapConfig(
        HashFn<K> hash_fn = hashfn_default<K>,
        EqualFn<K> equal_fn = equal_fn_default<K>,
        f32 load_factor = 0.8f,
        f32 grow_factor = 2.0f
    )
        : hash_fn{ hash_fn }
        , equal_fn{ equal_fn }
        , load_factor{ load_factor }
        , grow_factor{ grow_factor }
    {}
};

template<typename K>
static HashMapConfig<K> get_default_config() {
    return HashMapConfig<K>{
        hashfn_default<K>,
        equal_fn_default<K>,
        0.8f,
        2.0f,
    };
}

template<typename K, typename V, AllocatorTrait Allocator = GeneralPurposeAllocator, u32 DEFAULT_INIT_CAPACITY = 32>
struct HashMap {
public:
    using KeyType = K;
    using ValueType = V;

    struct Bucket {
        K   key;
        V   value;
        u64 hash;
    };

    union Data {
        Bucket* ptr;
        u32     handle; 
    };
private:
    // 1 capacity = 1 count = sizeof(Bucket<K, V>)
    Allocator*          _allocator;
    Data                _data;
    u32                 _capacity;
    u32                 _count;
    HashMapConfig<K>    _config;
public: 
    static constexpr u64 FREE_HASH = 0;
    static constexpr u64 TOMBSTONE_HASH = 1;
    static constexpr u64 FIRST_VALID_HASH = 2;
    static constexpr bool USE_HANDLE = Allocator::using_handle();

    HashMap(const HashMapConfig<K>& config = get_default_config<K>())
        : _allocator{get_current_gpa()}
        , _capacity{DEFAULT_INIT_CAPACITY}
        , _count{0}
        , _config{config}
    {
        SF_ASSERT(config.grow_factor > 1.0f);
        resize_empty(DEFAULT_INIT_CAPACITY);
    }

    HashMap(Allocator* allocator, const HashMapConfig<K>& config = get_default_config<K>())
        : _allocator{allocator}
        , _capacity{DEFAULT_INIT_CAPACITY}
        , _count{0}
        , _config{config}
    {
        SF_ASSERT(config.grow_factor > 1.0f);
        resize_empty(DEFAULT_INIT_CAPACITY);
    }
    
    HashMap(u32 prealloc_count, Allocator* allocator, const HashMapConfig<K>& config = get_default_config<K>())
        : _allocator{allocator}
        , _capacity{next_power_of_2(prealloc_count)}
        , _count{0}
        , _config{config}
    {
        SF_ASSERT(config.grow_factor > 1.0f);
        resize_empty(prealloc_count);
    }

    HashMap(HashMap<K, V, Allocator>&& rhs) noexcept
        : _allocator{rhs._allocator}
        , _data{rhs._data}
        , _capacity{rhs._capacity}
        , _count{rhs._count}
        , _config{rhs._config}
    {
        rhs._allocator = nullptr;
        if constexpr (USE_HANDLE) {
            rhs._data.handle = INVALID_ALLOC_HANDLE;
        } else {
            rhs._data.ptr = nullptr;
        }
        rhs._buffer = nullptr;
        rhs._capacity = 0;
        rhs._count = 0;
    }

    HashMap<K, V, Allocator>& operator=(HashMap<K, V, Allocator>&& rhs) noexcept
    {
        if (this == &rhs) {
            return *this;
        }

        Bucket* data = access_data();
        if (_allocator && data) {
            _allocator->free(data);
        }
        
        _allocator = rhs._allocator;
        _data = rhs._data;
        _capacity = rhs._capacity;
        _count = rhs._count;
        _config = rhs._config;

        rhs._allocator = nullptr;
        if constexpr (USE_HANDLE) {
            rhs._data.handle = INVALID_ALLOC_HANDLE;
        } else {
            rhs._data.ptr = nullptr;
        }
        rhs._buffer = nullptr;
        rhs._capacity = 0;
        rhs._count = 0;
    }
    
    HashMap(const HashMap<K, V, Allocator>& rhs) = delete;
    HashMap<K, V, Allocator>& operator=(const HashMap<K, V, Allocator>& rhs) = delete;

    ~HashMap() noexcept {
        free();
    }

    void free() noexcept {
        if constexpr (USE_HANDLE) {
            if (_data.handle != INVALID_ALLOC_HANDLE) {
                if constexpr (std::is_destructible_v<K> || std::is_destructible_v<V>) {
                    for (auto it{begin()}; it != end(); ++it) {
                        if (it->hash >= FIRST_VALID_HASH) {
                            if constexpr (std::is_destructible_v<K>) {
                                it->key.~K();
                            }
                            if constexpr (std::is_destructible_v<V>) {
                                it->value.~V();
                            }
                        }
                    }
                }
                _allocator->free_handle(_data.handle, alignof(Bucket));
                _data.handle = INVALID_ALLOC_HANDLE;
            }
        } else {
            if (_data.ptr) {
                if constexpr (std::is_destructible_v<K> || std::is_destructible_v<V>) {
                    for (auto it{begin()}; it != end(); ++it) {
                        if (it->hash >= FIRST_VALID_HASH) {
                            if constexpr (std::is_destructible_v<K>) {
                                it->key.~K();
                            }
                            if constexpr (std::is_destructible_v<V>) {
                                it->value.~V();
                            }
                        }
                    }
                }
                _allocator->free(_data.ptr, alignof(Bucket));
                _data.ptr = nullptr;
            }
        }
        _count = 0;
        _capacity = 0;
    }

    void clear() noexcept {
        if constexpr (USE_HANDLE) {
            if (_data.handle != INVALID_ALLOC_HANDLE) {
                for (auto it{begin()}; it != end(); ++it) {
                    if (it->hash >= FIRST_VALID_HASH) {
                        it->hash = FREE_HASH;
                        if constexpr (std::is_destructible_v<K>) {
                            it->key.~K();
                        }
                        if constexpr (std::is_destructible_v<V>) {
                            it->value.~V();
                        }
                    }
                }
            }
        } else {
            if (_data.ptr) {
                for (auto it{begin()}; it != end(); ++it) {
                    if (it->hash >= FIRST_VALID_HASH) {
                        it->hash = FREE_HASH;
                        if constexpr (std::is_destructible_v<K>) {
                            it->key.~K();
                        }
                        if constexpr (std::is_destructible_v<V>) {
                            it->value.~V();
                        }
                    }
                }
            }
        }
        _count = 0;
   }

    void set_allocator(Allocator* alloc) noexcept {
        SF_ASSERT_MSG(alloc, "Should be valid pointer");
        _allocator = alloc;
    }

    // updates entry with the same key
    template<typename Key, typename Val>
    requires SameTypes<K, Key> && SameTypes<V, Val>
    void put(Key&& key, Val&& val) noexcept {
        SF_ASSERT_MSG(_allocator, "Should be valid pointer");
        if (_count >= static_cast<u32>(_capacity * _config.load_factor)) {
            resize(_capacity * _config.grow_factor);
        }
        put_inner(std::forward<Key&&>(key), std::forward<Val&&>(val));
    }

    template<typename Key, typename Val>
    requires SameTypes<K, Key> && SameTypes<V, Val>
    void put_without_realloc(Key&& key, Val&& val) noexcept {
        SF_ASSERT_MSG(_allocator, "Should be valid pointer");
        SF_ASSERT_MSG(_count < static_cast<u32>(_capacity * _config.load_factor), "Should have empty space");
        put_inner(std::forward<Key&&>(key), std::forward<Val&&>(val));
    }

    // put without update
    template<typename Key, typename Val>
    requires SameTypes<K, Key> && SameTypes<V, Val>
    bool put_if_empty(Key&& key, Val&& val) noexcept {
        SF_ASSERT_MSG(_allocator, "Should be valid pointer");
        if (_count >= static_cast<u32>(_capacity * _config.load_factor)) {
            resize(_capacity * _config.grow_factor);
        }

        Bucket* data = access_data();
        u64 hash = hash_inner();
        u32 index = index_hash(hash);

        for (u32 i = index; i < _capacity; ++i) {
            if (data[i].hash < FIRST_VALID_HASH) {
                ::new (&data[i]) Bucket{ .key = std::forward<Key&&>(key), .value = std::forward<Val&&>(val), .hash = hash };
                ++_count;
                return true;
            }
            else if (hash == data[i].hash && _config.equal_fn(key, data[i].key)) {
                return false;
            }
        }

        for (u32 i = 0; i < index; ++i) {
            if (data[i].hash < FIRST_VALID_HASH) {
                ::new (&data[i]) Bucket{ .key = std::forward<Key&&>(key), .value = std::forward<Val&&>(val), .hash = hash };
                ++_count;
                return true;
            }
            else if (hash == data[i].hash && _config.equal_fn(key, data[i].key)) {
                return false;
            }
        }
    }

    Option<V*> get(ConstLRefOrValType<K> key) noexcept {
        Option<Bucket*> maybe_bucket = find_bucket(key);
        if (maybe_bucket.is_none()) {
            return {None::VALUE};
        }

        return &maybe_bucket.unwrap_ref()->value;
    }

    bool remove(ConstLRefOrValType<K> key) noexcept {
        Option<Bucket*> maybe_bucket = find_bucket(key);
        if (maybe_bucket.is_none()) {
            return false;
        }

        Bucket* bucket = maybe_bucket.unwrap_copy();
        if constexpr (std::is_destructible_v<K>) {
            bucket->key.~K();
        }
        if constexpr (std::is_destructible_v<V>) {
            bucket->value.~V();
        }
        
        bucket->hash = FREE_HASH;
        --_count;

        return true;
    }

    void reserve(u32 new_capacity) noexcept {
        SF_ASSERT_MSG(_allocator, "Allocator should be set");

        if (is_empty()) {
            resize_empty(new_capacity);
        } else {
            resize(new_capacity);
        }
    }

    void fill(ConstLRefOrValType<V> val) noexcept {
        for (auto it{begin()}; it != end(); ++it) {
            it->value = val;
        }
        _count = _capacity;
    }

    bool is_empty() const {
        if constexpr (USE_HANDLE) {
            return _data.handle == INVALID_ALLOC_HANDLE || _capacity == 0 || _count == 0;
        } else {
            return _data.ptr == nullptr || _capacity == 0 || _count == 0;
        }
    }

    constexpr u32 count() const noexcept { return _count; }
    constexpr u32 size_in_bytes() const noexcept { return sizeof(Bucket) * _count; }
    constexpr u32 capacity() const noexcept { return _capacity; }
    constexpr u32 capacity_remain() const noexcept { return _capacity - _count; }

    constexpr PtrRandomAccessIterator<Bucket> begin() noexcept {
        return PtrRandomAccessIterator<Bucket>(access_data());
    }

    constexpr PtrRandomAccessIterator<Bucket> end() noexcept {
        return PtrRandomAccessIterator<Bucket>(access_data() + _capacity);
    }
private:
    Bucket* access_data() {
        if constexpr (USE_HANDLE) {
            return static_cast<Bucket*>(_allocator->handle_to_ptr(_data.handle));
        } else {
            return _data.ptr;
        }
    }

    void resize_empty(u32 new_capacity) {
        SF_ASSERT_MSG(_allocator, "Should be valid pointer");
        _capacity = new_capacity == 0 ? DEFAULT_INIT_CAPACITY : new_capacity;

        if constexpr (USE_HANDLE) {
            _data.handle = _allocator->allocate_handle(_capacity * sizeof(Bucket), alignof(Bucket));
            init_buffer_empty(access_data(), _capacity);
        } else {
            _data.ptr = static_cast<Bucket*>(_allocator->allocate(_capacity * sizeof(Bucket), alignof(Bucket)));
            init_buffer_empty(_data.ptr, _capacity);
        }
    }

    void resize(u32 new_capacity) noexcept {
        SF_ASSERT_MSG(_allocator, "Should be valid pointer");

        u32 old_capacity = _capacity;
        if (_capacity == 0) {
            _capacity = std::max(DEFAULT_INIT_CAPACITY, new_capacity);
        }
        while (_capacity < new_capacity) {
            _capacity *= _config.grow_factor;
        }

        Bucket* old_buffer = access_data();
        Bucket* new_buffer = (Bucket*)_allocator->allocate(_capacity * sizeof(Bucket), alignof(Bucket));
        init_buffer_empty(new_buffer, _capacity);

        // copy old nodes
        for (u32 i{0}; i < old_capacity; ++i) {
            Bucket&& bucket{ std::move(old_buffer[i]) };
            if (bucket.hash < FIRST_VALID_HASH) {
                continue;
            }
            put_old_entry(new_buffer, std::forward<K&&>(bucket.key), std::forward<V&&>(bucket.value));
        }

        if constexpr (USE_HANDLE) {
            _data.handle = _allocator->ptr_to_handle(new_buffer);
        } else {
            _data.ptr = new_buffer;
        }

        _allocator->free(old_buffer, alignof(Bucket));
    }

    void init_buffer_empty(Bucket* new_buffer, u32 capacity) {
        if (!new_buffer) {
            return;
        }

        sf_mem_zero(new_buffer, capacity * sizeof(Bucket));
    }

    // returns "some" if only state of bucket is "FILLED"
    Option<Bucket*> find_bucket(ConstLRefOrValType<K> key) noexcept {
        u64 hash = hash_inner(key);
        u32 index = index_hash(hash);
        u32 search_count = 0;

        Bucket* data = access_data();
        for (u32 i = index; i < _capacity; ++i) {
            if (data[i].hash >= FIRST_VALID_HASH) {
                if (data[i].hash == hash && _config.equal_fn(key, data[i].key)) {
                    return {data + i};
                }
            } else if (data[i].hash == FREE_HASH) {
                return {None::VALUE};
            }
        }

        for (u32 i = 0; i < index; ++i) {
            if (data[i].hash >= FIRST_VALID_HASH) {
                if (data[i].hash == hash && _config.equal_fn(key, data[i].key)) {
                    return {data + i};
                }
            } else if (data[i].hash == FREE_HASH) {
                return {None::VALUE};
            }
        }

        return {None::VALUE};
    }

    template<typename Key, typename Val>
    requires SameTypes<K, Key> && SameTypes<V, Val>
    void put_inner(Key&& key, Val&& val) noexcept {
        Bucket* data = access_data();
        u64 hash = hash_inner(key);
        u32 index = index_hash(hash);

        for (u32 i = index; i < _capacity; ++i) {
            if (data[i].hash < FIRST_VALID_HASH) {
                ::new (&data[i]) Bucket{ .key = std::forward<Key&&>(key), .value = std::forward<Val&&>(val), .hash = hash };
                ++_count;
                return;
            }
            else if (hash == data[i].hash && _config.equal_fn(key, data[i].key)) {
                data[i].value = std::forward<Val&&>(val);
                return;
            }
        }

        for (u32 i = 0; i < index; ++i) {
            if (data[i].hash < FIRST_VALID_HASH) {
                ::new (&data[i]) Bucket{ .key = std::forward<Key&&>(key), .value = std::forward<Val&&>(val), .hash = hash };
                ++_count;
                return;
            }
            else if (hash == data[i].hash && _config.equal_fn(key, data[i].key)) {
                data[i].value = std::forward<Val&&>(val);
                return;
            }
        }
    }

    // used inside 'resize' for copying old entries
    template<typename Key, typename Val>
    requires SameTypes<K, Key> && SameTypes<V, Val>
    void put_old_entry(Bucket* new_buffer, Key&& key, Val&& val) noexcept {
        u64 hash = hash_inner(key);
        u32 index = index_hash(hash);

        for (u32 i = index; i < _capacity; ++i) {
            if (new_buffer[i].hash < FIRST_VALID_HASH) {
                ::new (&new_buffer[i]) Bucket{ .key = std::forward<Key&&>(key), .value = std::forward<Val&&>(val), .hash = hash };
                return;
            }
        }

        for (u32 i = 0; i < index; ++i) {
            if (new_buffer[i].hash < FIRST_VALID_HASH) {
                ::new (&new_buffer[i]) Bucket{ .key = std::forward<Key&&>(key), .value = std::forward<Val&&>(val), .hash = hash };
                return;
            }
        }

        SF_ASSERT_MSG(false, "Should not get here");
    }

    u64 hash_inner(ConstLRefOrValType<K> key) {
        return std::max(_config.hash_fn(key), FIRST_VALID_HASH);
    }

    u32 index_hash(u64 hash) {
        return hash & (_capacity - 1);
    }
};

} // sf
