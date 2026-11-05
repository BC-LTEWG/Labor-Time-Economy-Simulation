
#include <iostream>
#include <limits>
#include <random>
#include <cmath>
#include "Sim.h"

#include "Trader.h"
#include "Order.h"
#include "Product.h"
#include "Society.h"
#include "Plan.h"

Trader::Trader() : Firm() {}

Logger::Client Trader::get_client_type() {
    return Logger::TRADER;
}

void Trader::add_to_catalog(Product * product) {
    catalog.insert(product);
}

void Trader::initialize_inventory() {
    // # Testing
    std::cout << "[Trader] Initializing inventory..." << std::endl;
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

double Trader::get_ce_import_price(Product * product) {
    return get_ce_value(product);
}

bool Trader::import_product_from_ce(
    Product * product,
    double quantity,
    Firm * importing_firm,
    double ce_price_per_unit
) {
    if (quantity <= 0 || ce_price_per_unit <= 0 || importing_firm == nullptr) {
        return false;
    }

    double total_ce_import_cost = ce_price_per_unit * quantity;

    if (cash_balance < total_ce_import_cost) {
        return false;
    }

    double total_import_cost_in_labor_hours = total_ce_import_cost / get_melt();

    cash_balance -= total_ce_import_cost;

    importing_firm->receive_ce_import(
        product,
        quantity,
        total_import_cost_in_labor_hours
    );

    account += total_import_cost_in_labor_hours;

    std::cout << "[Trader] Imported product " << product->id
              << ", Quantity: " << quantity
              << ", CE price per unit: " << ce_price_per_unit
              << ", Total CE cost: " << total_ce_import_cost
              << ", LTE payment received: " << total_import_cost_in_labor_hours
              << ", Cash balance: " << cash_balance
              << std::endl;

    return true;
}

void Trader::select_and_order_most_profitable_export() {
    // # Testing
    std::cout << "[Trader] Searching for profitable exports..." << std::endl;

    // Do not place a new order if an outstanding order exists.
    for (const auto& entry : product_to_outbound_orders) {
        if (!entry.second.empty()) {
            // # Testing
            std::cout << "[Trader] Outstanding order exists." << std::endl;
            return;
        }
    }

    Product * best_product = nullptr;
    double best_profit = 0.0;
    double best_ce_price = 0.0;

    for (Product * product : Society::get_instance()->get_products()) {
        if (Society::get_instance()->get_suppliers(product).empty()) {
            continue;
        }

        double ce_price = get_ce_value(product);
        double profit = ce_price / get_melt() - product->price_per_unit;

        // # Testing
        std::cout << "[Trader] Product " << product->id
                  << " profitability: " << profit << std::endl;

        if (profit > best_profit) {
            best_profit = profit;
            best_product = product;
            best_ce_price = ce_price;
        }
    }

    if (!best_product) {
        // # Testing
        std::cout << "[Trader] No profitable export found." << std::endl;
        return;
    }

    double cash_shortfall = CASH_THRESHOLD - cash_balance;
    double quantity = std::ceil(cash_shortfall / best_ce_price);

    // # Testing
    std::cout << "[Trader] Selected product " << best_product->id
              << " with profitability " << best_profit << std::endl;

    std::cout << "[Trader] Cash shortfall: "
              << cash_shortfall << std::endl;

    std::cout << "[Trader] Expected CE price per unit: "
              << best_ce_price << std::endl;

    std::cout << "[Trader] Ordering quantity: "
              << quantity << std::endl;

    Order * order = new Order(
        best_product,
        quantity,
        this,
        FIRM_STOCKPILE_DURATION * FIRM_REORDER_MAX_PROP
    );

    if (!send_order(order)) {
        // # Testing
        std::cout << "[Trader] Order failed." << std::endl;
        delete order;
    } else {
        // # Testing
        std::cout << "[Trader] Order successfully placed." << std::endl;
    }
}

void Trader::export_product(Product * product, double quantity) {
    double labor_hours = product->price_per_unit * quantity;
    double ce_price = get_ce_value(product);
    double cash_received = ce_price * quantity;

    if (!remove_input_from_inventory(product, quantity)) {
        std::cerr << "[Trader] Export failed: insufficient inventory."
                  << std::endl;
        return;
    }

    cash_balance += cash_received;

    // # Testing
    std::cout << "[Trader] Exported product " << product->id
              << ", Quantity: " << quantity
              << ", Labor hours: " << labor_hours
              << ", CE price per unit: " << ce_price
              << ", Cash received: " << cash_received
              << ", Cash balance: " << cash_balance
              << std::endl;
}

void Trader::receive_shipment(Plan * plan) {
    Product * product = plan->order->product;
    double quantity = plan->order->quantity - plan->quantity_remaining;

    Firm::receive_shipment(plan);

    if (quantity > 0) {
        export_product(product, quantity);
    }
}

void Trader::on_time_step() {
    Firm::on_time_step();

    // # Testing
    std::cout << "\n[Trader] Time step: "
              << Sim::get_current_time_step() << std::endl;

    std::cout << "  LTE Account: " << account << std::endl;
    std::cout << "  Cash Balance: " << cash_balance << std::endl;

    std::cout << "  Inventory:" << std::endl;

    if (input_inventory.empty()) {
        std::cout << "    Empty" << std::endl;
    } else {
        for (const auto & entry : input_inventory) {
            std::cout << "    Product " << entry.first->id
                      << ": " << entry.second
                      << " units" << std::endl;
        }
    }

    if (cash_balance < CASH_THRESHOLD) {
        // # Testing
        std::cout << "[Trader] Cash below threshold. Checking exports."
                << std::endl;

        select_and_order_most_profitable_export();
    }
}
