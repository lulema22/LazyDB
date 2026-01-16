#ifndef LAZYDB_DBERRORS_H
#define LAZYDB_DBERRORS_H

#include <stdexcept>
#include <string>


enum class DbConstraintType {PrimaryKeyDuplicate,UniqueViolation,ForeignKeyViolation,RestrictViolation,IoOrParseError
};

inline std::string ToString(DbConstraintType t) {
    switch(t){
        case DbConstraintType::PrimaryKeyDuplicate: return "PK_DUPLICATE"; //уже существующим id
        case DbConstraintType::UniqueViolation: return "UNIQUE"; //нарушили уникальность поля (например, два department с одинаковым name)
        case DbConstraintType::ForeignKeyViolation:return "FK";  //ссылка на несуществующую строку
        case DbConstraintType::RestrictViolation: return "RESTRICT"; //попытка удалить строку, на которую кто-то ссылается
        case DbConstraintType::IoOrParseError:return "IO_OR_PARSE"; //ошибка чтения файла или парсинга CSV
        default: return "UNKNOWN";
    }
}

class DbConstraintError : public std::runtime_error { //тип ошибки что не атк в каком месте и тд
// наследник стандартной ошибки
public:
    DbConstraintError(DbConstraintType type,std::string message,std::string table = {},std::string field = {},int rowIndex = -1)
        : std::runtime_error(std::move(message)), type_(type), table_(std::move(table)),
          field_(std::move(field)),rowIndex_(rowIndex) {}
    //Создай ошибку с сообщением + типом + контекстом

    DbConstraintType GetType() const {return type_;}
    const std::string& GetTable() const {return table_;}
    const std::string& GetField() const {return field_; }
    int GetRowIndex() const {return rowIndex_; }

    static DbConstraintError PrimaryKeyDuplicate(std::string table, std::string field, std::string value, int rowIndex) {
        return DbConstraintError(
            DbConstraintType::PrimaryKeyDuplicate,
            "PK duplicate: " + table + "." + field + "=" + value,
            std::move(table), std::move(field), rowIndex
        );
    }

    static DbConstraintError Unique(std::string table, std::string field, std::string value, int rowIndex) {
        return DbConstraintError(
            DbConstraintType::UniqueViolation,
            "UNIQUE violation: " + table + "." + field + " value='" + value + "' already exists",
            std::move(table), std::move(field), rowIndex
        );
    }
    static DbConstraintError ForeignKey(std::string table, std::string field, std::string value,
                                        std::string refTable, std::string refField,int rowIndex) {
        return DbConstraintError(
            DbConstraintType::ForeignKeyViolation,
            "FK violation: "+table + "."+field + "=" + value +
            "not found in " + refTable + "." + refField,
            std::move(table), std::move(field), rowIndex
        );
    }
    static DbConstraintError Restrict(std::string table, std::string field,std::string value,
                                      std::string refTable, std::string refField,int rowIndex) {
        return DbConstraintError(
            DbConstraintType::RestrictViolation,
            "RESTRICT violation: cannot delete " + refTable + "." + refField + "=" + value +
            " because it is referenced by " +table + "." +field,
            std::move(table), std::move(field), rowIndex
        );
    }
private:
    DbConstraintType type_;
    std::string table_; //Имя таблицы, где произошла ошибка. (employees)
    std::string field_; //Имя поля (колонки), из-за которого возникла (id)
    int rowIndex_ = -1; //Номер строки (обычно в файле или в таблице), где обнаружена ошибка.
};

#endif // LAZYDB_DBERRORS_H
