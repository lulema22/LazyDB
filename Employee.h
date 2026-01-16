#ifndef LAZYDB_EMPLOYEE_H
#define LAZYDB_EMPLOYEE_H

#include <string>

class Employee {
public:
    Employee() = default;
    Employee(int id, std::string last, std::string first, std::string middle, int birthYear, int deptId);

    int GetId() const { return id_; }
    const std::string& GetLast() const { return last_; }
    const std::string& GetFirst() const { return first_; }
    const std::string& GetMiddle() const { return middle_; }
    int GetBirthYear() const { return birthYear_; }
    int GetDeptId() const { return deptId_; }

    std::string GetFullName() const;
    static Employee FromCSV(const std::string& line);

private:
    int id_ = 0;
    std::string last_;
    std::string first_;
    std::string middle_;
    int birthYear_ = 0;
    int deptId_ = 0;
};

#endif // LAZYDB_EMPLOYEE_H
