#include "model/Address.h"

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

Address::Address(int id, std::string city, std::string street, std::string building, std::string type)
    : id_(id),
      city_(std::move(city)),
      street_(std::move(street)),
      building_(std::move(building)),
      type_(std::move(type)) {}

Address Address::FromCSV(const std::string& line) {
    auto p = split(line, ';');
    if (p.size() < 4) throw std::runtime_error("Bad Address CSV line: " + line);

    // формат ожидаем: id;city;street;building;type
    std::string type = (p.size() >= 5 ? p[4] : "Unknown");
    if (type.empty()) type = "Unknown";

    return Address(
        std::stoi(p[0]),
        p[1],
        p[2],
        p[3],
        type
    );
}
