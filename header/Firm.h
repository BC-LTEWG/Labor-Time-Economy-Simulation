#pragma once

#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>

#include "Agent.h"
#include "Logger.h"
#include "Person.h"

struct Ability;
struct Machine;
struct Order;
struct Product;
class Firm;
class Producer;
class Society;

class Firm : public Agent {
  public:
    Firm();
    unsigned int get_id() override;
    virtual Logger::Client get_client_type() = 0;
    virtual void on_time_step() override;
    virtual void add_to_catalog(Product * product) = 0;
    virtual void initialize_inventory() = 0;
    void inject_randomness_into_demand();
    double get_avg_productivity();
    virtual double get_inventory_level(Product * product);
    void receive_shipment(Order * order);
    virtual void receive_shipment(Plan * plan);
    void receive_ce_import(
        Product * product,
        double quantity,
        double cost_in_labor_hours
    );
    void process_payment(Plan * plan, double transaction_amount);
    double get_busyness();
    double get_account();
    int get_num_workers();
    std::vector<Person *> propose_transfer(int workers_wanted);
    void hire_worker(Person * worker);
    void unassign_worker(Person * worker);
    static bool is_within_work_schedule(Plan * plan);
    static double get_work_week_proportion(Plan * plan);

  protected:
    unsigned int id;
    std::unordered_map<Product *, double> average_team_sizes;
    double account = 0.0;
    std::unordered_set<Person *> workers,
        standby_workers;
    std::unordered_map<Product *, double> input_inventory;
    std::unordered_set<Product *> catalog;
    
    std::unordered_map<Product *, double> consumer_demands;
    std::unordered_map<Product *, double> producer_demands;
    double recent_labor_hours = 0.0;
    double labor_hours_this_time_step = 0.0;
    std::unordered_map<Product *, std::unordered_set<Order *>> product_to_outbound_orders;
    std::unordered_map<Product *, double> recorded_living_labor_per_unit;
    std::unordered_set<Plan *> plans_in_progress;

    bool send_order(Order * order);
    bool remove_input_from_inventory(
        Product * product,
        double quantity
    );
    bool remove_input_from_inventory(
        Product * product,
        double quantity,
        std::unordered_map<Product *, double>& container
    );
    double get_reorder_threshold(Product * product);
    double get_resupply_deficit(Product * product);
    void update_recent_labor_hours();
    virtual void check_and_reorder_input(Product * product);
    bool start_plan(Plan * plan);
    void handle_start_plan_failure(Plan * plan, Product * product, double missing);
    void return_inputs_to_inventory(
        std::unordered_map<Product *, double> deducted_inputs
        );
    void refund_for_unused_inputs(
            const std::unordered_map<Product *, double> returned_inputs);
    void rollback_plan_inputs(Plan * plan);
    void move_plan_forward_one_step(Plan * plan);
    void end_plan(Plan * plan);
    void move_plans_forward_one_step();
    double calculate_quantity_produced_from_worker_suitability(Plan * plan);

    double get_pending_inventory(Product * product);
    void reorder_stalled_plan_input(Product * product, double deficit);
    int predict_workers_needed(const Order * order);
    std::vector<Person *> get_available_workers(const Order * order);
    void assign_workers(Plan * draft_plan);
    bool is_employed_here(Person * worker);
    void transfer_surplus_workers();
    int get_number_available_workers_to_transfer();
    void adjust_quantity_for_deadline(Plan * plan);
    double predict_turnaround_time(Plan * plan); 
    double predict_labor_hours(Order * order, std::vector<Person*>& workers);
    double calculate_raw_material_cost_for_order(Order * order);
    void initialize_plan_budget(Plan * draft_plan);
    double calculate_machinery_cost_for_plan(Plan * draft_plan);
    void assign_plan_dependent_fields(Plan * draft_plan);
    void add_demand_signal(
        Product * product,
        double quantity,
        Firm * firm
    );
    double get_demand(Product * product);
    Plan * draft_plan_for_order(Order * order); 
    void update_demands();
    void move_worker_off_standby(Person * worker, Plan * plan);

    void log_plans();
    void log_pursued_plan(const Plan * draft_plan);
    void log_ended_plan(const Plan * plan);
    void log_shipment_received(const Product * product, const double quantity);
    void log_inventory_level(const Product * product, const double quantity);
    void log_inventory_reduction(const Product * product, const double quantity);
    void log_initial_employment(const unsigned int worker_id, const unsigned int workplace_id);
    void log_busyness(double firm_busyness);
    void log_employment_transfer(
            const unsigned int worker_id,
            const unsigned int old_workplace_id,
            const unsigned int new_workplace_id
            );
    void log_drafting_failure_inputs(
        const Product * product,
        std::vector<Product *> missing_resources
    );
    void log_drafting_failure_workers(const Product * product);
    void log_reorder_attempt(const Product * product);
    void log_reorder_failure(const Product * product);
    void log_transfer_request();
    void log_product_quantity(
            const char * const label,
            const Product * product,
            const double quantity
            );
    void log_accepted_order(
        const Order * original_order,
        const Order * chosen_return_Order
    );
    void log_plan_stall(Plan *, Product *, double);
    void log_plan_stall_resolved(Plan *);
    void log_start_plan_stalled(Plan * plan, Product * product, double missing);
    void log_start_plan_stall_resolved(Plan * plan);
    void log_demand(Product * Product, double demand);
    void log_resupply_rate(
        const Product * product,
        double resupply_rate,
        double resupply_deficit
    );
    void log_catalog();
    void log_catalog_addition(Product * product);
    void log_account();
};
