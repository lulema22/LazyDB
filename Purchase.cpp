#include "model/Purchase.h"

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

Purchase::Purchase(int id, std::string date, int deptId, int supplierId, int productId, int qty, double unitPrice)
    : id_(id),
      date_(std::move(date)),
      deptId_(deptId),
      supplierId_(supplierId),
      productId_(productId),
      qty_(qty),
      unitPrice_(unitPrice) {}

Purchase Purchase::FromCSV(const std::string& line) {
    auto p = split(line, ';');
    if (p.size() < 7) throw std::runtime_error("Bad Purchase CSV line: " + line);

    return Purchase(
        std::stoi(p[0]),
        p[1],
        std::stoi(p[2]),
        std::stoi(p[3]),
        std::stoi(p[4]),
        std::stoi(p[5]),
        std::stod(p[6])
    );
}
