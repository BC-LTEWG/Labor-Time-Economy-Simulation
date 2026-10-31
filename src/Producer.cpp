#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <numeric>
#include <vector>

#include "ConsumerGood.h"
#include "Distributor.h"
#include "Good.h"
#include "Logger.h"
#include "Machine.h"
#include "Order.h"
#include "Person.h"
#include "Plan.h"
#include "Producer.h"
#include "Product.h"
#include "Sim.h"
#include "Society.h"

Producer::Producer(
        const std::unordered_set<Product *>& initial_catalog
        ) :
    Firm()
{
    for (Product * product : initial_catalog) {
        add_to_catalog(product);
    }
}

Logger::Client Producer::get_client_type() {
    return Logger::PRODUCER;
}

void Producer::add_to_catalog(Product * product) {
    catalog.insert(product);
    double gross_hourly_demand_per_capita =
        Society::get_instance()->get_gross_hourly_demand_per_capita()[product];
    double gross_hourly_demand = gross_hourly_demand_per_capita * Sim::get_num_people();
    double gross_demand_per_producer = 
        gross_hourly_demand /
        Society::get_instance()->get_number_of_producers_for_product()[product];

    double starting_num_firms =
        Sim::get_num_producers() +
        Sim::get_num_distributors();

    double average_team_size = std::max<double>(
            Sim::get_num_people() / starting_num_firms,
            1.0
        );
    for (Machine * machine : product->machines_needed) {
        double machine_use_per_unit =
            product->living_labor_per_unit / 
            (machine->lifetime * average_team_size);
        producer_demands[machine] +=
            machine_use_per_unit * gross_demand_per_producer;
    }
    for (std::pair<Good * const, double>& input :
            product->inputs_per_unit) {
        producer_demands[input.first] +=
            input.second * gross_demand_per_producer;
    }
    log_catalog_addition(product);
}

void Producer::initialize_inventory() {

    for (std::pair<Product * const, double>& demand : producer_demands) {
        double input_amount_added = get_reorder_threshold(demand.first);
        if (demand.first->product_type == Product::ProductType::kTypeMachine) {
            Machine * machine = static_cast<Machine *>(demand.first);
            std::uniform_int_distribution<> machine_initial_age_dist(
                static_cast<int>(
                    INITIAL_MACHINE_AGE_MIN_PROP * machine->lifetime
                ),
                machine->lifetime
            );
            input_amount_added = std::max(
                input_amount_added,
                machine_initial_age_dist(Sim::get_random_generator()) /
                machine->lifetime
            );
        }
        input_inventory[demand.first] = input_amount_added;
        log_input_inventory_level(demand.first, input_amount_added);
    }
}

bool Producer::can_produce(Product * product) {
    return catalog.count(product);
}

double Producer::get_max_order_quantity(
    const Order * order,
    std::vector<Product *>& missing_resources
) {
    double max_order_quantity = std::numeric_limits<double>::infinity();
    for (std::pair<Good * const, double>& input : order->product->inputs_per_unit) {
        if (input.second <= 0.0) {
            continue;
        }
        double input_max_order_quantity = 
                get_input_inventory_level(input.first) / input.second;
        if (input_max_order_quantity < 1.0) {
            missing_resources.push_back(input.first);
        }
        max_order_quantity = 
            std::min(max_order_quantity, input_max_order_quantity);
    }
    for (Machine * machine : order->product->machines_needed) {
        double desired_machine_usage =
            order->requested_turnaround_time
            * Society::get_instance()->get_work_week_proportion()
            / machine->lifetime;
        if (desired_machine_usage > get_input_inventory_level(machine)) {
            missing_resources.push_back(machine);
            max_order_quantity = 0;
        }
    }
    return max_order_quantity;
}

Order * Producer::draft_plan_and_return_order(const Order * order) {
    Order * return_order = new Order(
        order->product,
        order->quantity,
        order->customer,
        order->requested_turnaround_time
    );
    std::vector<Product *> missing_resources;
    return_order->quantity = std::min(
        return_order->quantity,
        static_cast<int>(get_max_order_quantity(
            order,
            missing_resources
        ))
    );
    Plan * draft_plan = nullptr;
    if (return_order->quantity <= 0) {
        log_drafting_failure_inputs(order->product, missing_resources);
    } else {
        draft_plan = draft_plan_for_order(return_order);
    }
    if (!draft_plan) {
        delete draft_plan;
        return_order->status = Order::kOrderRejected;
        return return_order;
    }

    customer_to_draft_plan[order->customer] = draft_plan;
    return return_order;
}

void Producer::drop_order(Firm * customer) {
    log_dropped_order(customer_to_draft_plan[customer]->order);
    customer_to_draft_plan[customer] = nullptr;
}

void Producer::pursue_order(Firm * customer) {
    Plan * plan = customer_to_draft_plan[customer];
    if (!plan) {
        std::cerr << "Error: pursuing order from firm with no approved draft plan" << std::endl;
        return;
    }
    customer_to_draft_plan[customer] = nullptr;
    if (start_plan(plan)) {
        log_pursued_plan(plan);
    }
}

void Producer::log_dropped_order(const Order * order) {
    Logger::log(
            Logger::PRODUCER,
            id,
            "dropped_order",
            LogPair("product_id", order->product->id),
            LogPair("quantity", order->quantity)
            );
}
