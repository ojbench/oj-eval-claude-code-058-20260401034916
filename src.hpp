#ifndef POKEDEX_HPP
#define POKEDEX_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <set>
#include <map>

class BasicException {
protected:
    const char *message;

public:
    explicit BasicException(const char *_message) : message(_message) {}

    virtual const char *what() const {
        return message;
    }
};

class ArgumentException : public BasicException {
public:
    explicit ArgumentException(const char *_message) : BasicException(_message) {}
};

class IteratorException : public BasicException {
public:
    explicit IteratorException(const char *_message) : BasicException(_message) {}
};

struct Pokemon {
    char name[12];
    int id;
    std::vector<std::string> types;

    Pokemon() : id(0) {
        memset(name, 0, sizeof(name));
    }

    Pokemon(const char *_name, int _id, const std::vector<std::string> &_types) : id(_id), types(_types) {
        strncpy(name, _name, 11);
        name[11] = '\0';
    }
};

class Pokedex {
private:
    std::string fileName;
    std::map<int, Pokemon> pokemons; // id -> Pokemon

    // Valid type names
    static const std::vector<std::string> validTypes;

    // Type effectiveness chart: [attacker][defender] = multiplier
    static const std::map<std::string, std::map<std::string, float>> typeChart;

    bool isValidName(const char *name) const {
        if (!name || strlen(name) == 0 || strlen(name) > 10) return false;
        for (int i = 0; name[i]; i++) {
            if (!((name[i] >= 'a' && name[i] <= 'z') || (name[i] >= 'A' && name[i] <= 'Z'))) {
                return false;
            }
        }
        return true;
    }

    bool isValidType(const std::string &type) const {
        return std::find(validTypes.begin(), validTypes.end(), type) != validTypes.end();
    }

    std::vector<std::string> parseTypes(const char *types) const {
        std::vector<std::string> result;
        std::string str(types);
        size_t pos = 0;
        while (pos < str.length()) {
            size_t nextPos = str.find('#', pos);
            if (nextPos == std::string::npos) {
                result.push_back(str.substr(pos));
                break;
            }
            result.push_back(str.substr(pos, nextPos - pos));
            pos = nextPos + 1;
        }
        return result;
    }

    void loadFromFile() {
        std::ifstream file(fileName);
        if (!file.is_open()) return;

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::istringstream iss(line);
            Pokemon p;
            std::string nameStr;
            std::string typesStr;

            if (!(iss >> nameStr >> p.id)) continue;
            std::getline(iss, typesStr);

            // Remove leading space
            if (!typesStr.empty() && typesStr[0] == ' ') {
                typesStr = typesStr.substr(1);
            }

            strncpy(p.name, nameStr.c_str(), 11);
            p.name[11] = '\0';
            p.types = parseTypes(typesStr.c_str());

            pokemons[p.id] = p;
        }
        file.close();
    }

    void saveToFile() const {
        std::ofstream file(fileName);
        if (!file.is_open()) return;

        for (const auto &pair : pokemons) {
            const Pokemon &p = pair.second;
            file << p.name << " " << p.id << " ";
            for (size_t i = 0; i < p.types.size(); i++) {
                if (i > 0) file << "#";
                file << p.types[i];
            }
            file << "\n";
        }
        file.close();
    }

public:
    explicit Pokedex(const char *_fileName) : fileName(_fileName) {
        loadFromFile();
    }

    ~Pokedex() {
        saveToFile();
    }

    bool pokeAdd(const char *name, int id, const char *types) {
        // Validate name
        if (!isValidName(name)) {
            std::string msg = "Argument Error: PM Name Invalid (";
            msg += name ? name : "";
            msg += ")";
            throw ArgumentException(msg.c_str());
        }

        // Parse and validate types
        std::vector<std::string> typeList = parseTypes(types);
        if (typeList.empty() || typeList.size() > 7) {
            std::string msg = "Argument Error: PM Type Invalid (";
            msg += types;
            msg += ")";
            throw ArgumentException(msg.c_str());
        }

        for (const auto &type : typeList) {
            if (!isValidType(type)) {
                std::string msg = "Argument Error: PM Type Invalid (";
                msg += type;
                msg += ")";
                throw ArgumentException(msg.c_str());
            }
        }

        // Check if id already exists
        if (pokemons.find(id) != pokemons.end()) {
            return false;
        }

        // Check if name already exists
        for (const auto &pair : pokemons) {
            if (strcmp(pair.second.name, name) == 0) {
                return false;
            }
        }

        Pokemon p(name, id, typeList);
        pokemons[id] = p;
        return true;
    }

    bool pokeDel(int id) {
        auto it = pokemons.find(id);
        if (it == pokemons.end()) {
            return false;
        }
        pokemons.erase(it);
        return true;
    }

    std::string pokeFind(int id) const {
        auto it = pokemons.find(id);
        if (it == pokemons.end()) {
            return "None";
        }
        return std::string(it->second.name);
    }

    std::string typeFind(const char *types) const {
        // Parse and validate types
        std::vector<std::string> typeList = parseTypes(types);

        for (const auto &type : typeList) {
            if (!isValidType(type)) {
                std::string msg = "Argument Error: PM Type Invalid (";
                msg += type;
                msg += ")";
                throw ArgumentException(msg.c_str());
            }
        }

        std::vector<const Pokemon*> matches;
        for (const auto &pair : pokemons) {
            const Pokemon &p = pair.second;
            bool hasAll = true;
            for (const auto &reqType : typeList) {
                if (std::find(p.types.begin(), p.types.end(), reqType) == p.types.end()) {
                    hasAll = false;
                    break;
                }
            }
            if (hasAll) {
                matches.push_back(&p);
            }
        }

        if (matches.empty()) {
            return "None";
        }

        // Sort by id
        std::sort(matches.begin(), matches.end(), [](const Pokemon *a, const Pokemon *b) {
            return a->id < b->id;
        });

        std::ostringstream oss;
        oss << matches.size() << "\n";
        for (const auto *p : matches) {
            oss << p->name << "\n";
        }
        return oss.str();
    }

    float attack(const char *type, int id) const {
        auto it = pokemons.find(id);
        if (it == pokemons.end()) {
            return -1.0f;
        }

        const Pokemon &defender = it->second;
        float multiplier = 1.0f;

        std::string attackType(type);
        for (const auto &defType : defender.types) {
            auto attackerIt = typeChart.find(attackType);
            if (attackerIt != typeChart.end()) {
                auto defenderIt = attackerIt->second.find(defType);
                if (defenderIt != attackerIt->second.end()) {
                    multiplier *= defenderIt->second;
                }
            }
        }

        return multiplier;
    }

    int catchTry() const {
        if (pokemons.empty()) return 0;

        std::set<int> owned;
        std::set<int> uncaught;

        // Start with smallest id
        int minId = pokemons.begin()->first;
        owned.insert(minId);

        // All others are uncaught initially
        for (const auto &pair : pokemons) {
            if (pair.first != minId) {
                uncaught.insert(pair.first);
            }
        }

        bool changed = true;
        while (changed) {
            changed = false;
            std::vector<int> newlyCaught;

            for (int targetId : uncaught) {
                const Pokemon &target = pokemons.at(targetId);

                // Check if any owned pokemon can catch this one
                for (int ownedId : owned) {
                    const Pokemon &ownedPoke = pokemons.at(ownedId);

                    // Check if any of owned pokemon's types can deal 2x damage to target
                    bool canCatch = false;
                    for (const auto &ownedType : ownedPoke.types) {
                        float dmg = 1.0f;
                        for (const auto &targetType : target.types) {
                            auto attackerIt = typeChart.find(ownedType);
                            if (attackerIt != typeChart.end()) {
                                auto defenderIt = attackerIt->second.find(targetType);
                                if (defenderIt != attackerIt->second.end()) {
                                    dmg *= defenderIt->second;
                                }
                            }
                        }
                        if (dmg >= 2.0f) {
                            canCatch = true;
                            break;
                        }
                    }

                    if (canCatch) {
                        newlyCaught.push_back(targetId);
                        break;
                    }
                }
            }

            for (int id : newlyCaught) {
                owned.insert(id);
                uncaught.erase(id);
                changed = true;
            }
        }

        return owned.size();
    }

    struct iterator {
        std::map<int, Pokemon>::iterator it;
        Pokedex *pokedex;

        iterator(std::map<int, Pokemon>::iterator _it, Pokedex *_pokedex)
            : it(_it), pokedex(_pokedex) {}

        iterator &operator++() {
            if (it == pokedex->pokemons.end()) {
                throw IteratorException("Iterator Error: Iterator Out of Bound");
            }
            ++it;
            return *this;
        }

        iterator &operator--() {
            if (it == pokedex->pokemons.begin()) {
                throw IteratorException("Iterator Error: Iterator Out of Bound");
            }
            --it;
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        iterator operator--(int) {
            iterator tmp = *this;
            --(*this);
            return tmp;
        }

        iterator &operator=(const iterator &rhs) {
            it = rhs.it;
            pokedex = rhs.pokedex;
            return *this;
        }

        bool operator==(const iterator &rhs) const {
            return it == rhs.it;
        }

        bool operator!=(const iterator &rhs) const {
            return it != rhs.it;
        }

        Pokemon &operator*() const {
            if (it == pokedex->pokemons.end()) {
                throw IteratorException("Iterator Error: Dereference Invalid Iterator");
            }
            return it->second;
        }

        Pokemon *operator->() const {
            if (it == pokedex->pokemons.end()) {
                throw IteratorException("Iterator Error: Dereference Invalid Iterator");
            }
            return &(it->second);
        }
    };

    iterator begin() {
        return iterator(pokemons.begin(), this);
    }

    iterator end() {
        return iterator(pokemons.end(), this);
    }
};

// Initialize static members
const std::vector<std::string> Pokedex::validTypes = {
    "fire", "water", "grass", "electric", "ground", "flying", "dragon"
};

const std::map<std::string, std::map<std::string, float>> Pokedex::typeChart = {
    {"fire", {{"fire", 0.5f}, {"water", 0.5f}, {"grass", 2.0f}}},
    {"water", {{"fire", 2.0f}, {"water", 0.5f}, {"grass", 0.5f}}},
    {"grass", {{"water", 2.0f}, {"grass", 0.5f}, {"ground", 2.0f}}},
    {"electric", {{"water", 2.0f}, {"electric", 0.5f}, {"ground", 0.0f}, {"flying", 2.0f}}},
    {"ground", {{"fire", 2.0f}, {"electric", 2.0f}, {"grass", 0.5f}, {"flying", 0.0f}}},
    {"flying", {{"grass", 2.0f}, {"electric", 0.5f}, {"ground", 2.0f}}},
    {"dragon", {{"dragon", 2.0f}}}
};

#endif // POKEDEX_HPP
