#ifndef LAZYDB_TABLE_H
#define LAZYDB_TABLE_H

#include <fstream>
#include <functional>
#include <string>
#include <vector>
#include <type_traits>
#include <sstream>
#include <stdexcept>
#include "core/HashTable.h"
#include "db/DbErrors.h"

template<typename T, typename IdT>
class Table {
public:
    Table() {}

    const std::string& GetTableName() const {return tableName_;} //получить имя табл

    template<typename IdGetter>
    static Table LoadFromFile(const std::string& path,const std::string& tableName,IdGetter idGetter)
    {
        Table t;
        t.tableName_ = tableName;
        t.idGetter_ = idGetter;
       // idGetter передаётся извне


        std::ifstream in(path); //Открываем файл
        if (!in) {
            return t;
        }

        std::string line;
        int rowIndex = 0;

        while (std::getline(in, line)) {
            if (line.empty()) {
                rowIndex++;
                continue;
            }
            T row = T::FromCSV(line); //разбирает CSV-строку и создаёт объект типа T
            t.InsertInternal(row, rowIndex, true);
            rowIndex++; //Увеличиваем номер строки файла
        }
        return t;
    }


    size_t GetRowCount() const {return aliveCount_;}

    const T& GetRow(size_t aliveIndex) const {
        size_t slot = AliveIndexToSlot(aliveIndex);
        return records_[slot];
    }

    T& GetRow(size_t aliveIndex) {
        size_t slot = AliveIndexToSlot(aliveIndex);
        return records_[slot];
    }

    bool ContainsId(const IdT& id) const {
        return pkIndex_.ContainsKey(id);
    }

    void Insert(const T& row) {
        InsertInternal(row, -1, true);
    }

    bool DeleteById(const IdT& id) {
        size_t* slotPtr = pkIndex_.TryGet(id);
        if (!slotPtr) return false;

        const size_t slot = *slotPtr;
        if (slot >= alive_.size() || !alive_[slot]) {
            pkIndex_.Remove(id); //чистим индекс
            return false;
        }
        //помечаем как удалённую
        //уменьшаем число живых
        //слот кладём в freeList (можно повторно использовать при вставке)
        alive_[slot] = 0;
        aliveCount_--;
        freeList_.push_back(slot);
        pkIndex_.Remove(id);
        return true;
    }

    bool UpdateById(const IdT& id, const T& newRow) {
        size_t* slotPtr = pkIndex_.TryGet(id);
        if (!slotPtr) return false; // строки с таким id нет

        size_t slot = *slotPtr;
        if (slot >= alive_.size() || !alive_[slot]) return false; // строка удалена

        // запрещаем менять первичный ключ
        if (idGetter_(newRow) != id) return false; // не даёт изменить первичный ключ при обновлении строки

        records_[slot] = newRow; // обновляем данные
        return true;
    }

public:
    // slot = индекс в records
    bool IsAliveSlot(size_t slot) const {
        return slot < alive_.size() && alive_[slot];
    }

    const T& GetRowBySlot(size_t slot) const {
        if (!IsAliveSlot(slot)) {
            throw std::runtime_error("GetRowBySlot: slot is not alive");

        }
        return records_[slot];
    }

    std::vector<size_t> GetAliveSlots() const {
        std::vector<size_t> slots;
        slots.reserve(aliveCount_);
        for (size_t i = 0; i < alive_.size(); ++i) {
            if (alive_[i]) slots.push_back(i);
        }
        return slots;
    }


private:
    std::string tableName_ = "table";
    std::vector<T> records_;
    std::vector<uint8_t> alive_;
    std::vector<size_t> freeList_;
    size_t aliveCount_ = 0; //живых строк.

    HashTable<IdT, size_t> pkIndex_{1024};
    std::function<IdT(const T&)> idGetter_;


    void InsertInternal(const T& row, int rowIndexForError, bool throwOnPkDuplicate) { // функция вставки строки в таблицу.
        IdT id = idGetter_(row); //Получаем первичный ключ строки
        if (pkIndex_.ContainsKey(id)) {
            return; // просто не вставляем дубликат
        }
        size_t slot;
        if (!freeList_.empty()) {
            slot = freeList_.back();
            freeList_.pop_back();
            records_[slot] = row;
            alive_[slot] = 1;
        } else {
            slot = records_.size();
            records_.push_back(row);
            alive_.push_back(1);
        }
        pkIndex_.Add(id, slot);
        aliveCount_++;
    }

    size_t AliveIndexToSlot(size_t aliveIndex) const { //Какой реальный индекс в records_ соответствует живой строке
        size_t count = 0;
        for (size_t slot = 0; slot < alive_.size(); ++slot) {
            if (alive_[slot]) {
                if (count == aliveIndex) return slot;
                count++;
            }
        }
        return 0;
    }


};

#endif // LAZYDB_TABLE_H
