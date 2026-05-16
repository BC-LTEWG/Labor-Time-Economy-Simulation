#include <iostream>
#include <string>
#include <cstdlib>
#include <map>

#include "Sim.h"

void print_usage() {
    std::cout << "Usage: sim [<args> ...]" << std::endl;
    std::cout << "\t-n N: Run the simulation for N time steps." << std::endl;
    std::cout << "\t-p N: Simulate a society with N people." << std::endl;
    std::cout << "\t-h N: Set the workday to N hours." << std::endl;
    std::cout << "\t-w N: Set the work week to N days." << std::endl;
    std::cout << "\t-o N: Set the initial number of products to N." << std::endl;
    std::cout << "\t-m N: Set the initial number of products per machine to N." << std::endl;
    std::cout << "\t-r N: Set the initial number of producers to N." << std::endl;
    std::cout << "\t-d N: Set the initial number of distributors to N." << std::endl;
    std::cout << "\t-e N: Set the random seed to N." << std::endl;
    std::cout << "\t-s N: Set the annual chance of an agent getting sick." << std::endl;
    std::cout << "\t-j: Write JSON log traces to stdout." << std::endl;
}

enum class argType {
    TimeSteps,
    People,
    WorkHours,
    WorkDays,
    Products,
    ProductsPerMachine,
    Producers,
    Distributors,
    Seed
};

void set_params(int argc, const char ** argv, SimArgs& args) {
    bool error = false;

    static const std::map<std::string, argType> valid_args = {
        {"-n", argType::TimeSteps}, {"--time-steps", argType::TimeSteps},
        {"-p", argType::People}, {"--people", argType::People},
        {"-h", argType::WorkHours}, {"--work-hours", argType::WorkHours},
        {"-w", argType::WorkDays}, {"--work-days", argType::WorkDays},
        {"-o", argType::Products}, {"--products", argType::Products},
        {"-m", argType::ProductsPerMachine}, {"--products-per-machine", argType::ProductsPerMachine},
        {"-r", argType::Producers}, {"--producers", argType::Producers},
        {"-d", argType::Distributors}, {"--distributors", argType::Distributors},
        {"-e", argType::Seed}, {"--seed", argType::Seed},
    };


    for (int i = 1; i < argc; ++i) {
        unsigned int value = 0;
        double dvalue = 0.0;
        std::string arg = argv[i];

        if(arg == "-j" || arg == "--json") { //"j" needs no additional value
            args.json = true;
            continue;
        }
        if (i + 1 >= argc) {
            error = true;
            break;
        }
        
        if(valid_args.count(arg)) {
            long negative_check = strtol(argv[++i], NULL, 10);
            bool seed_exception = (arg == "-e" || arg == "--seed");
            if(negative_check < 0 || (negative_check == 0 && !seed_exception)) {
                error = true;
            }
            else {
                value = static_cast<unsigned int>(negative_check);

                switch(valid_args.at(arg)) {
                    case argType::TimeSteps: {
                        args.time_steps = value;
                        break;
                    }
                    case argType::People: {
                        args.num_people = value;
                        break;
                    }
                    case argType::WorkHours: {
                        args.work_hours_daily = value;
                        break;
                    }
                    case argType::WorkDays: {
                        args.work_days_weekly = value;
                        break;
                    }
                    case argType::Products: {
                        args.num_products = value;
                        break;
                    }
                    case argType::ProductsPerMachine: {
                        args.products_per_machine = value;
                        break;
                    }
                    case argType::Producers: {
                        args.num_producers = value;
                        break;
                    }
                    case argType::Distributors: {
                        args.num_distributors = value;
                        break;
                    }
                    case argType::Seed: {
                        args.seed = value;
                        args.fixed_seed = true;
                        break;
                    }
                }
            }
        }
        else if(arg == "-s" || arg == "--sick-chance") {
            dvalue = strtod(argv[++i], NULL);
            if (dvalue <= 0.0 || dvalue >= 1.0) {
                error = true;
            } else {
                args.sickness_chance = dvalue;
            }
        }
        else if(arg == "-v" || arg == "--ability-stddev") {
            dvalue = strtod(argv[++i], NULL);
            if (dvalue <= 0.0) {
                error = true;
            } else {
                args.ability_stddev = dvalue;
            }
        } else {
            error = true;
            break;
        }
    }
    if(error) {
        print_usage();
        exit(1);
    }
}

int main(int argc, const char ** argv) {
    SimArgs args;
    set_params(argc, argv, args);
    Sim::run(args);
	return EXIT_SUCCESS;
}
