#include "model/Employee.h"

#include <sstream>
#include <vector>
#include <stdexcept>

static std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) parts.push_back(item);
    return parts;
}

Employee::Employee(int id, std::string last, std::string first, std::string middle, int birthYear, int deptId)
    : id_(id),
      last_(std::move(last)),
      first_(std::move(first)),
      middle_(std::move(middle)),
      birthYear_(birthYear),
      deptId_(deptId) {}

std::string Employee::GetFullName() const {
    return last_ + " " + first_ + " " + middle_;
}

Employee Employee::FromCSV(const std::string& line) {
    auto p = split(line, ';');
    if (p.size() < 6) throw std::runtime_error("Bad Employee CSV line: " + line);
    return Employee(std::stoi(p[0]), p[1], p[2], p[3], std::stoi(p[4]), std::stoi(p[5]));
}
