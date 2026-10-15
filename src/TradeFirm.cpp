#include <limits>

#include "TradeFirm.h"
#include "Order.h"
#include "Product.h"
#include "Society.h"

TradeFirm::TradeFirm() : Firm() {}

Logger::Client TradeFirm::get_client_type() {
    return Logger::FIRM;
}

void TradeFirm::add_to_catalog(Product * product) {
    catalog.insert(product);
}

void TradeFirm::initialize_inventory() {
    // Start with no inventory.
}

double TradeFirm::calculate_trade_profitability(Product * product) {
    double ce_value = get_ce_value(product);
    double melt = get_melt();

    // Profit = CE value converted to LTE units - LTE purchase cost.
    double ce_value_in_lte = ce_value * melt;
    double lte_cost = product->price_per_unit;

    return ce_value_in_lte - lte_cost;
}

void TradeFirm::request_most_profitable_input() {
    if (initial_order_submitted) {
        return;
    }

    Product * best_product = nullptr;
    double best_profit = 0.0;

    // Compare profitability across available LTE inputs.
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

    // Do not trade if no profitable input exists.
    if (!best_product) {
        return;
    }

    selected_product = best_product;

    // Request one unit of the selected input.
    Order * order = new Order(
        selected_product,
        1,
        this,
        FIRM_STOCKPILE_DURATION * FIRM_REORDER_MAX_PROP
    );

    // Use the existing LTE supplier-selection mechanism.
    if (send_order(order)) {
        initial_order_submitted = true;
    } else {
        delete order;
    }
}

void TradeFirm::on_time_step() {
    // Continue normal firm operations.
    Firm::on_time_step();
}