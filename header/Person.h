#pragma once

#include <unordered_map>
#include <vector>
#include "Agent.h"

struct Plan;
struct Product;
class Distributor;
class Firm;
class Society;

class Person : public Agent {
  public:
    enum HealthStatus { HEALTHY, UNHEALTHY };
	
    Person(Society * society);
    unsigned int get_id() override;
	void on_time_step() override;
  
	std::unordered_map<int, double>& get_abilities();
    double get_busyness();
	void train(std::unordered_map<int, double>& target_abilities);
    HealthStatus get_health_status();
    float productivity();
    double suitability(std::vector<int>& required_abilities);
    void register_hours_worked(double hours_worked);
    bool charge(double cost);
    void purchase_good(Product * p, int quantity);
    void set_firm(Firm *);
    Firm * get_firm();
  
  private:
    Society * society;
    unsigned int id;
    std::unordered_map<int, double> abilities;
    int age;
    HealthStatus health_status;
    std::unordered_map<Product *, int> inventory;
    std::unordered_map<Product *, double> to_consume;
    Firm * firm = nullptr;
    double account;
    double busyness_this_time_step = 0.0;
    double busyness = 0.0;
 	std::vector<Distributor *> ranked_distributors;
    static const char * health_status_names[];

    void consume();
	bool will_shop();
	void shop();
    bool will_retire();
	void retire();
	void update_health_status();
    void update_busyness();
    void log_hours(const double hours);
    void log_purchase(const std::string& product_name, int quantity);
    void log_shopping();
    void log_shopping_deficit(double deficit);
    void log_placement();
    void log_abilities();
    void log_inventory();
    void log_account();
    void log_health_status();
    void log_consumption(const Product * product, const int quantity);
};
