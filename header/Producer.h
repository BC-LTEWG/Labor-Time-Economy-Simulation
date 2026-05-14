#pragma once

#include <unordered_set>
#include <string>
#include <unordered_map>
#include <vector>

#include "Constants.h"
#include "Firm.h"

struct Order;
struct Product;
class Distributor;
struct Machine;
class Person;

class Producer : public Firm {
  public:
    Producer(
        Society * society,
        const std::unordered_set<Product *>& initial_catalog
    );
    Logger::Client get_client_type() override;
    void on_time_step() override;
    bool can_produce(Product * product);
	Order * draft_plan_and_return_order(const Order * order);
	void drop_order(Firm * customer);
	void pursue_order(Firm * customer);
    double get_input_products_account();

  private:
    std::unordered_map<Firm *, Plan *> customer_to_draft_plan;
    int get_max_order_quantity(Product * product);
    void add_order_input_demand_signals(const Order * order);
    double calculate_quantity_produced_from_worker_suitability(Plan * plan);
    bool is_within_work_schedule() const;

	void start_plan(Plan * plan);
	void move_plan_forward_one_step(Plan * plan);
	void end_plan(Plan * plan);
	void move_plans_forward_one_step();
    std::unordered_set<Product *> get_products_to_reorder() override;
    void log_plans();
    void log_draft_plan(const Plan * draft_plan);
    void log_dropped_order(const Order * order);
    void log_pursued_plan(const Plan * draft_plan);
    void log_ended_plan(const Plan * plan);
};
