#ifndef LAZYDB_ADDRESS_H
#define LAZYDB_ADDRESS_H

#include <string>

class Address {
public:
    Address() = default;
    Address(int id, std::string city, std::string street, std::string building, std::string type);

    int GetId() const { return id_; }
    const std::string& GetCity() const { return city_; } //возвращаем ссылку
    const std::string& GetStreet() const { return street_; }
    const std::string& GetBuilding() const { return building_; }
    const std::string& GetType() const { return type_; } // теперь просто строка

    //const до  нельзя поменять сроку, которую выдаст функция
    //const после запрещает менять объект (this) внутри метода и позволяет вызывать этот метод у const-объектов
    static Address FromCSV(const std::string& line);

private:
    int id_ = 0;
    std::string city_;
    std::string street_;
    std::string building_;
    std::string type_ = "Unknown";
};

#endif // LAZYDB_ADDRESS_H
