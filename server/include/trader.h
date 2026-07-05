#pragma once

#include <map>
#include <string>

class Trader {
public:
    explicit Trader(float initial_balance);

    void update_balance(float amount);
    // Adjusts held quantity of symbol by delta; removes the symbol from the
    // holdings map once its quantity drops to zero.
    void update_quantity(const std::string& symbol, int delta);

    float get_balance() const;
    const std::map<std::string, int>& get_holdings() const;
    int get_quantity(const std::string& symbol) const;

    // Human-readable summary of balance + holdings, sent back to clients.
    std::string get_stocks() const;

private:
    float balance_;
    std::map<std::string, int> stocks_;
};
