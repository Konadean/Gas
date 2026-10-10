#include <iostream>
#include <string>
#include <map>
#include <conio.h>
#include "Scanner.h"

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
    // sku is a string so full UPCs (12+ digits, leading zeros) fit and match scans directly
    void initItem(const std::string& sku, const std::string& name, double price, int stock) {
        inventory[sku] = {name, price, stock};
    }

    void initFuel(int cnxsFuelID, const std::string& grade, double price, double tankVol) {
        fuelReserves[cnxsFuelID] = {grade, price, tankVol};
    }

    bool sellItem(const std::string& sku, int qty) {
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
    std::map<std::string, Item> inventory;
    std::map<int, Fuel> fuelReserves;
    double revenue = 0.0;
};

int main(int argc, char* argv[]) {
    Store store;

    store.initItem("1001", "Coffee", 1.99, 50);
    store.initItem("1002", "Chips", 1.49, 30);

    store.initFuel(1, "REGULAR", 1.013, 1000.0);
    store.initFuel(2, "PLUS", 1.025, 1000.0);
    store.initFuel(19, "DIESEL", 1.100, 2000.0);

    // store.sellItem("1001", 2);
    // store.sellFuel(19,30);
    // store.sellItem("1002",4);

    // Scanner reads on its own thread; the sale loop just checks for completed scans
    Scanner scanner((argc > 1) ? argv[1] : "COM3");
    if (!scanner.start()) return 1;

    std::cout << "Ready - scan an item (press q to quit)\n";

    // Main sale loop
    while (true) {
        if (auto code = scanner.tryGetScan()) {
            if (store.sellItem(*code, 1))
                std::cout << "Sold: " << *code << "\n";
            else
                std::cout << "Unknown or out of stock: " << *code << "\n";
        }

        // _kbhit() is non-blocking, so the loop never stalls waiting on the keyboard
        if (_kbhit() && _getch() == 'q') break;

        Sleep(10);  // don't spin at 100% CPU
    }

    std::cout << "Revenue: $" << store.totalRevenue() << "\n";
}   // scanner's destructor stops the reader thread and closes the port
