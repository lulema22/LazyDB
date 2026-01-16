#ifndef LAZYDB_INDEX_H
#define LAZYDB_INDEX_H

#include <vector>
#include <functional>
#include "core/HashTable.h"
#include "db/BTree.h"

// Ref "ссылка на строку"
template<typename K, typename Ref>
class IIndex {
public:
    virtual ~IIndex() {}
    virtual void Clear() = 0;
    // Build по набору ссылок (slot)
    virtual void Build(const std::vector<Ref>& refs,
                       const std::function<K(Ref)>& keySelector) = 0;
    virtual void Insert(const K& key, Ref ref) = 0;

    virtual std::vector<Ref> FindEquals(const K& key) const = 0;

    virtual std::vector<Ref> FindRange(const K& from, const K& to) const = 0;
};

template<typename K, typename Ref>
class HashIndex : public IIndex<K, Ref> {
public:
    HashIndex(size_t initialCapacity = 1024): map_(initialCapacity) {}

    void Clear() override {
        map_.Clear();
    }

    void Build(const std::vector<Ref>& refs,
               const std::function<K(Ref)>& keySelector) override { //refs - ссылка на список ссылок на строки таблицы
        //keySelector  это функция, которая по ссылке на строку (Ref) возвращает ключ (K), по которому строится индекс.
        Clear();
        for (const Ref& r : refs) {
            Insert(keySelector(r), r);
        }
    }

    void Insert(const K& key, Ref ref) override {
        std::vector<Ref>* vec = map_.GetPtr(key);
        if (!vec) {
            map_.Set(key, std::vector<Ref>{ref});
        } else {
            vec->push_back(ref);
        }
    }

    std::vector<Ref> FindEquals(const K& key) const override {
        auto val = map_.Get(key);
        if (!val) return {};
        return *val;
    }

    std::vector<Ref> FindRange(const K& from, const K& to) const override {
        std::vector<Ref> out;
        map_.ForEach([&](const K& k, const std::vector<Ref>& refs) {
            if (!(k < from) && !(to < k)) {
                out.insert(out.end(), refs.begin(), refs.end());
            }
        });

        return out;
    }

private:
    HashTable<K, std::vector<Ref>> map_;
};

template<typename K, typename Ref>
class BTreeIndex : public IIndex<K, Ref> {
public:
    BTreeIndex(size_t minDegree = 16)
        : tree_(minDegree) {}

    void Clear() override {
        tree_.Clear();
    }

    void Build(const std::vector<Ref>& refs,
               const std::function<K(Ref)>& keySelector) override {
        Clear();
        for (const Ref& r : refs) {
            tree_.Insert(keySelector(r), r);
        }
    }

    void Insert(const K& key, Ref ref) override {
        tree_.Insert(key, ref);
    }

    std::vector<Ref> FindEquals(const K& key) const override {
        return tree_.FindEquals(key);
    }

    std::vector<Ref> FindRange(const K& from, const K& to) const override {
        return tree_.FindRange(from, to);
    }

private:
    BTree<K, Ref> tree_;
};

#endif // LAZYDB_INDEX_H
