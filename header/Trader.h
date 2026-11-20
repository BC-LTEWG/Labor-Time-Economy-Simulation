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

    bool should_import(Order * order, double domestic_cost);
    double get_import_lte_price(Order * order);

private:
    double cash_balance = 0.0;
    struct TradeJob {
        Product * product;
        int quantity;
        Order * import_order;
        double labor_hours_remaining;
        bool is_export;
        double ce_export_unit_price = 0.0;
    };

    std::vector<TradeJob> pending_trade_jobs;
    std::unordered_map<Order *, double> import_ce_prices;
    std::unordered_map<Order *, double> import_lte_prices;
    std::unordered_set<Order *> purchased_import_orders;
    std::unordered_map<Order *, double> export_ce_prices;

    void process_trade_jobs();
    bool recruit_trade_worker();
    double get_ce_value(Product * product);
    double get_melt();

    void receive_shipment(Plan * plan) override;
    void select_and_order_most_profitable_export();
    void export_product(Product * product, int quantity, double ce_price);
    void process_pending_import_orders();

    std::vector<Order *> pending_import_orders;
    std::unordered_set<Order *> imported_orders;

    bool import_into_inventory(Order * order);
    bool fulfill_import_order(Order * order);
};