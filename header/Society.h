#pragma once

#include <Eigen/Dense>
#include <unordered_map>
#include <unordered_set>

#include "Agent.h"
#include "Constants.h"

struct Product;
class Distributor;
class Firm;
struct Machine;
struct ConsumerGood;
class Person;
class Producer;

class Society : public Agent {
    public:
        static Society * get_instance();
        unsigned int get_id() override;
        void on_time_step() override;
        std::vector<Product *>& get_goods();
        std::vector<Product *>& get_products();
        ConsumerGood * get_consumer_good(Product * product);
        void add_consumer_good(Product * product);
        std::vector<Distributor *>& get_distributors();
        std::unordered_set<Person *>& get_unemployed_people();
        void retire_person(Person * person);
        unsigned int get_current_work_hours_daily();
        unsigned int get_current_work_days_weekly();
        int get_initial_account();
        std::unordered_map<Product *, double>& get_initial_production();
        std::vector<Producer *>& get_producers();
        double get_busyness();
        double get_total_employment();
        void log_total_employment();

    private:
        Society();
        unsigned int id;
        Person * birth_person();
        void set_initial_products();
        void set_product_prices_production_consumption();
        std::unordered_map<Product *, std::size_t> get_product_to_index_map();
        void populate_io_matrix_and_labor_vector(
                std::unordered_map<Product *, std::size_t>&,
                Eigen::MatrixXd&,
                Eigen::VectorXd&
                );
        void adjust_io_matrix(Eigen::MatrixXd&, double max_eigenvalue);
        void set_initial_account();
        void update_work_hours_daily();
        void log_io_matrix(Eigen::MatrixXd&, size_t);
        void log_labor_vector(Eigen::VectorXd&, size_t);
        void log_consumption_frequencies();

        std::vector<Person *> people;
        std::vector<Product *> goods;
        std::vector<Machine *> machines;
        std::vector<Product *> products;
        std::unordered_map<Product *, std::size_t> product_to_index;
        std::unordered_map<Product *, ConsumerGood *> consumer_goods;
        std::vector<Firm *> firms;
        std::vector<Producer *> producers;
        std::vector<Distributor *> distributors;
        std::unordered_map<Product *, std::vector<Distributor *>>
            product_to_distributors;
        unsigned int current_work_hours_daily = INITIAL_WORK_HOURS_DAILY;
		unsigned int current_work_days_weekly = INITIAL_WORK_DAYS_WEEKLY;
        std::unordered_set<Person *> unemployed_people;
        double initial_account;
        std::unordered_map<Product *, double> initial_production;
};
