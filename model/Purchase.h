#ifndef LAZYDB_PURCHASE_H
#define LAZYDB_PURCHASE_H

#include <string>

class Purchase {
public:
    Purchase() = default;
    Purchase(int id, std::string date, int deptId, int supplierId, int productId, int qty, double unitPrice);

    int GetId() const { return id_; }
    const std::string& GetDate() const { return date_; }
    int GetDeptId() const { return deptId_; }
    int GetSupplierId() const { return supplierId_; }
    int GetProductId() const { return productId_; }
    int GetQty() const { return qty_; }
    double GetUnitPrice() const { return unitPrice_; }

    static Purchase FromCSV(const std::string& line);

private:
    int id_ = 0;
    std::string date_; // "YYYY-MM-DD"
    int deptId_ = 0;
    int supplierId_ = 0;
    int productId_ = 0;
    int qty_ = 0;
    double unitPrice_ = 0.0;
};

#endif // LAZYDB_PURCHASE_H
