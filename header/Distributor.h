#pragma once

#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "Firm.h"

constexpr double PRODUCTION_THRESHOLD = 1.5;
struct Product;
class Distributor;
class Person;
class Producer;

class Distributor : public Firm {
  public:
    Distributor(
        Society * society,
        const std::unordered_set<Product *>& initial_catalog
    );
    Logger::Client get_client_type() override;
    void on_time_step() override;
    int try_sell_goods(Product& product, int quantity, Person * person);
    
  private:
    std::unordered_set<Product *> get_products_to_reorder() override;
    std::unordered_map<Product *, Plan *> product_to_plan;
    void log_shortfall(std::string product_name, int shortfall);
};
