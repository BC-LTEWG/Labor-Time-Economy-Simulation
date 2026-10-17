
#include <limits>
#include <random>
#include "Sim.h"

#include "Trader.h"
#include "Order.h"
#include "Product.h"
#include "Society.h"

Trader::Trader() : Firm() {}

Logger::Client Trader::get_client_type() {
    return Logger::TRADER;
}

void Trader::add_to_catalog(Product * product) {
    catalog.insert(product);
}

void Trader::initialize_inventory() {
    select_and_order_most_profitable_export();
}

double Trader::calculate_trade_profitability(Product * product) {
    double ce_value = get_ce_value(product);
    double melt = get_melt();

    double ce_value_in_lte = ce_value / melt;
    double lte_cost = product->price_per_unit;

    return ce_value_in_lte - lte_cost;
}

double Trader::get_ce_value(Product * product) {
    std::uniform_real_distribution<double> price_variation(
        -CE_PRICE_VARIATION,
        CE_PRICE_VARIATION
    );

    double variation = price_variation(Sim::get_random_generator());

    return product->price_per_unit * get_melt() * (1.0 + variation);
}

double Trader::get_melt() {
    return MELT;
}

void Trader::select_and_order_most_profitable_export() {
    for (const auto& entry : product_to_outbound_orders) {
        if (!entry.second.empty()) {
            return;
        }
    }
    Product * best_product = nullptr;
    double best_profit = 0.0;
    for (Product * product : Society::get_instance()->get_products()) {
        if (Society::get_instance()->get_suppliers(product).empty()) {
            continue;
        }
        double profit = calculate_trade_profitability(product);
        if (profit > best_profit) {
            best_profit = profit;
            best_product = product;
        }
    }
    if (!best_product) {
        return;
    }
    Order * order = new Order(
        best_product,
        1,
        this,
        FIRM_STOCKPILE_DURATION * FIRM_REORDER_MAX_PROP
    );
    if (!send_order(order)) {
        delete order;
    }
}

void Trader::on_time_step() {
    Firm::on_time_step();
}
