#include <algorithm>
#include <climits>
#include <iostream>
#include <string>

#include "Constants.h"
#include "ConsumerGood.h"
#include "Distributor.h"
#include "Good.h"
#include "Logger.h"
#include "Machine.h"
#include "Order.h"
#include "Person.h"
#include "Plan.h"
#include "PriceController.h"
#include "Producer.h"
#include "Product.h"
#include "Sim.h"
#include "Society.h"

Distributor::Distributor(
        const std::unordered_set<Product *>& initial_catalog
        ) :
    Firm()
{
    for (Product * product : initial_catalog) {
        add_to_catalog(product);
    }
    initialize_inventory();
    inject_randomness_into_demand();
}

Logger::Client Distributor::get_client_type() {
    return Logger::DISTRIBUTOR;
}

void Distributor::add_to_catalog(Product * product) {
    catalog.insert(product);
    ConsumerGood * consumer_good = static_cast<ConsumerGood *>(product);
    Good * good = consumer_good->corresponding_good;
    consumer_demands[good] = 
        consumer_demands[consumer_good] = 
        consumer_good->mean_consumption_frequency 
        * Sim::get_num_people() 
        / Sim::get_num_distributors();
    log_catalog_addition(product);
}

void Distributor::initialize_inventory() {
    std::normal_distribution<double>
        input_noise_dist(1.0, DEMAND_PREDICTION_VARIANCE);

    for (std::pair<Product * const, double>& demand : consumer_demands) {
        double noise = std::max(
            input_noise_dist(Sim::get_random_generator()),
            0.01
        );
        input_inventory[demand.first] = 
            get_reorder_threshold(demand.first) * noise;

        log_inventory_level(demand.first, input_inventory[demand.first]);
    }
}


int Distributor::try_sell_goods(
        ConsumerGood * consumer_good,
        int quantity, Person * person
        ) {
    int available = std::min(static_cast<int>(
                get_inventory_level(consumer_good)), quantity);
    if (!available) {
        return 0;
    }
    if (available < quantity) {
        log_shortfall(consumer_good->id, quantity - available);
    }
    double cost = available * consumer_good->price_per_unit;
    if (consumer_good->public_sector) {
        Society::get_instance()->charge_from_public_fund(cost);
    } else if (!person->charge(cost)) {
        return 0;
    }
    account += cost;
    remove_input_from_inventory(consumer_good, available);
    PriceController::get_instance()->report_distribution(consumer_good, available);
    return available;
}

void Distributor::check_and_reorder_input(Product * product) {
    if (product->product_type != Product::ProductType::kTypeConsumerGood) {
        Firm::check_and_reorder_input(product);
        return;
    }
    double resupply_deficit = get_resupply_deficit(product);
    if (resupply_deficit <= 0) {
        return;
    }
    ConsumerGood * consumer_good = static_cast<ConsumerGood *>(product);
    Good * good = consumer_good->corresponding_good;

    double reorder_threshold = get_reorder_threshold(consumer_good);
    log_demand(consumer_good, reorder_threshold);

    // double inventory = get_pending_inventory(consumer_good);
    // if (inventory >= reorder_threshold || !reorder_threshold) {
    //     return;
    // }
    log_reorder_attempt(product);
    double lead_time = 
        static_cast<int>(get_inventory_level(good)) / resupply_deficit;
    lead_time = std::min(
        lead_time,
        FIRM_STOCKPILE_DURATION * FIRM_REORDER_MAX_PROP
    );
    int order_quantity = std::ceil(lead_time * resupply_deficit);
    Order * order = new Order(
            consumer_good,
            order_quantity,
            this,
            lead_time
            );

    Plan * plan = draft_plan_for_order(order);
    if (!plan) {
        log_reorder_failure(product);
        return;
    }
    product_to_outbound_orders[consumer_good].insert(order);
    if (start_plan(plan)) {
        log_pursued_plan(plan);
    }
}

void Distributor::log_shortfall(unsigned int product_id, int shortfall) {
    Logger::log(
            Logger::DISTRIBUTOR,
            id,
            "product_shortfall",
            LogPair("product_id", product_id),
            LogPair("shortfall", shortfall)
            );
}

