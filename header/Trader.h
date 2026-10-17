#pragma once

#include "Firm.h"

struct Product;

class Trader : public Firm {
public:
    Trader();

    Logger::Client get_client_type() override;
    void add_to_catalog(Product * product) override;
    void initialize_inventory() override;
    void on_time_step() override;

private:
    double calculate_trade_profitability(Product * product);

    double get_ce_value(Product * product);
    double get_melt();

    void select_and_order_most_profitable_export();
};