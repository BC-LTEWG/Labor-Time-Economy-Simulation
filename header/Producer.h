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

  private:
    std::unordered_map<Firm *, Plan *> customer_to_draft_plan;
    int get_max_order_quantity(Product * product);

    std::unordered_set<Product *> get_products_to_reorder() override;
    void log_draft_plan(const Plan * draft_plan);
    void log_dropped_order(const Order * order);
};
