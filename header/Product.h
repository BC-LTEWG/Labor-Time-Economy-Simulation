#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Person.h"

struct Machine;

struct Product {
    enum ProductType { TYPE_GOOD, TYPE_MACHINE, TYPE_CONSUMER_GOOD, TYPE_UNKNOWN };
    Product(int id, const std::string& name);
    void set_inputs(std::vector<Product *>& products);
    void set_machines(std::vector<Machine*> machines);
    void log_mean_consumption_frequency();
    int id;
    std::string product_name;
    ProductType product_type;
    double price_per_unit;
    int order_size;
    int mean_consumption_period;
    std::vector<Machine *> machines_needed;
    std::unordered_map<Product *, double> inputs_per_unit;
 	double living_labor_per_unit; 
    double societal_living_labor_per_unit;
	std::vector<Person::Ability> required_abilities;
	double mean_consumption_frequency;
};

