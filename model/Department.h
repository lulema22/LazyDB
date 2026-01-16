#ifndef LAZYDB_DEPARTMENT_H
#define LAZYDB_DEPARTMENT_H

#include <string>

class Department {
public:
    Department() = default;
    Department(int id, std::string name, int addressId);

    int GetId() const { return id_; }
    const std::string& GetName() const { return name_; }
    int GetAddressId() const { return addressId_; }

    static Department FromCSV(const std::string& line);

private:
    int id_ = 0;
    std::string name_;
    int addressId_ = 0;
};

#endif // LAZYDB_DEPARTMENT_H
