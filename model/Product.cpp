#include "model/Product.h"

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

Product::Product(int id, std::string name, std::string category, std::string unit, int defaultSupplierId)
    : id_(id),
      name_(std::move(name)),
      category_(std::move(category)),
      unit_(std::move(unit)),
      defaultSupplierId_(defaultSupplierId) {}

Product Product::FromCSV(const std::string& line) {
    auto p = split(line, ';');
    if (p.size() < 5) throw std::runtime_error("Bad Product CSV line: " + line);
    return Product(std::stoi(p[0]), p[1], p[2], p[3], std::stoi(p[4]));
}
