#include <iostream>

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
        unsigned j = 0;
        unsigned int value = 0;
        double dvalue = 0.0;
        char flag = 0;
        if (argv[i][j] == '-') {
            ++j;
            switch (argv[i][j]) {
                case 'n':
                case 'p':
                case 'h':
                case 'w':
                case 'o':
                case 'm':
                case 'r':
                case 'd':
                case 'e':
                    flag = argv[i][j];
                    ++i;
                    value =
                        static_cast<unsigned int>(strtol(argv[i], NULL, 10));
                    if (value <= 0 && flag != 'e') {
                        error = true;
                    } else {
                        switch (flag) {
                            case 'n':
                                args.time_steps = value;
                                break;
                            case 'p':
                                args.num_people = value;
                                break;
                            case 'h':
                                args.work_hours_daily = value;
                                break;
                            case 'w':
                                args.work_days_weekly = value;
                                break;
                            case 'o':
                                args.num_products = value;
                                break;
                             case 'm':
                                args.products_per_machine = value;
                                break;
                            case 'r':
                                args.num_producers = value;
                                break;
                            case 'd':
                                args.num_distributors = value;
                                break;
                            case 'e':
                                args.seed = value;
                                args.fixed_seed = true;
                                break;
                            default:
                                error = true;
                                break;
                        }
                    }
                    break;

                case 's':
                    ++i;
                    dvalue = strtod(argv[i], NULL);
                    if (dvalue <= 0.0 || dvalue >= 1.0) {
                        error = true;
                    } else {
                        args.sickness_chance = dvalue;
                    }
                    break;
                case 'v':
                    ++i;
                    dvalue = strtod(argv[i], NULL);
                    if (dvalue <= 0.0) {
                        error = true;
                    } else {
                        args.ability_stddev = dvalue;
                    }
                    break;

                case 'j':
                    args.json = true;
                    break;
                default:
                    error = true;
                    break;

            }
            if (error) {
                break;
            }
        } else {
            error = true;
            break;
        }
    }
    if (error) {
        print_usage();
    }
}

int main(int argc, const char ** argv) {
    SimArgs args;
    set_params(argc, argv, args);
    Sim::run(args);
	return EXIT_SUCCESS;
}
