#pragma once

#include "Firm.h"

struct Product;

// Firm responsible for trading LTE products with the CE.
class TradeFirm : public Firm {
public:
    TradeFirm();

    Logger::Client get_client_type() override;
    void add_to_catalog(Product * product) override;
    void initialize_inventory() override;
    void on_time_step() override;

    // Select and request the most profitable LTE input.
    void request_most_profitable_input();

private:
    Product * selected_product = nullptr;
    bool initial_order_submitted = false;

    // Calculate the profit from exchanging one unit.
    double calculate_trade_profitability(Product * product);

    // Obtain CE value and the MELT conversion factor.
    double get_ce_value(Product * product);
    double get_melt();
};