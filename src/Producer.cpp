#include <algorithm>
#include <iostream>
#include <unordered_set>
#include <numeric>

#include "Distributor.h"
#include "Logger.h"
#include "PriceController.h"
#include "Person.h"
#include "Producer.h"
#include "Product.h"
#include "Sim.h"
#include "Society.h"

Producer::Producer(
        Society * society,
        const std::unordered_set<Product *>& initial_catalog
        ) :
    Firm(society, initial_catalog) {
    std::unordered_set<Machine *> initial_machines;
    for (Product * product : initial_catalog) {
        for (Machine * machine : product->machines_needed) {
            initial_machines.insert(machine);
        }
    }
    for (Machine * machine : initial_machines) {
        machines.push_back(machine);
    }
    for (Product * product : catalog) {
        for (std::pair<Product * const, double>& input :
                product->inputs_per_unit) {
            this->input_inventory[input.first] +=
                input.second * 
                society->get_initial_production()[product] * 
                (FIRM_STOCKPILE_DURATION + FIRM_DEMAND_WINDOW_MIN) *
                Sim::get_num_people() * Sim::get_num_products() / Sim::get_num_producers();
        }
    }
    for (Product * product : get_products_to_reorder()) {
        log_inventory_level(product, input_inventory[product]);
    } 
    log_catalog();
}

Logger::Client Producer::get_client_type() {
    return Logger::PRODUCER;
}

void Producer::on_time_step() {
    Firm::on_time_step();
	move_plans_forward_one_step();
    if (plans_in_progress.size()) {
        log_plans();
    }
}

bool Producer::can_produce(Product * product) {
    return catalog.count(product);
}

bool Producer::has_sufficient_inputs_for_order(const Order * order) {
    for (std::pair<Product * const, double>& input : order->product->inputs_per_unit) {
        if (input_inventory[input.first] < input.second * order->quantity) {
            return false;
        }
    }
    return true;
}

int Producer::draft_plan_or_reject(Order * order) {
    if (!has_sufficient_inputs_for_order(order)) {
        return DRAFT_ORDER_REJECTED;
    }
	Plan * draft_plan = draft_plan_with_required_abilities(order,
            order->product->required_abilities);
    if (draft_plan->workers.empty()) {
        delete draft_plan;
        return DRAFT_ORDER_REJECTED;
    }
	order_to_draft_plan[order] = draft_plan;
    log_draft_plan(draft_plan);
	return draft_plan->predicted_turnaround_time;
}

void Producer::drop_order(Order * order) {
    log_dropped_order(order);
	order_to_draft_plan.erase(order);
}

void Producer::add_order_input_demand_signals(const Order * order) {
    for (std::pair<Product * const, double>& input : order->product->inputs_per_unit) {
        add_demand_signal(input.first, input.second * order->quantity);
    }
}

bool Producer::pursue_order(Order * order) {
	if (!order_to_draft_plan.count(order)) {
		return false;
	}
	Plan * draft_plan = order_to_draft_plan[order];
    add_order_input_demand_signals(order);
    for (Person * worker : draft_plan->workers) {
        move_worker_off_standby(worker);
    }

	// move draft_plan to plans_in_progress
	order_to_draft_plan[order] = nullptr;
	plans_in_progress.push_back(draft_plan);
    log_pursued_plan(draft_plan);
    society->log_total_employment();
	return true;
}

void Producer::start_plan(Plan * plan) {
	// simplification: consume all raw materials at start of plan
	for (std::pair<Product * const, double>& input :
            plan->order->product->inputs_per_unit) {
        int required_input = static_cast<int>(
            std::ceil(input.second * plan->order->quantity)
            );
        remove_input_from_inventory(input.first, required_input);
        check_and_reorder_input(input.first);
	}
    pooled_input_value_account += plan->raw_materials;
    plan->raw_materials = 0;
    plan->order->status = Order::ORDER_IN_PROGRESS;
}

void Producer::move_plan_forward_one_step(Plan * plan) {
	int labor_hours_done = plan->workers.size();
    double quantity_produced = calculate_quantity_produced_from_worker_suitability(plan);
    if (quantity_produced <= 0.0) {
        return;
    }

    double raw_materials_used =
        plan->raw_materials_remaining *
        quantity_produced /
        plan->order->quantity;

	//pay workers
	for (Person * worker : plan->workers) {
		worker->register_hours_worked(1);
        worker->register_busyness();
	}
    plan->labor_hours_remaining -= labor_hours_done;
    plan->raw_materials_remaining = std::max(
            0.0,
            plan->raw_materials_remaining - raw_materials_used
            );
    plan->total_hours_remaining =
        plan->labor_hours_remaining + plan->raw_materials_remaining;
    plan->quantity_remaining -= quantity_produced;
}

double Producer::get_input_products_account() {
    return pooled_input_value_account;
}

void Producer::end_plan(Plan * plan) {
    log_ended_plan(plan);
    plan->order->status = Order::ORDER_FINISHED;
	input_inventory[plan->order->product] += plan->order->quantity;
	input_inventory[plan->order->product] -= plan->order->quantity;
    plan->order->customer->receive_shipment(plan);
    recorded_living_labor_per_unit[plan->order->product] = 
        (double) (plan->labor_hours - plan->labor_hours_remaining) 
        / (plan->order->quantity - plan->quantity_remaining); 
    PriceController::get_instance()->update_price(plan);
    for (Person * worker : plan->workers) {
        standby_workers.insert(worker);
    }
}

double Producer::calculate_quantity_produced_from_worker_suitability(Plan * plan) {
    double total_worker_suitability = 0.0;
    for (Person * worker : plan->workers) {
        total_worker_suitability +=
            worker->suitability(plan->order->product->required_abilities);
    }
    if (total_worker_suitability <= 0.0) {
        return 0.0;
    }
    return total_worker_suitability / plan->order->product->living_labor_per_unit;
}


bool Producer::is_within_work_schedule() const {
    return Sim::get_current_time_step() % DAY <
        Society::get_instance()->get_current_work_hours_daily() &&
        Sim::get_current_time_step() / DAY % 7 <
        Society::get_instance()->get_current_work_days_weekly() ;
}

void Producer::move_plans_forward_one_step() {
	for (
            auto iter = plans_in_progress.begin();
            iter != plans_in_progress.end();
            ++iter
            ) {
		Plan * plan = *iter;
        if (plan->order->status == Order::ORDER_REQUESTED) {
			start_plan(plan);
		}
        if (plan->order->status == Order::ORDER_IN_PROGRESS &&
			is_within_work_schedule()) {
			move_plan_forward_one_step(plan);
		}
		if (plan->quantity_remaining <= 0) {
			end_plan(plan);
			iter = plans_in_progress.erase(iter);
			--iter; 
		}
	}
}

std::unordered_set<Product *> Producer::get_products_to_reorder() {
    std::unordered_set<Product *> products_to_reorder;
    for (Product * product : catalog) {
        for (std::pair<Product * const, double>& input :
                product->inputs_per_unit) {
            products_to_reorder.insert(input.first);
        }
    }
    return products_to_reorder;
}

void Producer::log_plans() {
    for (Plan * plan : plans_in_progress) {
        Logger::get_instance()->log(
                Logger::PRODUCER,
                "plan_quantity_remaining",
                id,
                plan->order->product->product_name,
                plan->quantity_remaining
                );
    }
}

void Producer::log_draft_plan(const Plan * draft_plan) {
    Logger::get_instance()->log(
            Logger::PRODUCER,
            "draft_plan",
            id,
            draft_plan->order->product->product_name,
            draft_plan->order->quantity
            );
}

void Producer::log_dropped_order(const Order * order) {
    Logger::get_instance()->log(
            Logger::PRODUCER,
            "dropped_order",
            id,
            order->product->product_name,
            order->quantity
            );
}

void Producer::log_pursued_plan(const Plan * draft_plan) {
    Logger::get_instance()->log(
            Logger::PRODUCER,
            "pursued_plan",
            id,
            draft_plan->order->customer->get_id(),
            draft_plan->order->product->id,
            draft_plan->order->quantity
            );
}

void Producer::log_ended_plan(const Plan * plan) {
    Logger::get_instance()->log(
            Logger::PRODUCER,
            "ended_plan",
            id,
            plan->order->product->product_name,
            plan->order->quantity
            );
}
