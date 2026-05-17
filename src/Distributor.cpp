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
        input_inventory[product] =
            product->mean_consumption_frequency *
            (FIRM_STOCKPILE_DURATION + FIRM_DEMAND_WINDOW_MIN) * 
            Sim::get_num_people() / Sim::get_num_distributors();
        log_inventory_level(product, input_inventory[product]);
    }
}

Logger::Client Distributor::get_client_type() {
    return Logger::DISTRIBUTOR;
}

void Distributor::on_time_step() {
    Firm::on_time_step();
}

int Distributor::try_sell_goods(Product * product, int quantity, Person * person) {
    ConsumerGood * consumer_good = society->get_consumer_good(product);
    add_demand_signal(product, quantity);
    check_and_reorder_input(product);
    if (!catalog.count(product)) return 0;
    int available = std::min(static_cast<int>(get_inventory_level(consumer_good)), quantity);
    if (available < quantity) {
        log_shortfall(product->product_name, quantity - available);
    }
    double cost = available * consumer_good->price_per_unit;
    if (!person->charge(cost)) {
        return 0;
    } 
    Plan * plan = product_to_plan[product];
    if (plan) {
        plan->outgoing_units_consumed += quantity;
        plan->prd += cost;
    }
    remove_input_from_inventory(consumer_good, quantity);
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

