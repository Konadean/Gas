#include <iostream>
#include <string>
#include <map>

struct Item {
    std::string name;
    double price;
    int stock;
};

struct Fuel {
    std::string grade;
    double price;
    double tankVol;
};

class Store {
public:
    void initItem(int sku, const std::string& name, double price, int stock) {
        inventory[sku] = {name, price, stock};
    }

    void initFuel(int cnxsFuelID, const std::string& grade, double price, double tankVol) {
        fuelReserves[cnxsFuelID] = {grade, price, tankVol};
    }

    bool sellItem(int sku, int qty) {
        auto it = inventory.find(sku);
        // it is the map<key,val> pairing; it->first, it->second is like doing map[0] and map[1] in Python respectively
        if (it == inventory.end() || it->second.stock < qty) return false;
        it->second.stock -= qty;
        revenue += it->second.price * qty;
        return true;
    }

    bool sellFuel(int cnxsFuelID, int qty) {
        auto it = fuelReserves.find(cnxsFuelID);
        if (it == fuelReserves.end() || it->second.tankVol < qty) return false;
        it->second.tankVol -= qty;
        revenue += it->second.price * qty;
        return true;
    }

    double totalRevenue() const { return revenue; }

private:
    std::map<int, Item> inventory;
    std::map<int, Fuel> fuelReserves;
    double revenue = 0.0;
};

int main() {
    Store store;

    store.initItem(1001, "Coffee", 1.99, 50);
    store.initItem(1002, "Chips", 1.49, 30);

    store.initFuel(1, "REGULAR", 1.013, 1000.0);
    store.initFuel(2, "PLUS", 1.025, 1000.0);
    store.initFuel(19, "DIESEL", 1.100, 2000.0);

    store.sellItem(1001, 2);
    store.sellFuel(19,30);
    store.sellItem(1002,4);

    std::cout << "Revenue: $" << store.totalRevenue() << "\n";
}