
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
#include "Person.h"
#include <algorithm>

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

bool Trader::should_import(Order * order, double domestic_cost) {
    double ce_price = get_ce_import_price(order->product);

    double labor_cost_per_unit =
        order->product->price_per_unit * TRADE_LABOR_COST_MULT;

    double import_price_per_unit =
        ce_price / MELT + labor_cost_per_unit;

    double import_cost =
        import_price_per_unit * order->quantity;

    if (import_cost >= domestic_cost) {
        return false;
    }

    import_ce_prices[order] = ce_price;
    import_lte_prices[order] = import_price_per_unit;

    return true;
}

bool Trader::recruit_trade_worker() {
    auto& unemployed =
        Society::get_instance()->get_unemployed_people();

    if (!unemployed.empty()) {
        hire_worker(*unemployed.begin());
        return true;
    }

    for (Firm * firm : Society::get_instance()->get_firms()) {
        if (firm == this) continue;

        auto transfers = firm->propose_transfer(1);
        if (!transfers.empty()) {
            hire_worker(transfers.front());
            return true;
        }
    }

    return false;
}

void Trader::process_trade_jobs() {
    if (!Society::get_instance()->is_within_work_schedule()) {
        return;
    }

    if (pending_trade_jobs.empty()) {
        return;
    }

    if (standby_workers.empty()) {
        recruit_trade_worker();
    }

    if (standby_workers.empty()) {
        return;
    }

    // Track each worker's available labor time.
    std::unordered_map<Person *, double> available_hours;

    for (Person * worker : standby_workers) {
        available_hours[worker] = 1.0;
    }

    for (auto it = pending_trade_jobs.begin();
         it != pending_trade_jobs.end();) {

        TradeJob& job = *it;

        for (Person * worker : standby_workers) {
            if (job.labor_hours_remaining <= 1e-9) {
                break;
            }

            double& remaining = available_hours[worker];

            if (remaining <= 1e-9) {
                continue;
            }

            double hours = std::min(
                remaining,
                job.labor_hours_remaining
            );

            worker->register_hours_worked(hours);
            account -= hours;
            labor_hours_this_time_step += hours;

            remaining -= hours;
            job.labor_hours_remaining -= hours;
        }

        if (job.labor_hours_remaining <= 1e-9) {
            if (job.is_export) {
                export_product(
                    job.product,
                    job.quantity,
                    job.ce_export_unit_price
                );
            } else {
                imported_orders.insert(job.import_order);
            }

            it = pending_trade_jobs.erase(it);
        } else {
            ++it;
        }
    }
}

double Trader::get_import_lte_price(Order * order) {
    auto it = import_lte_prices.find(order);

    if (it == import_lte_prices.end()) {
        return 0.0;
    }

    return it->second;
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

bool Trader::import_into_inventory(Order * order) {
    if (!order || !import_ce_prices.count(order)) {
        return false;
    }

    Product * product = order->product;
    int quantity = order->quantity;

    double ce_price = import_ce_prices.at(order);
    double total_cost = ce_price * quantity;

    if (quantity <= 0 || ce_price <= 0.0 ||
        cash_balance < total_cost) {
        return false;
    }

    cash_balance -= total_cost;
    input_inventory[product] += quantity;

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

    for (const TradeJob& job : pending_trade_jobs) {
        if (job.is_export) {
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
        double trade_labor_cost = product->price_per_unit * TRADE_LABOR_COST_MULT;

        double profit =
            ce_price / get_melt()
            - product->price_per_unit
            - trade_labor_cost;

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
    int quantity = static_cast<int>(
        std::ceil(
            (cash_shortfall / best_ce_price) * EXPORT_ORDER_MULTIPLIER
        )
    );

    // # Testing
    std::cout << "[Trader] Selected product " << best_product->id
              << " with profitability " << best_profit << std::endl;

    std::cout << "[Trader] Cash shortfall: "
              << cash_shortfall << std::endl;

    std::cout << "[Trader] Expected CE price per unit: "
              << best_ce_price << std::endl;

    std::cout << "[Trader] Ordering quantity: "
              << quantity << std::endl;

    if (quantity <= 0) {
        return;
    }

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
        export_ce_prices[order] = best_ce_price;

        // # Testing
        std::cout << "[Trader] Order successfully placed." << std::endl;
    }
}

void Trader::export_product(
    Product * product,
    int quantity,
    double ce_price
) {
    double labor_hours =
        product->price_per_unit * quantity * TRADE_LABOR_COST_MULT;

    double cash_received = ce_price * quantity;
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
    Order * order = plan->order;
    Product * product = order->product;

    int quantity = static_cast<int>(
        order->quantity - plan->quantity_remaining
    );

    product_to_outbound_orders[product].erase(order);

    double transaction_amount =
        product->price_per_unit * quantity;

    account -= transaction_amount;
    plan->firm->process_payment(plan, transaction_amount);

    if (quantity > 0) {
        double labor_hours =
            product->price_per_unit *
            quantity *
            TRADE_LABOR_COST_MULT;

        double ce_price = export_ce_prices.at(order);

        pending_trade_jobs.push_back({
            product,
            quantity,
            nullptr,
            labor_hours,
            true,
            ce_price
        });

        export_ce_prices.erase(order);
    }

    log_account();
}

bool Trader::fulfill_import_order(Order * order) {
    if (!order || !imported_orders.count(order)) {
        return false;
    }

    Product * product = order->product;
    int quantity = order->quantity;

    if (get_inventory_level(product) < quantity) {
        return false;
    }

    double payment =
        get_import_lte_price(order) * quantity;

    Plan shipment{};
    shipment.order = order;
    shipment.firm = this;
    shipment.quantity_remaining = 0;
    shipment.debt = payment;

    if (!remove_input_from_inventory(product, quantity)) {
        return false;
    }

    order->status = Order::kOrderFinished;
    order->customer->receive_shipment(&shipment);

    account += payment;

    log_inventory_level(product, get_inventory_level(product));
    log_account();

    return true;
}

void Trader::process_pending_import_orders() {
    for (auto it = pending_import_orders.begin();
         it != pending_import_orders.end();) {

        Order * order = *it;

        if (!purchased_import_orders.count(order)) {
            if (import_into_inventory(order)) {
                purchased_import_orders.insert(order);

                double labor_hours =
                    order->product->price_per_unit *
                    order->quantity *
                    TRADE_LABOR_COST_MULT;

                pending_trade_jobs.push_back({
                    order->product,
                    order->quantity,
                    order,
                    labor_hours,
                    false
                });
            }
        }

        if (imported_orders.count(order) &&
            fulfill_import_order(order)) {

            imported_orders.erase(order);
            purchased_import_orders.erase(order);
            import_ce_prices.erase(order);
            import_lte_prices.erase(order);

            it = pending_import_orders.erase(it);
        } else {
            ++it;
        }
    }
}

void Trader::on_time_step() {
    Firm::on_time_step();

    process_pending_import_orders();
    process_trade_jobs();
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
