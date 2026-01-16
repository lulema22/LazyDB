#ifndef LAZYDB_HASHTABLE_H
#define LAZYDB_HASHTABLE_H

#include <vector>
#include <functional>
#include <optional>
#include <utility>
#include <stdexcept>

template<typename K, typename V>
class HashTable {
public:
    struct KeyValue {
        K key;
        V value;
    };

    using Bucket = std::vector<KeyValue>;
    HashTable(size_t capacity = 16, double maxLoadFactor = 0.75) {
        if (capacity < 1) {
            capacity = 1;
        }
        buckets_.resize(capacity);
        count_ = 0;
        maxLoadFactor_ = maxLoadFactor;
    }



    size_t Size() const { return count_; }
    size_t Capacity() const { return buckets_.size(); }

    void Clear() {
        for (auto& b : buckets_) b.clear();
        count_ = 0;
    }

    bool Contains(const K& key) const {
        return GetPtr(key) != nullptr;
    }

    std::optional<V> Get(const K& key) const {
        const V* p = GetPtr(key);
        if (!p) return std::nullopt;
        return *p;
    }


    V* GetPtr(const K& key) {
        if (buckets_.empty()) return nullptr;
        auto& bucket = buckets_[bucketIndex(key, buckets_.size())];
        for (auto& kv : bucket) {
            if (kv.key == key) return &kv.value;
        }
        return nullptr;
    }

    const V* GetPtr(const K& key) const {
        if (buckets_.empty()) return nullptr;
        const auto& bucket = buckets_[bucketIndex(key, buckets_.size())];
        for (const auto& kv : bucket) {
            if (kv.key == key) return &kv.value;
        }
        return nullptr;
    }

    void Set(const K& key, V value) {
        if (V* existing = GetPtr(key)) {
            *existing = std::move(value);
            return;
        }

        MaybeRehashForInsert();
        auto& bucket = buckets_[bucketIndex(key, buckets_.size())];
        bucket.push_back(KeyValue{key, std::move(value)});
        ++count_;
    }

    bool Remove(const K& key) {
        if (buckets_.empty()) return false;
        auto& bucket = buckets_[bucketIndex(key, buckets_.size())];
        for (size_t i = 0; i < bucket.size(); ++i) {
            if (bucket[i].key == key) {
                bucket[i] = std::move(bucket.back());
                bucket.pop_back();
                --count_;
                return true;
            }
        }
        return false;
    }

    template<typename F>
    void ForEach(F&& fn) const {
        for (const auto& bucket : buckets_) {
            for (const auto& kv : bucket) {
                fn(kv.key, kv.value);
            }
        }
    }

    bool ContainsKey(const K& key) const { return Contains(key); }
    void Add(const K& key, const V& value) {Set(key, value); }
    void Add(const K& key, V&& value) {Set(key, std::move(value)); }

    V* TryGet(const K& key) { return GetPtr(key); }
    const V* TryGet(const K& key) const { return GetPtr(key); }

    bool TryGet(const K& key, V& out) const {
        const V* p = GetPtr(key);
        if (!p) return false;
        out = *p;
        return true;
    }

    bool RemoveKey(const K& key) { return Remove(key); }

    size_t GetCount() const { return Size(); }
    size_t GetCapacity() const { return Capacity(); }

private:
    std::vector<Bucket> buckets_;
    size_t count_;
    double maxLoadFactor_;

    static size_t bucketIndex(const K& key, size_t cap) {
        return std::hash<K>{}(key) % cap;
    }

    void MaybeRehashForInsert() {
        size_t cap = buckets_.size();
        if (cap == 0) {
            cap = 1;
        }
        double loadFactor = double(count_ + 1) / double(cap);

        if (loadFactor > maxLoadFactor_) {
            Rehash(cap * 2);   // увеличиваем таблицу в 2 раза
        }
    }


    void Rehash(size_t newCapacity) {
        if (newCapacity < 1) newCapacity = 1;
        std::vector<Bucket> newBuckets(newCapacity);

        for (const auto& bucket : buckets_) {
            for (const auto& kv : bucket) {
                const size_t idx = bucketIndex(kv.key, newCapacity);
                newBuckets[idx].push_back(kv);
            }
        }
        buckets_ = std::move(newBuckets);
        // count_ не меняется
    }
};

#endif // LAZYDB_HASHTABLE_H
