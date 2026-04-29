#pragma once

#include <random>
#include <string>
#include <vector>
#include "Constants.h"
#include "Society.h"

struct SimArgs {
    unsigned int time_steps = NUM_SIM_RUNS;
    unsigned int num_people = STARTING_NUM_PEOPLE;
    unsigned int work_hours_daily = INITIAL_WORK_HOURS_DAILY;
    unsigned int work_days_weekly = INITIAL_WORK_DAYS_WEEKLY;
    unsigned int num_products = STARTING_NUM_PRODUCTS;
    unsigned int products_per_machine = STARTING_PRODUCTS_PER_MACHINE;
    unsigned int num_producers = STARTING_NUM_PRODUCERS;
    unsigned int num_distributors = STARTING_NUM_DISTRIBUTORS;
    double sickness_chance = ANNUAL_SICKNESS_CHANCE;
    double ability_stddev = PERSON_ABILITY_STDDEV;
    bool json = false;
};

class Sim {
    public:
        static Sim& get_instance();
        static void run(SimArgs& args);
        static unsigned int get_num_people();
        static unsigned int get_work_hours_daily();
        static unsigned int get_work_days_weekly();
        static unsigned int get_num_products();
        static unsigned int get_products_per_machine();
        static unsigned int get_num_producers();
        static unsigned int get_num_distributors();
        static double get_annual_sickness_chance();
        static double get_person_ability_stddev();
        static bool does_json();
        static int get_current_time_step();
        static std::random_device& get_random_device();
        static std::mt19937& get_random_generator();
        void set_params(SimArgs& args);
    private:
        Sim();
        void run();
        SimArgs args;
        std::random_device rd;
        std::mt19937 gen;
        int current_time_step;
        Society * society;
};
