#include <iostream>
#include <string>
#include <cstdlib>

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

void set_params(int argc, const char ** argv, SimArgs& args) {
    bool error = false;
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
        
        if(arg == "-n" || arg == "--time-steps" ||
           arg == "-p" || arg == "--people" || 
           arg == "-h" || arg == "--work-hours" || 
           arg == "-w" || arg == "--work-days" || 
           arg == "-o" || arg == "--products" || 
           arg == "-m" || arg == "--products-per-machine" || 
           arg == "-r" || arg == "--producers" || 
           arg == "-d" || arg == "--distributors" ||
           arg == "-e" || arg == "--seed") {

            long negative_check = strtol(argv[++i], NULL, 10);

            bool seed_exception = (arg == "-e" || arg == "--seed");
            if(negative_check < 0 || (negative_check == 0 && !seed_exception)) error = true; //prevents "0" seed from sending error
            else {
                value =
                    static_cast<unsigned int>(negative_check);
                    if (arg == "-n" || arg == "--time-steps") args.time_steps = value;
                    else if (arg == "-p" || arg == "--people") args.num_people = value;
                    else if (arg == "-h" || arg == "--work-hours") args.work_hours_daily = value;
                    else if (arg == "-w" || arg == "--work-days") args.work_days_weekly = value;
                    else if (arg == "-o" || arg == "--products") args.num_products = value;
                    else if (arg == "-m" || arg == "--products-per-machine") args.products_per_machine = value;
                    else if (arg == "-r" || arg == "--producers") args.num_producers = value;
                    else if (arg == "-d" || arg == "--distributors") args.num_distributors = value;
                    else if (seed_exception) {
                        args.seed = value;
                        args.fixed_seed = true;
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
