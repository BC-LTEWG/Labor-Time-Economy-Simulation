#pragma once

#include <Eigen/Dense>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Agent.h"
#include "Constants.h"

struct Ability;
struct ConsumerGood;
struct Good;
struct Machine;
struct Product;
class Distributor;
class Firm;
class TradeFirm;  // Firm handling exchanges with the CE.
class Person;
class Producer;

class Society : public Agent {
    public:
        static Society * get_instance();
        unsigned int get_id() override;
        void on_time_step() override;

        std::vector<Ability *>& get_abilities();
        std::vector<Ability *>& get_distribution_abilities();
        std::vector<Good *>& get_goods();
        std::vector<ConsumerGood *>& get_consumer_goods();
        std::vector<Machine *>& get_machines();
        std::vector<Product *>& get_products();
        double get_underlying_living_labor_per_unit(Product * product);
        void set_underlying_living_labor_per_unit(Product * product, double ll);
        std::vector<Firm *>& get_firms();
        std::vector<Distributor *>& get_distributors();
        std::unordered_set<Person *>& get_unemployed_people();
        void retire_person(Person * person);
        unsigned int get_current_work_hours_daily();
        unsigned int get_current_work_days_weekly();
        double get_work_week_proportion();
        int get_initial_account();
        std::unordered_map<Product *, double>& get_gross_hourly_demand_per_capita();
        std::vector<Producer *>& get_producers();
        std::vector<Producer *>& get_suppliers(Product * product);
        void update_busyness();
        double get_busyness();
        bool is_within_work_schedule();
        void update_average_account();
        double get_average_account();
        double get_total_employment();
        std::unordered_map<Product *, int>& get_number_of_producers_for_product();
        void pay_into_public_fund(double amount);
        void charge_from_public_fund(double amount);

    private:
        Society();
        void initialize();

        unsigned int id = 0;
        Person * birth_person();
        void set_initial_products();
        void add_consumer_goods();
        void set_product_prices_production_consumption();
        void populate_io_matrix_and_labor_vector(
                Eigen::MatrixXd&,
                Eigen::VectorXd&
                );
        void set_initial_prices(
                const Eigen::MatrixXd&,
                const Eigen::VectorXd&,
                const std::vector<ConsumerGood *>&,
                std::size_t
                );
        double normalize_io_matrix(Eigen::MatrixXd&);
        void apply_machine_scaling(Eigen::MatrixXd&, Eigen::VectorXd&);
        void apply_normalization_to_products(const Eigen::MatrixXd&, double);
        double get_production_spectral_radius(const Eigen::MatrixXd&);
        void check_expand_public_sector();
        void check_update_work_week();
        void log_io_matrix(Eigen::MatrixXd&, size_t);
        void log_vector(Eigen::VectorXd&, std::string, size_t);
        void log_consumption_frequencies();
        void log_public_fund();
        void log_public_revenue(double);
        void log_public_expenditure(double);
        void log_public_sector_expansion(ConsumerGood * consumer_good);
        void log_busyness();
        void log_total_employment();
        void log_work_hours_weekly();
        void set_abilities(std::vector<Ability *>& abilities, std::vector<Ability *>& distribution_abilities);
        void set_initial_account(double& initial_account, const std::vector<ConsumerGood *>& consumer_goods);

        double public_fund = 0.0;
        std::vector<Person *> people;
        std::vector<Good *> goods;
        std::vector<ConsumerGood *> consumer_goods;
        std::vector<Machine *> machines;
        std::vector<Product *> products;
        std::unordered_map<Product *, double> underlying_living_labor_per_unit;
        std::vector<Firm *> firms;
        std::vector<Producer *> producers;
        TradeFirm * trade_firm = nullptr;  // Firm responsible for LTE-CE trade.
        std::unordered_map<Product *, std::vector<Producer *>>
            product_to_suppliers;
        std::vector<Distributor *> distributors;
        std::unordered_map<Product *, std::vector<Distributor *>>
            product_to_distributors;
        std::vector<Ability *> abilities;
        std::vector<Ability *> distribution_abilities;
        unsigned int current_work_hours_daily = INITIAL_WORK_HOURS_DAILY;
		unsigned int current_work_days_weekly = INITIAL_WORK_DAYS_WEEKLY;
        std::unordered_set<Person *> unemployed_people;
        double initial_account;
        double average_account;
        double busyness;
        std::unordered_map<Product *, double> gross_hourly_demand_per_capita;
        std::unordered_map<Product *, int> product_to_number_of_producers;
};
