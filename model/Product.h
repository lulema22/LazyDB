#ifndef LAZYDB_PRODUCT_H
#define LAZYDB_PRODUCT_H

#include <string>

class Product {
public:
    Product() = default;
    Product(int id, std::string name, std::string category, std::string unit, int defaultSupplierId);

    int GetId() const { return id_; }
    const std::string& GetName() const { return name_; }
    const std::string& GetCategory() const { return category_; }
    const std::string& GetUnit() const { return unit_; }
    int GetDefaultSupplierId() const { return defaultSupplierId_; }

    static Product FromCSV(const std::string& line);

private:
    int id_ = 0;
    std::string name_;
    std::string category_;
    std::string unit_;
    int defaultSupplierId_ = 0;
};

#endif // LAZYDB_PRODUCT_H
