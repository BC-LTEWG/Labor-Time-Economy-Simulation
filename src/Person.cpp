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
#include "PriceController.h"
#include "Product.h"
#include "Sim.h"
#include "Society.h"

Ability::Ability() {
    static unsigned int unique_id = 0;
    id = unique_id++;
}

Person::Person():
    age(INITIAL_AGE),
    health_status(HEALTHY)
{
    static unsigned int unique_id = 0;
    id = unique_id++;

    double ability_std_dev = Sim::get_ability_stddev();
    std::lognormal_distribution<>
        ability_dist(-0.5 * ability_std_dev * ability_std_dev, ability_std_dev);
    for (Ability * ability : Society::get_instance()->get_abilities()) {
        abilities[ability] = ability_dist(Sim::get_random_generator());
    }
    log_abilities();
    ranked_distributors = Society::get_instance()->get_distributors();
    std::shuffle(
            ranked_distributors.begin(),
            ranked_distributors.end(),
            Sim::get_random_generator()
            );
    for (ConsumerGood * consumer_good : Society::get_instance()->get_consumer_goods()) {
        inventory[consumer_good] = 
            static_cast<int>(PERSON_STOCKPILE_DURATION * consumer_good->mean_consumption_frequency);
    }
    log_inventory();
    account = Society::get_instance()->get_initial_account();
    log_account();
}

unsigned int Person::get_id() {
    return id;
}

bool Person::get_is_retired() {
    return is_retired;
}

std::unordered_map<Ability *, double>& Person::get_abilities() {
    return abilities;
}

double Person::get_busyness() {
    return busyness;
}

double Person::get_account() {
    return account;
}

void Person::train(std::unordered_map<Ability *, double>& target_abilities) {
    // can introduce < 100% effectiveness on training later
    for (const std::pair<Ability * const, double>& ability : target_abilities) {
        abilities[ability.first] = ability.second;
    }
    log_abilities();
}

void Person::register_hours_worked(double hours_worked) {
    log_hours_worked(hours_worked);
    busyness_this_time_step += hours_worked;
    double individual_portion = PriceController::get_instance()->get_fic() * hours_worked;
    double public_portion = hours_worked - individual_portion;
    account += individual_portion;
    Society::get_instance()->pay_into_public_fund(public_portion); 
    log_account();
}

bool Person::charge(double cost) {
    if(is_retired) {
        Society::get_instance()->charge_from_public_fund(cost);
    }
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

void Person::purchase_good(ConsumerGood * consumer_good, int quantity) {
    int purchased = 0;
    for (Distributor * distributor : ranked_distributors) {
        int available = distributor->try_sell_goods(consumer_good, quantity, this);
        quantity -= available;
        inventory[consumer_good] += available;
        purchased += available;
    }
    log_purchase(consumer_good, purchased);
    log_shopping_deficit(1 - static_cast<double>(purchased) / (purchased + quantity));
    log_account();
}

void Person::consume() {
    for (ConsumerGood * consumer_good : Society::get_instance()->get_consumer_goods()) {
        double consumption = consumer_good->mean_consumption_frequency;
        to_consume[consumer_good] += consumption;
        int consumed = static_cast<int>(to_consume[consumer_good]);
        if (consumed) {
            inventory[consumer_good] -= consumed;
            log_consumption(consumer_good, consumed);
        }
        to_consume[consumer_good] -= (int) to_consume[consumer_good];
    }
}

bool Person::will_shop() {
    if (busyness_this_time_step) {
        return false;
    }
    double total_deficit = 0.0;
    for (ConsumerGood * consumer_good : Society::get_instance()->get_consumer_goods()) {
        total_deficit += std::max(0.0, 
            PERSON_STOCKPILE_DURATION - 
            inventory[consumer_good] / consumer_good->mean_consumption_frequency
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
    static std::unordered_map<ConsumerGood *, int> purchase_quantities;
    for (ConsumerGood * consumer_good : Society::get_instance()->get_consumer_goods()) {
        purchase_quantities[consumer_good] = std::max(0, 
            (int) (PERSON_STOCKPILE_DURATION * consumer_good->mean_consumption_frequency) - 
            inventory[consumer_good]
        );
        if (consumer_good->public_sector) {
            continue;
        }
        total_price += purchase_quantities[consumer_good] * 
            consumer_good->price_per_unit;
    }
    double price_scalar = std::min(account / total_price, 1.0);
    for (std::pair<ConsumerGood *, int> consumer_good : purchase_quantities) {
        int quantity = consumer_good.second;
        if (!consumer_good.first->public_sector) {
            quantity = (int) (price_scalar * consumer_good.second);
        }
        if (quantity > 0) {
            purchase_good(consumer_good.first, quantity);
        }
    }
}

bool Person::will_retire() {
    static std::uniform_real_distribution<> dist(0, 1);
    if (age >= GUARANTEED_RETIREMENT_AGE) { return true; }
    return dist(Sim::get_random_generator()) < RANDOM_RETIREMENT_CHANCE;
}

void Person::retire() {
    is_retired = true;
    if(firm) {
        set_firm(nullptr);
        set_plan(nullptr);
    }

    Society::get_instance()->retire_person(this);
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
    double growth = busyness_this_time_step;
    double decay = busyness;

    double work_week_proportion;
    if (plan) {
        work_week_proportion =
            Firm::get_work_week_proportion(plan);
    } else {
        work_week_proportion =
            Society::get_instance()->get_work_week_proportion();
    }

    decay /= work_week_proportion;

    busyness += 
        (growth - decay) 
        / BUSYNESS_AVERAGING_WINDOW;

    busyness_this_time_step = 0.0;
}

void Person::on_time_step() {
	++age;
    consume();
    // this is not a good idea until we implement replacement population growth
	if (!is_retired && will_retire()) {
        retire();
    }
	update_health_status();

	if (will_shop()) {
        shop();
    }
    if (
        (plan && Firm::is_within_work_schedule(plan))
        || (!plan && Society::get_instance()->is_within_work_schedule())
    ) {
        update_busyness();
    }
}

void Person::set_firm(Firm * workplace) {
    firm = workplace;
    log_placement();
}

void Person::set_plan(Plan * new_plan) {
    plan = new_plan;
}

Firm * Person::get_firm() {
    return firm;
}

double Person::suitability(std::vector<Ability *>& required_abilities) {
    double suitability = 0.0;
    for (Ability * ability : required_abilities) {
        suitability += abilities[ability];

    }
    suitability /= required_abilities.size();
    suitability *= productivity();
    return suitability;
}

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
    for (std::pair<Ability *, double> ability : abilities) {
        Logger::log(
                Logger::PERSON,
                id,
                "ability",
                LogPair("ability", ability.first->id),
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
