#ifndef LAZYDB_SUPPLIER_H
#define LAZYDB_SUPPLIER_H

#include <string>

class Supplier {
public:
    Supplier() = default;
    Supplier(int id, std::string name, std::string city, std::string phone, std::string email);

    int GetId() const { return id_; }
    const std::string& GetName() const { return name_; }
    const std::string& GetCity() const { return city_; }
    const std::string& GetPhone() const { return phone_; }
    const std::string& GetEmail() const { return email_; }

    static Supplier FromCSV(const std::string& line);

private:
    int id_ = 0;
    std::string name_;
    std::string city_;
    std::string phone_;
    std::string email_;
};

#endif // LAZYDB_SUPPLIER_H
