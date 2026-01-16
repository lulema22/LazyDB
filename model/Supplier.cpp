#include "model/Supplier.h"

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

Supplier::Supplier(int id, std::string name, std::string city, std::string phone, std::string email)
    : id_(id),
      name_(std::move(name)),
      city_(std::move(city)),
      phone_(std::move(phone)),
      email_(std::move(email)) {}

Supplier Supplier::FromCSV(const std::string& line) {
    auto p = split(line, ';');
    if (p.size() < 5) throw std::runtime_error("Bad Supplier CSV line: " + line);
    return Supplier(std::stoi(p[0]), p[1], p[2], p[3], p[4]);
}
