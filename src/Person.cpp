#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <random>

#include "Constants.h"
#include "ConsumerGood.h"
#include "Distributor.h"
#include "Firm.h"
#include "Logger.h"
#include "Person.h"
#include "Product.h"
#include "Sim.h"
#include "Society.h"

Person::Person(Society * society):
    society{society},
    age(INITIAL_AGE),
    health_status(HEALTHY)
{
    static unsigned int unique_id = 0;
    id = unique_id++;

    std::lognormal_distribution<>
        ability_dist(0.0, Sim::get_person_ability_stddev());
    std::vector<Person::Ability> varied_abilities;
    for (int i = 0; i < Person::NUM_ABILITIES; i++) {
        abilities[(Person::Ability) i] = ability_dist(Sim::get_random_generator());
    }
    log_abilities();
    ranked_distributors = society->get_distributors();
    std::shuffle(
            ranked_distributors.begin(),
            ranked_distributors.end(),
            Sim::get_random_generator()
            );
    for (Product * product : society->get_goods()) {
        inventory[product] = 
            (int) (PERSON_STOCKPILE_DURATION * product->mean_consumption_frequency);
    }
    log_inventory();
    account = society->get_initial_account();
    log_account();
}

unsigned int Person::get_id() {
    return id;
}

std::unordered_map<Person::Ability, double>& Person::get_abilities() {
    return this->abilities;
}

double Person::get_busyness() {
    return busyness;
}

void Person::train(std::unordered_map<Person::Ability, double> target_abilities) {
    // can introduce < 100% effectiveness on training later
    for (auto &pair : target_abilities) {
        abilities[pair.first] = pair.second;
    }
    log_abilities();
}

void Person::register_hours_worked(double hours_worked) {
    log_hours_worked(hours_worked);
    account += hours_worked;
    busyness_this_time_step += hours_worked;
}

bool Person::charge(double cost) {
    if (cost > account) {
        return false;
    }
    account -= cost; 
    return true;
}

Person::HealthStatus Person::get_health_status() {
    return this->health_status;
}

float Person::productivity() {
	switch(health_status) {
		case HEALTHY:
			return 1.0;
		case UNHEALTHY:
			return UNHEALTHY_PRODUCTIVITY;
		default:
			return 1.0;
	}
}

void Person::purchase_good(Product * product, int quantity) {
    int purchased = 0;
    for (Distributor * distributor : ranked_distributors) {
        int available = distributor->try_sell_goods(*product, quantity, this);
        quantity -= available;
        inventory[product] += available;
        purchased += available;
    }
    log_purchase(product, purchased);
    log_account();
}

void Person::consume() {
    for (Product * product : society->get_goods()) {
        to_consume[product] += product->mean_consumption_frequency;
        int consumed = static_cast<int>(to_consume[product]);
        if (consumed) {
            inventory[product] -= consumed;
            log_consumption(product, consumed);
        }
        to_consume[product] -= (int) to_consume[product];
    }
}

// void Person::consume() {
//     int time = Sim::get_current_time_step();
//     for (Product * product : society->get_goods()) {
//         int period = product->mean_consumption_period;
//         if (time % period == 0) {
//             inventory[product] -= 1;
//             log_consumption(product, 1);
//         }
//     }
// }

bool Person::will_shop() {
    double total_deficit = 0.0;
    for (Product * product : society->get_goods()) {
        total_deficit += std::max(0.0, 
            PERSON_STOCKPILE_DURATION - 
            inventory[product] / product->mean_consumption_frequency
        );
    }
    bool should_shop = total_deficit > PERSON_DEFICIT_THRESHOLD;
    if (should_shop) {
        log_shopping();
    }
    return should_shop;
}

void Person::shop() {
    double total_price = 0.0;
    static std::unordered_map<Product *, int> purchase_quantities;
    for (Product * product : society->get_goods()) {
        purchase_quantities[product] = std::max(0, 
            (int) (PERSON_STOCKPILE_DURATION * product->mean_consumption_frequency) - 
            inventory[product]
        );
        total_price += purchase_quantities[product] * 
            society->get_consumer_good(product)->price_per_unit;
    }
    double price_scalar = std::min(account / total_price, 1.0);
    log_shopping_deficit(std::max(0.0, 1.0 - price_scalar)); 
    for (std::pair<Product *, int> p : purchase_quantities) {
        int quantity = (int) (price_scalar * p.second);
        if (quantity > 0) {
            purchase_good(p.first, quantity);
        }
    }
}

bool Person::will_retire() {
    static std::uniform_real_distribution<> dist(0, 1);
    if (age >= GUARANTEED_RETIREMENT_AGE) { return true; }
    return dist(Sim::get_random_generator()) < RANDOM_RETIREMENT_CHANCE;
}

void Person::retire() {
    society->retire_person(this);
}

void Person::update_health_status() {
    bool changed = false;

    double annual_prob = Sim::get_annual_sickness_chance();
    double sickness_rate = -std::log(1.0 - annual_prob);

    double p_sick_hour = 1.0 - std::exp(-sickness_rate / YEAR);
    double p_recover_hour = 1.0 - std::exp(-1.0 / (AVG_DAYS_TO_RECOVERY * DAY));

    std::bernoulli_distribution get_sick(p_sick_hour);
    std::bernoulli_distribution recover(p_recover_hour);

    if (health_status == HEALTHY && get_sick(Sim::get_random_generator())) {
        health_status = UNHEALTHY;
        changed = true;

    } else if (health_status == UNHEALTHY && recover(Sim::get_random_generator())) {
        health_status = HEALTHY;
        changed = true;
    }

    if (changed) {
        log_health_status();
    }
}

void Person::update_busyness() {
    double duration_prop = 1.0 / BUSYNESS_AVERAGING_WINDOW;
    busyness = busyness * (1 - duration_prop) + busyness_this_time_step * duration_prop;
    busyness_this_time_step = 0.0;
}

void Person::on_time_step() {
	++age;
    consume();
	if (will_shop()) { shop(); }
	if (will_retire()) { retire(); }
	update_health_status();
    update_busyness();
}

void Person::set_firm(Firm * workplace) {
    firm = workplace;
    log_placement();
}

Firm * Person::get_firm() {
    return firm;
}

double Person::suitability(std::vector<Ability>& required_abilities) {
    double suitability = 0.0;
    for (Ability ability : required_abilities) {
        suitability += abilities[ability];
    }
    suitability /= required_abilities.size();
    suitability *= productivity();
    return suitability;
}

const char * Person::ability_names[] = { "Ability_1", "Ability_2", "Ability_3" };

const char * Person::health_status_names[] = { "Healthy", "Unhealthy" };

void Person::log_hours_worked(const double hours) {
    Logger::log(Logger::PERSON, id, "hours_worked", LogPair("hours", hours));
}

void Person::log_purchase(const Product * product, const int quantity) {
    Logger::log(
            Logger::PERSON,
            id,
            "purchase",
            LogPair("product_id", product->id),
            LogPair("quantity", quantity)
            );
}

void Person::log_shopping_deficit(const double deficit) {
    Logger::log(Logger::PERSON, id, "shopping_deficit", LogPair("deficit", deficit));
}

void Person::log_shopping() {
    Logger::log(Logger::PERSON, id, "is_shopping", LogPair("account", account));
}

void Person::log_placement() {
    Logger::log(
            Logger::PERSON,
            id,
            "placement",
            LogPair("firm", firm ? firm->get_id() : -1)
            );
}

void Person::log_abilities() {
    for (std::pair<Ability, double> ability : abilities) {
        Logger::log<LogPair, LogPair>(
                Logger::PERSON,
                id,
                "ability",
                LogPair("ability", ability.first),
                LogPair("value", ability.second)
                );
    }
}

void Person::log_inventory() {
    for (std::pair<Product *, int> entry : inventory) {
        Logger::log(
                Logger::PERSON,
                id,
                "inventory",
                LogPair("product_id", entry.first->id),
                LogPair("amount", entry.second)
                );
    }
}

void Person::log_account() {
    Logger::log(Logger::PERSON, id, "account", LogPair("value", account));
}

void Person::log_health_status() {
    Logger::log(
            Logger::PERSON,
            id,
            "health_status",
            LogPairS("status", health_status_names[health_status])
            );
}

void Person::log_consumption(const Product * product, const int quantity) {
    Logger::log(
            Logger::PERSON,
            id,
            "consumption",
            LogPair("product_id", product->id),
            LogPair("quantity", quantity)
            );
}
