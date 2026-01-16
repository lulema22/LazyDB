#ifndef LAZYDB_BTREE_H
#define LAZYDB_BTREE_H

#include <vector>
#include <memory>
#include <algorithm>


template<typename K, typename Ref> // k- тип ключа ref  ссылка на запись таблицы
class BTree {
public:
    // t = минимальная степень B-Tree
    // max keys = 2t-1, max children = 2t
    BTree(int minDegree = 16) {
        if (minDegree < 2) {
            t_ = 2; // так как постоянно один ключ и будет все время делиться
        } else {
            t_ = minDegree;
        }
        root_ = std::make_unique<Node>(true); //Каждый узел принадлежит ровно одному родителю и не прописывать вручную делет
    }
    void Clear() {
        root_ = std::make_unique<Node>(true);
    }

    void Insert(const K& key, Ref ref) {
        if (root_->keys.size() == MaxKeys()) {
            auto newRoot = std::make_unique<Node>(false);
            newRoot->children.push_back(std::move(root_));
            SplitChild(*newRoot, 0);
            root_ = std::move(newRoot);
        }
        InsertNonFull(*root_, key, ref);
    }
    // все записи ключ равен key
    std::vector<Ref> FindEquals(const K& key) const {
        const std::vector<Ref>* vec = FindPtr(*root_, key);
        if (!vec) return {};
        return *vec; // копия
    }

    std::vector<Ref> FindRange(const K& from, const K& to) const {
        std::vector<Ref> out;
        RangeCollect(*root_, from, to, out);
        return out;
    }

private:
    //узел дерева
    struct Node {
        Node(bool leaf) : leaf(leaf) {}

        bool leaf;
        std::vector<K> keys;
        std::vector<std::vector<Ref>> values;  // values[i] соответствует keys[i]
        std::vector<std::unique_ptr<Node>> children; // если !leaf, то children.size() == keys.size()+1
    };
    size_t t_;
    std::unique_ptr<Node> root_;

    size_t MaxKeys() const {return 2*t_-1;}



    //не привязана к конкретному объекту BTree
    static int lbIndex(const std::vector<K>& keys, const K& key) {
        auto it = std::lower_bound(keys.begin(), keys.end(), key);
        int pos = it - keys.begin();
        return pos;

    }

    void SplitChild(Node& parent, size_t i) {
        Node* y = parent.children[i].get();     // переполненный ребёнок
        auto z = std::make_unique<Node>(y->leaf);    // новый правый узел
        int mid = t_-1;  // индекс медианы

        // медиана  поднимем в parent
        K medianKey = y->keys[mid];
        std::vector<Ref> medianVal = std::move(y->values[mid]);
        //перенесём правую часть в z
        for (size_t j = mid + 1; j < y->keys.size(); j++) {
            z->keys.push_back(y->keys[j]);
            z->values.push_back(std::move(y->values[j]));
        }
        //  y не лист зн правых детей в z
        if (!y->leaf) {
            for (size_t j = mid+1; j < y->children.size(); ++j) {
                z->children.push_back(std::move(y->children[j]));
            }
        }
        // y до левой части
        y->keys.resize(mid);
        y->values.resize(mid);
        if (!y->leaf) {
            y->children.resize(mid + 1);
        }
        //вставим медиану в родителя и подключим z как ребёнка справа
        parent.keys.insert(parent.keys.begin() + i, std::move(medianKey));
        parent.values.insert(parent.values.begin() + i, std::move(medianVal));
        parent.children.insert(parent.children.begin() + (i + 1), std::move(z)); // типо сосед справа от того кого делили
    }


    void InsertNonFull(Node& x, const K& key, Ref ref) {
        // если ключ уже есть в этом узле - просто добавляем значение и выходим
        size_t pos = lbIndex(x.keys, key);  // позиция, где ключ должен стоять
        if (pos < x.keys.size() && x.keys[pos] == key) {
            x.values[pos].push_back(ref);
            return;
        }
        //  лист  вставляем новый ключ прямо сюда
        if (x.leaf) {
            x.keys.insert(x.keys.begin() + pos, key);
            x.values.insert(x.values.begin() + pos, std::vector<Ref>{ref});
            return;
        }
        //  не лист  спускаемся в ребёнка с индексом pos
        size_t i = pos;

        //  ребёнок полный  делим его
        if (x.children[i]->keys.size() == MaxKeys()) {
            SplitChild(x, i);

            // После split в x.keys[i] теперь стоит медиана.
            // Нужно понять, куда теперь идти: влево или вправо, или ключ == медиане.
            //уже указывает на левого ребенка
            if (key > x.keys[i]) {
                i = i + 1; // идём в правого ребёнка
            } else if (key == x.keys[i]) {
                x.values[i].push_back(ref);
                return;
            }
        }
        //  продолжаем вставку ниже
        InsertNonFull(*x.children[i], key, ref); //при разыменовывании указателя мы получаем ссылку на ноду
    }

    const std::vector<Ref>* FindPtr(const Node& x, const K& key) const {
        const size_t i = lbIndex(x.keys, key);
        if (i < x.keys.size() && x.keys[i] == key) {
            return &x.values[i];
        }
        if (x.leaf) return nullptr;
        return FindPtr(*x.children[i], key);
    }

    void RangeCollect(const Node& x, const K& from, const K& to, std::vector<Ref>& out) const {
        // идём по ключам в узле слева направо
        for (size_t i = 0; i < x.keys.size(); ++i) {

            // перед ключом i есть ребёнок i сначала обходим его (если это не лист)
            if (!x.leaf) {
                RangeCollect(*x.children[i], from, to, out);
            }
            // если ключ попадает в добавляем все его значения
            if (x.keys[i] >= from && x.keys[i] <= to) {
                out.insert(out.end(), x.values[i].begin(), x.values[i].end());
            }

            // если текущий ключ уже больше to  дальше смысла нет
            if (x.keys[i] > to) {
                return;
            }
        }

        // после последнего ключа есть последний ребёнок (keys.size()) из-за того что у него K+1 ребенок
        if (!x.leaf) {
            RangeCollect(*x.children[x.keys.size()], from, to, out);
        }
    }

};

#endif // LAZYDB_BTREE_H
