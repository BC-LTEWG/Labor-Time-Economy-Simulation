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

    double get_ce_import_price(Product * product);
    bool receive_import_order(Order * order);

private:
    double cash_balance = 0.0;
    double get_ce_value(Product * product);
    double get_melt();

    void receive_shipment(Plan * plan) override;
    void select_and_order_most_profitable_export();
    void export_product(Product * product, int quantity);
    void process_pending_import_orders();

    std::vector<Order *> pending_import_orders;
    std::unordered_set<Order *> imported_orders;

    bool import_into_inventory(Product * product, int quantity);
    bool fulfill_import_order(Order * order);
};