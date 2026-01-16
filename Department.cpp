#include "model/Department.h"

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

Department::Department(int id, std::string name, int addressId)
    : id_(id), name_(std::move(name)), addressId_(addressId) {}

Department Department::FromCSV(const std::string& line) {
    auto p = split(line, ';');
    if (p.size() < 3) throw std::runtime_error("Bad Department CSV line: " + line);
    return Department(std::stoi(p[0]), p[1], std::stoi(p[2]));
}
