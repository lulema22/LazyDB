#ifndef LAZYDB_DATABASE_H
#define LAZYDB_DATABASE_H

#include <string>
#include <vector>
#include <stdexcept>
#include "db/Table.h"
#include "db/DbErrors.h"
#include "db/Index.h"
#include "core/HashTable.h"
#include "model/Address.h"
#include "model/Department.h"
#include "model/Employee.h"
#include "model/Supplier.h"
#include "model/Product.h"
#include "model/Purchase.h"

class Database {
public:
    static Database LoadFromFiles(const std::string& addressesPath, const std::string& departmentsPath,const std::string& employeesPath,
        const std::string& suppliersPath,const std::string& productsPath,const std::string& purchasesPath)
    {
        Database db;
        db.addresses_ = Table<Address, int>::LoadFromFile(
            addressesPath, "addresses",[](const Address& a) {return a.GetId();}
        );
        db.suppliers_ = Table<Supplier, int>::LoadFromFile(
            suppliersPath, "suppliers", [](const Supplier& s) {return s.GetId();}
        );
        db.products_ = Table<Product, int>::LoadFromFile(
            productsPath, "products",[](const Product& p) {return p.GetId();}
        );
        db.ValidateUniqueSupplierNames();
        db.ValidateUniqueProductNames();
        db.ValidateProductsDefaultSupplierFk();

        db.departments_ = Table<Department, int>::LoadFromFile(
            departmentsPath, "departments",[](const Department& d) {return d.GetId();}
        );

        db.ValidateUniqueDepartmentNames();
        db.ValidateDepartmentsAddressFk();

        db.employees_ = Table<Employee, int>::LoadFromFile(
            employeesPath, "employees",[](const Employee& e) {return e.GetId();}
        );

        db.ValidateEmployeesDeptFk();

        db.purchases_ = Table<Purchase, int>::LoadFromFile(
            purchasesPath, "purchases",[](const Purchase& p) {return p.GetId();}
        );

        db.ValidatePurchasesFk();
        db.BuildIndexes();

        return db;
    }
//     departments ссылается на addresses (address_id)
// employees ссылается на departments (dept_id)
// products ссылается на suppliers (default_supplier_id)
// purchases ссылается на dept/supplier/product


    //геттеры таблиц
    const Table<Address, int>& Addresses() const {return addresses_;}
    const Table<Department, int>& Departments() const {return departments_;}
    const Table<Employee, int>& Employees() const {return employees_;}
    const Table<Supplier, int>& Suppliers() const {return suppliers_;}
    const Table<Product, int>& Products() const {return products_;}
    const Table<Purchase, int>& Purchases() const {return purchases_;}



    using Slot = size_t;
    // Addresses
    //Найди через индекс  получи слоты →преврати в id  верни пользователю
    std::vector<int> FindAddressIdsByCity(const std::string& city) const {
        return SlotsToIds(addresses_, addressesByCity_.FindEquals(city));
    }
    std::vector<int> FindAddressIdsByIdRange(int fromId, int toId) const {
        return SlotsToIds(addresses_, addressesById_.FindRange(fromId, toId));
    }

    // Departments
    std::vector<int> FindDepartmentIdsByName(const std::string& name) const {
        return SlotsToIds(departments_, departmentsByName_.FindEquals(name));
    }
    std::vector<int> FindDepartmentIdsByAddressId(int addressId) const {
        return SlotsToIds(departments_, departmentsByAddressId_.FindEquals(addressId));
    }

    // Employees
    std::vector<int> FindEmployeeIdsByFullName(const std::string& fullName) const {
        return SlotsToIds(employees_, employeesByFullName_.FindEquals(fullName));
    }
    std::vector<int> FindEmployeeIdsByBirthYearRange(int y1, int y2) const {
        return SlotsToIds(employees_, employeesByBirthYear_.FindRange(y1, y2));
    }
    std::vector<int> FindEmployeeIdsByDeptId(int deptId) const {
        return SlotsToIds(employees_, employeesByDeptId_.FindEquals(deptId));
    }

    // Suppliers
    std::vector<int> FindSupplierIdsByName(const std::string& name) const {
        return SlotsToIds(suppliers_, suppliersByName_.FindEquals(name));
    }
    std::vector<int> FindSupplierIdsByCity(const std::string& city) const {
        return SlotsToIds(suppliers_, suppliersByCity_.FindEquals(city));
    }

    // Products
    std::vector<int> FindProductIdsByName(const std::string& name) const {
        return SlotsToIds(products_, productsByName_.FindEquals(name));
    }
    std::vector<int> FindProductIdsByDefaultSupplierId(int supplierId) const {
        return SlotsToIds(products_, productsByDefaultSupplierId_.FindEquals(supplierId));
    }

    // Purchases
    std::vector<int> FindPurchaseIdsByDateRange(const std::string& from, const std::string& to) const {
        return SlotsToIds(purchases_, purchasesByDate_.FindRange(from, to));
    }
    std::vector<int> FindPurchaseIdsBySupplierId(int supplierId) const {
        return SlotsToIds(purchases_, purchasesBySupplierId_.FindEquals(supplierId));
    }
    std::vector<int> FindPurchaseIdsByProductId(int productId) const {
        return SlotsToIds(purchases_, purchasesByProductId_.FindEquals(productId));
    }
    std::vector<int> FindPurchaseIdsByDeptId(int deptId) const {
        return SlotsToIds(purchases_, purchasesByDeptId_.FindEquals(deptId));
    }

    // Построение всех индексов (вызывать после загрузки / после массовых правок)
    void BuildIndexes() {
        // Addresses
        addressesByCity_.Build(addresses_.GetAliveSlots(), [&](Slot s) {return addresses_.GetRowBySlot(s).GetCity();});
        addressesById_.Build(addresses_.GetAliveSlots(), [&](Slot s) {return addresses_.GetRowBySlot(s).GetId();});

        // Departments
        departmentsByName_.Build(departments_.GetAliveSlots(), [&](Slot s) {return departments_.GetRowBySlot(s).GetName();});
        departmentsByAddressId_.Build(departments_.GetAliveSlots(), [&](Slot s) {return departments_.GetRowBySlot(s).GetAddressId();});

        // Employees
        employeesByFullName_.Build(employees_.GetAliveSlots(), [&](Slot s) {return employees_.GetRowBySlot(s).GetFullName();});
        employeesByBirthYear_.Build(employees_.GetAliveSlots(), [&](Slot s) {return employees_.GetRowBySlot(s).GetBirthYear();});
        employeesByDeptId_.Build(employees_.GetAliveSlots(), [&](Slot s) {return employees_.GetRowBySlot(s).GetDeptId();});

        // Suppliers
        suppliersByName_.Build(suppliers_.GetAliveSlots(), [&](Slot s) {return suppliers_.GetRowBySlot(s).GetName(); });
        suppliersByCity_.Build(suppliers_.GetAliveSlots(), [&](Slot s) {return suppliers_.GetRowBySlot(s).GetCity(); });

        // Products
        productsByName_.Build(products_.GetAliveSlots(), [&](Slot s) {return products_.GetRowBySlot(s).GetName(); });
        productsByDefaultSupplierId_.Build(products_.GetAliveSlots(), [&](Slot s) {return products_.GetRowBySlot(s).GetDefaultSupplierId(); });

        // Purchases
        purchasesByDate_.Build(purchases_.GetAliveSlots(), [&](Slot s) {return purchases_.GetRowBySlot(s).GetDate(); });
        purchasesBySupplierId_.Build(purchases_.GetAliveSlots(), [&](Slot s) {return purchases_.GetRowBySlot(s).GetSupplierId(); });
        purchasesByProductId_.Build(purchases_.GetAliveSlots(), [&](Slot s) {return purchases_.GetRowBySlot(s).GetProductId(); });
        purchasesByDeptId_.Build(purchases_.GetAliveSlots(), [&](Slot s) {return purchases_.GetRowBySlot(s).GetDeptId(); });
    }


    void DeleteDepartment(int deptId) {
        for (size_t i = 0; i < employees_.GetRowCount(); ++i) {
            const auto& e = employees_.GetRow(i);
            if (e.GetDeptId() == deptId) {
                throw DbConstraintError::Restrict(
                    "employees", "dept_id",
                    std::to_string(deptId),
                    "departments", "id",
                    (int)i
                );
            }
        }
        for (size_t i = 0; i < purchases_.GetRowCount(); ++i) {
            const auto& p = purchases_.GetRow(i);
            if (p.GetDeptId() == deptId) {
                throw DbConstraintError::Restrict(
                    "purchases", "dept_id",
                    std::to_string(deptId),
                    "departments", "id",
                    (int)i
                );
            }
        }
        if (!departments_.DeleteById(deptId)) {
            throw std::runtime_error("Department not found: id=" + std::to_string(deptId));
        }
    }

    void DeleteSupplier(int supplierId) {
        for (size_t i = 0; i < products_.GetRowCount(); ++i) {
            const auto& pr = products_.GetRow(i);
            if (pr.GetDefaultSupplierId() == supplierId) {
                throw DbConstraintError::Restrict(
                    "products", "default_supplier_id",
                    std::to_string(supplierId),
                    "suppliers", "id",
                    (int)i
                );
            }
        }
        for (size_t i = 0; i < purchases_.GetRowCount(); ++i) {
            const auto& p = purchases_.GetRow(i);
            if (p.GetSupplierId() == supplierId) {
                throw DbConstraintError::Restrict(
                    "purchases", "supplier_id",
                    std::to_string(supplierId),
                    "suppliers", "id",
                    (int)i
                );
            }
        }
        if (!suppliers_.DeleteById(supplierId)) {
            throw std::runtime_error("Supplier not found: id=" + std::to_string(supplierId));
        }
    }

    void DeleteProduct(int productId) {
        for (size_t i = 0; i < purchases_.GetRowCount(); ++i) {
            const auto& p = purchases_.GetRow(i);
            if (p.GetProductId() == productId) {
                throw DbConstraintError::Restrict(
                    "purchases", "product_id",
                    std::to_string(productId),
                    "products", "id",
                    (int)i
                );
            }
        }
        if (!products_.DeleteById(productId)) {
            throw std::runtime_error("Product not found: id=" + std::to_string(productId));
        }
    }

    void DeleteAddress(int addressId) {
        for (size_t i = 0; i < departments_.GetRowCount(); ++i) {
            const auto& d = departments_.GetRow(i);
            if (d.GetAddressId() == addressId) {
                throw DbConstraintError::Restrict(
                    "departments", "address_id",
                    std::to_string(addressId),
                    "addresses", "id",
                    (int)i
                );
            }
        }
        if (!addresses_.DeleteById(addressId)) {
            throw std::runtime_error("Address not found: id=" + std::to_string(addressId));
        }
    }

    void DeleteEmployee(int employeeId) {
        if (!employees_.DeleteById(employeeId)) {
            throw std::runtime_error("Employee not found: id=" + std::to_string(employeeId));
        }
    }

    void DeletePurchase(int purchaseId) {
        if (!purchases_.DeleteById(purchaseId)) {
            throw std::runtime_error("Purchase not found: id=" + std::to_string(purchaseId));
        }
    }

private:

    template<typename TRow>
    // внутренние индексы строк (slot) в внешние идентификаторы (id)
    static std::vector<int> SlotsToIds(const Table<TRow, int>& t, const std::vector<Slot>& slots) {
        std::vector<int> ids;
        ids.reserve(slots.size()); //заранее выделяем память (чисто оптимизация)
        for (auto s : slots) {
            ids.push_back(t.GetRowBySlot(s).GetId());
        }
        return ids;
    }

    // Addresses
    HashIndex<std::string, Slot> addressesByCity_{512};
    BTreeIndex<int, Slot> addressesById_{16};
    // Departments
    HashIndex<std::string, Slot> departmentsByName_{256};
    HashIndex<int, Slot> departmentsByAddressId_{256};

    // Employees
    HashIndex<std::string, Slot> employeesByFullName_{1024};
    BTreeIndex<int, Slot> employeesByBirthYear_{16};
    HashIndex<int, Slot> employeesByDeptId_{1024};

    // Suppliers
    HashIndex<std::string, Slot> suppliersByName_{1024};
    HashIndex<std::string, Slot> suppliersByCity_{512};

    // Products
    HashIndex<std::string, Slot> productsByName_{1024};
    HashIndex<int, Slot> productsByDefaultSupplierId_{1024};

    // Purchases
    BTreeIndex<std::string, Slot> purchasesByDate_{16}; // YYYY-MM-DD => range works
    HashIndex<int, Slot> purchasesBySupplierId_{2048};
    HashIndex<int, Slot> purchasesByProductId_{2048};
    HashIndex<int, Slot> purchasesByDeptId_{2048};

    Table<Address, int> addresses_;
    Table<Department, int> departments_;
    Table<Employee, int> employees_;
    Table<Supplier, int> suppliers_;
    Table<Product, int> products_;
    Table<Purchase, int> purchases_;


    void ValidateUniqueDepartmentNames() const { //в таблице departments поле name должно быть уникальным
        HashTable<std::string, int> seen(128);
        for (size_t i = 0; i < departments_.GetRowCount(); ++i) {
            const auto& d = departments_.GetRow(i);
            const std::string& name = d.GetName();
            if (seen.ContainsKey(name)) {
                throw DbConstraintError::Unique("departments", "name", name, (int)i);
            }
            seen.Add(name, d.GetId());
        }
    }

    void ValidateUniqueSupplierNames() const {
        HashTable<std::string, int> seen(256);
        for (size_t i = 0; i < suppliers_.GetRowCount(); ++i) {
            const auto& s = suppliers_.GetRow(i);
            const std::string& name = s.GetName();
            if (seen.ContainsKey(name)) {
                throw DbConstraintError::Unique("suppliers", "name", name, (int)i);
            }
            seen.Add(name, s.GetId());
        }
    }

    void ValidateUniqueProductNames() const {
        HashTable<std::string, int> seen(512);
        for (size_t i = 0; i < products_.GetRowCount(); ++i) {
            const auto& p = products_.GetRow(i);
            const std::string& name = p.GetName();
            if (seen.ContainsKey(name)) {
                throw DbConstraintError::Unique("products", "name", name, (int)i);
            }
            seen.Add(name, p.GetId());
        }
    }

    void ValidateDepartmentsAddressFk() const {
        for (size_t i = 0; i < departments_.GetRowCount(); ++i) {
            const auto& d = departments_.GetRow(i);
            int addrId = d.GetAddressId();
            if (!addresses_.ContainsId(addrId)) {
                throw DbConstraintError::ForeignKey(
                    "departments", "address_id",
                    std::to_string(addrId),
                    "addresses", "id",
                    (int)i
                );
            }
        }
    }

    void ValidateEmployeesDeptFk() const {
        for (size_t i = 0; i < employees_.GetRowCount(); ++i) {
            const auto& e = employees_.GetRow(i);
            int deptId = e.GetDeptId();
            if (!departments_.ContainsId(deptId)) {
                throw DbConstraintError::ForeignKey(
                    "employees", "dept_id",
                    std::to_string(deptId),
                    "departments", "id",
                    (int)i
                );
            }
        }
    }

    void ValidateProductsDefaultSupplierFk() const {
        for (size_t i = 0; i < products_.GetRowCount(); ++i) {
            const auto& p = products_.GetRow(i);
            int supId = p.GetDefaultSupplierId();
            if (!suppliers_.ContainsId(supId)) {
                throw DbConstraintError::ForeignKey(
                    "products", "default_supplier_id",
                    std::to_string(supId),
                    "suppliers", "id",
                    (int)i
                );
            }
        }
    }

    void ValidatePurchasesFk() const {
        for (size_t i = 0; i < purchases_.GetRowCount(); ++i) {
            const auto& pur = purchases_.GetRow(i);

            if (!departments_.ContainsId(pur.GetDeptId())) {
                throw DbConstraintError::ForeignKey(
                    "purchases", "dept_id",
                    std::to_string(pur.GetDeptId()),
                    "departments", "id",
                    (int)i
                );
            }
            if (!suppliers_.ContainsId(pur.GetSupplierId())) {
                throw DbConstraintError::ForeignKey(
                    "purchases", "supplier_id",
                    std::to_string(pur.GetSupplierId()),
                    "suppliers", "id",
                    (int)i
                );
            }
            if (!products_.ContainsId(pur.GetProductId())) {
                throw DbConstraintError::ForeignKey(
                    "purchases", "product_id",
                    std::to_string(pur.GetProductId()),
                    "products", "id",
                    (int)i
                );
            }
        }
    }
};

#endif // LAZYDB_DATABASE_H
