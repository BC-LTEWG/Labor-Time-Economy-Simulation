#include <algorithm>
#include <iostream>
#include <random>
#include <set>

#include "Constants.h"
#include "Logger.h"
#include "Machine.h"
#include "Product.h"
#include "Sim.h"

struct Machine;

Product::Product(int id, const std::string& name) :
    id{id}, product_name{name},
    product_type{TYPE_GOOD}
{
    static std::uniform_int_distribution<>
        order_size_dist(PRODUCT_ORDER_SIZE_MIN, PRODUCT_ORDER_SIZE_MAX);
    order_size = order_size_dist(Sim::get_random_generator());
    static std::uniform_real_distribution<>
        living_labor_dist(
                PRODUCT_LABOR_PER_UNIT_MIN,
                PRODUCT_LABOR_PER_UNIT_MAX
                );
    living_labor_per_unit = 
        societal_living_labor_per_unit = 
        living_labor_dist(Sim::get_random_generator());
    for (int i = 0; i < Person::NUM_ABILITIES; i++) {
        required_abilities.push_back((Person::Ability) i);
    }
    std::shuffle(required_abilities.begin(), required_abilities.end(), Sim::get_random_generator());
    static std::uniform_int_distribution<>
        ability_count_dist(1, PRODUCT_ABILITY_COUNT_MAX);
    required_abilities.resize(ability_count_dist(Sim::get_random_generator()));
    static std::uniform_real_distribution<>
        consumption_freq_dist(0, 0.5);
    mean_consumption_frequency = consumption_freq_dist(Sim::get_random_generator());
    mean_consumption_period = static_cast<int>(std::ceil(1 / mean_consumption_frequency));
}

void Product::set_inputs(std::vector<Product *>& goods) {
    int max_num_inputs =
        std::max<int>(PRODUCT_NUM_INPUTS_MAX, PRODUCT_NUM_INPUTS_MAX);
    max_num_inputs = std::min<int>(max_num_inputs, goods.size());
    static std::uniform_int_distribution<>
        num_inputs_dist(PRODUCT_NUM_INPUTS_MIN, max_num_inputs);
    const std::size_t num_inputs = num_inputs_dist(Sim::get_random_generator());
    std::uniform_int_distribution<>
        product_input_index_dist(0, goods.size() - 1);
    std::set<int> indices;
    while (indices.size() < num_inputs) {
        indices.insert(product_input_index_dist(Sim::get_random_generator()));
    }
    static std::uniform_real_distribution<>
        input_per_unit_dist(
                PRODUCT_INPUT_PER_UNIT_MIN,
                PRODUCT_INPUT_PER_UNIT_MAX
                );
    for (int index : indices) {
        inputs_per_unit[goods[index]] = input_per_unit_dist(Sim::get_random_generator());
    }
}

void Product::set_machines(std::vector<Machine*> machines) {
    if (!machines.size()) return;
    const unsigned int global_num_machines =
        Sim::get_num_products() / Sim::get_products_per_machine();
    const int num_machines_max =
        global_num_machines / MAX_PROPORTION_OF_MACHINES_PER_PRODUCT;
    static std::uniform_int_distribution<>
        num_machines_dist(PRODUCT_NUM_MACHINES_MIN, num_machines_max);
    const std::size_t num_machines = num_machines_dist(Sim::get_random_generator());
    std::uniform_int_distribution<>
        product_machine_index_dist(0, machines.size() - 1);
    std::set<int> indices;
    while (indices.size() < num_machines) {
        indices.insert(product_machine_index_dist(Sim::get_random_generator()));
    }
    for (int index : indices) {
        machines_needed.push_back(machines[index]);
    }
 }


void Product::log_mean_consumption_frequency() {
    Logger::get_instance()->log(Logger::PRODUCT, "mean_consumption_frequency", id, mean_consumption_frequency);
}
