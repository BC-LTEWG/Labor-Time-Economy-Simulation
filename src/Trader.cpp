
#include <iostream>
#include <limits>
#include <random>
#include <cmath>
#include "Sim.h"
#include <unordered_set>

#include "Trader.h"
#include "Order.h"
#include "Product.h"
#include "Society.h"
#include "Plan.h"
#include "Constants.h"

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

bool Trader::receive_import_order(Order * order) {
    if (order == nullptr || order->quantity <= 0 ||
        order->customer == nullptr) {
        return false;
    }

    std::cout << "[Trader] Import order received: Product "
          << order->product->id
          << ", Quantity: " << order->quantity << std::endl;

    pending_import_orders.push_back(order);
    return true;
}

bool Trader::import_into_inventory(Product * product, int quantity) {
    if (product == nullptr || quantity <= 0) {
        return false;
    }

    double ce_price = get_ce_import_price(product);
    double total_cost = ce_price * quantity;

    if (ce_price <= 0 || cash_balance < total_cost) {
        return false;
    }

    cash_balance -= total_cost;
    input_inventory[product] += quantity;

    std::cout << "[Trader] CE import purchased: Product "
          << product->id << ", Quantity: " << quantity
          << ", Total cost: " << total_cost << std::endl;

    log_inventory_level(product, input_inventory[product]);

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
    int quantity = static_cast<int>(std::ceil(cash_shortfall / best_ce_price));

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

void Trader::export_product(Product * product, int quantity) {
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
    int quantity = static_cast<int>(
        plan->order->quantity - plan->quantity_remaining
    );

    Firm::receive_shipment(plan);

    if (quantity > 0) {
        export_product(product, quantity);
    }
}


bool Trader::fulfill_import_order(Order * order) {
    if (order == nullptr || !imported_orders.count(order)) {
        return false;
    }

    Product * product = order->product;
    int quantity = order->quantity;

    if (get_inventory_level(product) < quantity) {
        return false;
    }

    Plan shipment{};
    shipment.order = order;
    shipment.firm = this;
    shipment.quantity_remaining = 0;
    shipment.debt = order->product->price_per_unit * order->quantity;

    double payment = product->price_per_unit * quantity;

    if (!remove_input_from_inventory(product, quantity)) {
        return false;
    }

    order->status = Order::kOrderFinished;
    order->customer->receive_shipment(&shipment);

    account += payment;

    std::cout << "[Trader] Import delivered: Product "
          << product->id
          << ", Quantity: " << quantity
          << ", LTE payment: " << payment << std::endl;

    log_inventory_level(product, get_inventory_level(product));
    log_account();

    return true;
}


void Trader::process_pending_import_orders() {
    for (auto it = pending_import_orders.begin();
         it != pending_import_orders.end();) {

        Order * order = *it;

        if (!imported_orders.count(order)) {
            if (import_into_inventory(order->product, order->quantity)) {
                imported_orders.insert(order);
            }
        }

        if (fulfill_import_order(order)) {
            imported_orders.erase(order);
            it = pending_import_orders.erase(it);
        } else {
            ++it;
        }
    }
}

void Trader::on_time_step() {
    Firm::on_time_step();
    process_pending_import_orders();

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
