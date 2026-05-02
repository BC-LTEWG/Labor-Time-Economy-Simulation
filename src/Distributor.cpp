#include <algorithm>
#include <climits>
#include <iostream>
#include <string>

#include "ConsumerGood.h"
#include "Distributor.h"
#include "Logger.h"
#include "Machine.h"
#include "Person.h"
#include "Producer.h"
#include "Product.h"
#include "Sim.h"
#include "Society.h"

Distributor::Distributor(
        Society * society,
        const std::unordered_set<Product *>& initial_catalog
        ) :
    Firm(society, initial_catalog)
{
    for (Product * product : get_products_to_reorder()) {
        society->add_consumer_good(product);
        int quantity =
            product->mean_consumption_frequency *
            (FIRM_STOCKPILE_DURATION + FIRM_DEMAND_WINDOW_MIN) * 
            Sim::get_num_people() / Sim::get_num_distributors();
        Order * order = new Order(product, quantity, this, 0);
        Plan * plan = new Plan;
        plan->order = order;
        plan->firm = this;
        plan->labor_hours = DISTRIBUTION_LABOR_PER_UNIT * quantity;
        plan->raw_materials_remaining = plan->raw_materials = pooled_input_value_account += product->price_per_unit * quantity;
        plan->total_hours_remaining = plan->total_hours =
            plan->labor_hours + plan->raw_materials;
        plan->prd = -(plan->total_hours);
        plan->outgoing_units_consumed = 0;
        plan->machinery_cost = 0.0;
        plans_in_progress.push_back(plan);
        product_to_plan[product] = plan;
        input_inventory[product] = quantity;
        log_inventory_level(product, input_inventory[product]);
        log_catalog();
    }
}

Logger::Client Distributor::get_client_type() {
    return Logger::DISTRIBUTOR;
}

void Distributor::on_time_step() {
    Firm::on_time_step();
    for (Plan * plan : plans_in_progress) {
        plan->labor_hours_remaining -= plan->workers.size();
        for (Person * worker : plan->workers) {
            worker->register_hours_worked(1);
        }
    }
}

int Distributor::try_sell_goods(Product& product, int quantity, Person * person) {
    ConsumerGood * consumer_good = society->get_consumer_good(&product);
    if (!consumer_good) {
        std::cerr << "No consumer good for product " << product.product_name << std::endl;
        return 0;
    }
    add_demand_signal(&product, quantity);
    check_and_reorder_input(&product);
    if (!catalog.count(&product)) return 0;
    int available = std::min(get_inventory(&product), quantity);
    if (available < quantity) {
        log_shortfall(product.product_name, quantity - available);
    }
    double cost = available * consumer_good->price_per_unit;
    if (!person->charge(cost)) {
        std::cerr << "Person cannot afford " << quantity
            << " units of " << product.product_name << " costing " 
            << cost << std::endl;
        return 0;
    } 
    Plan * plan = product_to_plan[&product];
    if (plan) {
        plan->outgoing_units_consumed += quantity;
        plan->prd += cost;
    }
    remove_input_from_inventory(&product, quantity);
    return available;
}

std::unordered_set<Product *> Distributor::get_products_to_reorder() {
    return catalog;
}

void Distributor::log_shortfall(std::string product_name, int shortfall) {
    Logger::get_instance()->log(
            Logger::DISTRIBUTOR,
            "shortfall",
            id,
            product_name,
            shortfall
            );
}

