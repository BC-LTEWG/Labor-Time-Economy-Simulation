#include "Constants.h"
#include "Logger.h"
#include "Sim.h"

Sim& Sim::get_instance() {
    static Sim sim;
    return sim;
}

void Sim::run(SimArgs& args) {
    Sim& sim = get_instance();
    sim.set_params(args);
    sim.run();
}

Sim::Sim() :
    gen{rd()}
{}

unsigned int Sim::get_num_people() {
    return get_instance().args.num_people;
}

unsigned int Sim::get_work_hours_daily() {
    return get_instance().args.work_hours_daily;
}

unsigned int Sim::get_work_days_weekly() {
    return get_instance().args.work_days_weekly;
}

unsigned int Sim::get_num_products() {
    return get_instance().args.num_products;
}

unsigned int Sim::get_products_per_machine() {
    return get_instance().args.products_per_machine;
}

unsigned int Sim::get_num_producers() {
    return get_instance().args.num_producers;
}

unsigned int Sim::get_num_distributors() {
    return get_instance().args.num_distributors;
}

double Sim::get_annual_sickness_chance() {
    return get_instance().args.sickness_chance;
}

double Sim::get_person_ability_stddev() {
    return get_instance().args.ability_stddev;
}

bool Sim::does_json() {
    return get_instance().args.json;
}

void Sim::set_params(SimArgs& args) {
    this->args = args;
}

std::random_device& Sim::get_random_device() {
    return get_instance().rd;
}

std::mt19937& Sim::get_random_generator() {
    return get_instance().gen;
}

int Sim::get_current_time_step() {
	return get_instance().current_time_step;
}

void Sim::run() {
    society = Society::get_instance();
    for (std::size_t i = 0; i < args.time_steps; ++i) {
        society->on_time_step();
        ++current_time_step;
    }
}
