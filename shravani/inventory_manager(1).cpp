// ============================================================
//  Hybrid Inventory Manager — single file version
//  Compile:  g++ -std=c++17 -Wall -o inventory_manager inventory_manager.cpp
//  Run:      ./inventory_manager
// ============================================================

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <string>
#include <cstring>
#include <limits>

// ============================================================
//  SECTION 1 — Data structure (matches the C struct spec)
// ============================================================

struct Item {
    int   id;
    char  name[40];
    int   quantity;
    float price;
    int   is_deleted;   // 0 = active, 1 = soft-deleted
};

// ============================================================
//  SECTION 2 — C-style backend (binary file I/O)
//  All functions return 1 for success, 0 for failure.
//  list_items returns the number of items copied.
// ============================================================

#define DATA_FILE "inventory.dat"

static FILE* open_file(const char* mode) {
    return fopen(DATA_FILE, mode);
}

static int get_item(int id, Item* out) {
    FILE* f = open_file("rb");
    if (!f) return 0;
    Item temp;
    while (fread(&temp, sizeof(Item), 1, f) == 1) {
        if (temp.id == id && temp.is_deleted == 0) {
            *out = temp;
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

static int add_item(const Item* item) {
    Item existing;
    if (get_item(item->id, &existing) == 1) return 0;  // duplicate ID
    FILE* f = open_file("ab");
    if (!f) return 0;
    size_t written = fwrite(item, sizeof(Item), 1, f);
    fclose(f);
    return (written == 1) ? 1 : 0;
}

static int update_item(int id, const Item* updated) {
    FILE* f = open_file("r+b");
    if (!f) return 0;
    Item temp;
    int  index = 0;
    while (fread(&temp, sizeof(Item), 1, f) == 1) {
        if (temp.id == id && temp.is_deleted == 0) {
            fseek(f, (long)index * (long)sizeof(Item), SEEK_SET);
            fwrite(updated, sizeof(Item), 1, f);
            fclose(f);
            return 1;
        }
        index++;
    }
    fclose(f);
    return 0;
}

static int delete_item(int id) {
    FILE* f = open_file("r+b");
    if (!f) return 0;
    Item temp;
    int  index = 0;
    while (fread(&temp, sizeof(Item), 1, f) == 1) {
        if (temp.id == id && temp.is_deleted == 0) {
            temp.is_deleted = 1;
            fseek(f, (long)index * (long)sizeof(Item), SEEK_SET);
            fwrite(&temp, sizeof(Item), 1, f);
            fclose(f);
            return 1;
        }
        index++;
    }
    fclose(f);
    return 0;
}

static int list_items(Item* buffer, int max_items) {
    FILE* f = open_file("rb");
    if (!f) return 0;
    Item temp;
    int  count = 0;
    while (fread(&temp, sizeof(Item), 1, f) == 1 && count < max_items) {
        if (temp.is_deleted == 0) buffer[count++] = temp;
    }
    fclose(f);
    return count;
}

// ============================================================
//  SECTION 3 — C++ InventoryManager class
// ============================================================

class InventoryManager {
public:
    bool addItem   (int id, const std::string& name, int qty, float price);
    bool viewItem  (int id);
    bool updateItem(int id, const std::string& name, int qty, float price);
    bool deleteItem(int id);
    void listItems (const std::string& sortBy = "id");

private:
    void fillItem (Item& item, int id, const std::string& name,
                   int qty, float price, int is_deleted = 0);
    void printItem(const Item& item) const;
};

void InventoryManager::fillItem(Item& item, int id, const std::string& name,
                                 int qty, float price, int is_deleted) {
    item.id         = id;
    item.quantity   = qty;
    item.price      = price;
    item.is_deleted = is_deleted;
    std::strncpy(item.name, name.c_str(), 39);
    item.name[39]   = '\0';
}

void InventoryManager::printItem(const Item& item) const {
    std::cout << std::left
              << "  ID: "     << std::setw(6)  << item.id
              << "  Name: "   << std::setw(22) << item.name
              << "  Qty: "    << std::setw(6)  << item.quantity
              << "  Price: $" << std::fixed << std::setprecision(2) << item.price
              << "\n";
}

bool InventoryManager::addItem(int id, const std::string& name, int qty, float price) {
    Item item;
    fillItem(item, id, name, qty, price);
    if (add_item(&item)) {
        std::cout << "[OK] Item added.\n";
        return true;
    }
    std::cout << "[ERROR] Could not add item — ID " << id << " may already exist.\n";
    return false;
}

bool InventoryManager::viewItem(int id) {
    Item item;
    if (get_item(id, &item)) {
        std::cout << "\n--- Item found ---\n";
        printItem(item);
        return true;
    }
    std::cout << "[ERROR] Item with ID " << id << " not found.\n";
    return false;
}

bool InventoryManager::updateItem(int id, const std::string& name, int qty, float price) {
    Item updated;
    fillItem(updated, id, name, qty, price);
    if (update_item(id, &updated)) {
        std::cout << "[OK] Item updated.\n";
        return true;
    }
    std::cout << "[ERROR] Could not update — ID " << id << " not found.\n";
    return false;
}

bool InventoryManager::deleteItem(int id) {
    if (delete_item(id)) {
        std::cout << "[OK] Item " << id << " deleted (soft delete).\n";
        return true;
    }
    std::cout << "[ERROR] Could not delete — ID " << id << " not found.\n";
    return false;
}

void InventoryManager::listItems(const std::string& sortBy) {
    const int MAX = 1000;
    Item buffer[MAX];
    int count = list_items(buffer, MAX);

    if (count == 0) {
        std::cout << "  (no active items)\n";
        return;
    }

    std::vector<Item> items(buffer, buffer + count);

    if (sortBy == "name") {
        std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
            return std::string(a.name) < std::string(b.name);
        });
    } else {
        std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
            return a.id < b.id;
        });
    }

    std::cout << "\n--- Inventory (" << count << " item"
              << (count != 1 ? "s" : "") << ", sorted by " << sortBy << ") ---\n";
    for (const auto& item : items) printItem(item);
}

// ============================================================
//  SECTION 4 — Input helpers (validation + re-ask on error)
// ============================================================

static int readPositiveInt(const std::string& prompt) {
    int val;
    while (true) {
        std::cout << prompt;
        if (std::cin >> val && val > 0) return val;
        std::cout << "  [!] Must be a positive integer. Try again.\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

static int readNonNegativeInt(const std::string& prompt) {
    int val;
    while (true) {
        std::cout << prompt;
        if (std::cin >> val && val >= 0) return val;
        std::cout << "  [!] Must be 0 or more. Try again.\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

static float readNonNegativeFloat(const std::string& prompt) {
    float val;
    while (true) {
        std::cout << prompt;
        if (std::cin >> val && val >= 0.0f) return val;
        std::cout << "  [!] Must be 0 or more. Try again.\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

static std::string readName(const std::string& prompt) {
    std::string val;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    while (true) {
        std::cout << prompt;
        std::getline(std::cin, val);
        if (!val.empty() && val.length() <= 39) return val;
        if (val.empty())
            std::cout << "  [!] Name cannot be empty. Try again.\n";
        else
            std::cout << "  [!] Name must be 39 characters or fewer. Try again.\n";
    }
}

// ============================================================
//  SECTION 5 — main: menu loop
// ============================================================

static void printMenu() {
    std::cout << "\n╔════════════════════════════╗\n"
              << "║   Hybrid Inventory Manager  ║\n"
              << "╠════════════════════════════╣\n"
              << "║  1. Add Item                ║\n"
              << "║  2. View Item               ║\n"
              << "║  3. Update Item             ║\n"
              << "║  4. Delete Item             ║\n"
              << "║  5. List All                ║\n"
              << "║  6. Exit                    ║\n"
              << "╚════════════════════════════╝\n"
              << "Choice: ";
}

int main() {
    InventoryManager mgr;
    int choice;

    while (true) {
        printMenu();

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "  [!] Invalid input. Enter a number 1-6.\n";
            continue;
        }

        switch (choice) {

        case 1: {
            std::cout << "\n-- Add Item --\n";
            int         id   = readPositiveInt("  ID       : ");
            std::string name = readName       ("  Name     : ");
            int         qty  = readNonNegativeInt  ("  Quantity : ");
            float       prc  = readNonNegativeFloat("  Price    : ");
            mgr.addItem(id, name, qty, prc);
            break;
        }

        case 2: {
            std::cout << "\n-- View Item --\n";
            int id = readPositiveInt("  ID: ");
            mgr.viewItem(id);
            break;
        }

        case 3: {
            std::cout << "\n-- Update Item --\n";
            int         id   = readPositiveInt("  ID        : ");
            std::string name = readName       ("  New Name  : ");
            int         qty  = readNonNegativeInt  ("  New Qty   : ");
            float       prc  = readNonNegativeFloat("  New Price : ");
            mgr.updateItem(id, name, qty, prc);
            break;
        }

        case 4: {
            std::cout << "\n-- Delete Item --\n";
            int id = readPositiveInt("  ID: ");
            mgr.deleteItem(id);
            break;
        }

        case 5: {
            std::cout << "\n-- List All --\n";
            std::cout << "  Sort by: (1) ID  (2) Name  > ";
            int s = 1;
            std::cin >> s;
            mgr.listItems(s == 2 ? "name" : "id");
            break;
        }

        case 6:
            std::cout << "Goodbye!\n";
            return 0;

        default:
            std::cout << "  [!] Invalid choice. Enter 1-6.\n";
        }
    }
}
