#include <algorithm>
#include <cmath>
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>

#include "Constants.h"
#include "ConsumerGood.h"
#include "Distributor.h"
#include "Firm.h"
#include "Logger.h"
#include "Machine.h"
#include "Person.h"
#include "Product.h"
#include "Producer.h"
#include "Sim.h"
#include "Society.h"

Society * Society::get_instance() {
    static Society *instance = new Society;
    return instance;
}

Society::Society() :
    current_work_hours_daily{Sim::get_work_hours_daily()},
    current_work_days_weekly{Sim::get_work_days_weekly()}
{
    static unsigned int unique_id = 0;
    id = unique_id++;
    set_initial_products();
    for (Product * product : products) {
        Logger::get_instance()->log(Logger::SOCIETY, "price", product->id, product->price_per_unit);
        Logger::get_instance()->log(Logger::SOCIETY, "order_size", product->id, product->order_size);
    }
    for (unsigned int i = 0; i < Sim::get_num_producers(); i++) {
        Producer * producer = new Producer(this, {goods[i % Sim::get_num_products()]});
        producers.push_back(producer);
        firms.push_back(producer);
    }
    std::unordered_set<Product *> distributor_catalog(goods.begin(), goods.end());
    for (unsigned int i = 0; i < Sim::get_num_distributors(); i++) {
        Distributor * distributor = new Distributor(this, distributor_catalog);
        distributors.push_back(distributor);
        firms.push_back(distributor);
    }
    for (Firm * firm : firms) {
        for (Producer * producer : producers) {
            firm->add_supplier(producer);
        }
    }
    set_initial_account();
    for (unsigned int i = 0; i < Sim::get_num_people(); i++) {
        birth_person();
    }
}

unsigned int Society::get_id() {
    if (id) {
        throw std::invalid_argument("Society should be a singleton.");
    }
    return id;
}

void Society::on_time_step() {
    for (Person * person : people) {
        person->on_time_step();
    }
    for (Firm * firm : firms) {
        firm->on_time_step();
    }
    if (Sim::get_current_time_step() >= WORK_HOURS_UPDATE_START &&
            Sim::get_current_time_step() % WORK_HOURS_UPDATE_PERIOD == 0) {
        update_work_hours_daily();
    }
}

void Society::set_initial_products() {
    unsigned int starting_num_products = Sim::get_num_products();
    std::size_t i = 0;
    for (; i < starting_num_products; ++i) {
        Product *new_product = new Product(
            i,
            "Product_" + std::to_string(i));
        goods.push_back(new_product);
        products.push_back(new_product);
        product_to_index[new_product] = i;
    }
    static std::uniform_int_distribution<>
        machine_lifetime_dist(MACHINE_LIFETIME_MIN, MACHINE_LIFETIME_MAX);
    const unsigned int starting_num_machines =
        starting_num_products / Sim::get_products_per_machine();
    for (std::size_t j = 0; j < starting_num_machines; ++j, ++i) {
        Machine *new_machine = new Machine(
            i,
            "Machine_" + std::to_string(i),
            machine_lifetime_dist(Sim::get_random_generator()));
        machines.push_back(new_machine);
        products.push_back(new_machine);
        product_to_index[new_machine] = i;
    }
    for (Product * product : products) {
        product->set_inputs(goods);
        product->set_machines(machines);
    }
    set_product_prices_production_consumption();
    log_consumption_frequencies();
    log_consumption_periods();
}

void Society::populate_io_matrix_and_labor_vector(
    std::unordered_map<Product *, std::size_t> &product_to_index,
    Eigen::MatrixXd &input_output_matrix,
    Eigen::VectorXd &labor_vector) {
    const unsigned int starting_num_firms =
        Sim::get_num_producers() + Sim::get_num_distributors();
    const unsigned int average_team_size =
        std::max<unsigned int>(Sim::get_num_people() / starting_num_firms, 1);
    for (Product * output_product : products) {
        for (const std::pair<Product * const, double> &input :
             output_product->inputs_per_unit) {
            input_output_matrix(
                product_to_index[input.first],
                product_to_index[output_product]) = input.second;
        }
        double machine_use_hours =
            output_product->living_labor_per_unit / average_team_size;
        for (Machine * const machine : output_product->machines_needed) {
            input_output_matrix(
                product_to_index[static_cast<Product * const>(machine)],
                product_to_index[output_product]) = machine_use_hours / machine->lifetime;
        }
        labor_vector(product_to_index[output_product]) =
            output_product->living_labor_per_unit;
    }
}

double get_max_eigenvalue(Eigen::MatrixXd &io_matrix) {
    Eigen::EigenSolver<Eigen::MatrixXd> eigen_solver(io_matrix, false);
    Eigen::VectorXcd eigenvalues = eigen_solver.eigenvalues();
    double max_eigenvalue = 0.0;
    for (size_t i = 0; i < static_cast<unsigned long>(eigenvalues.size()); ++i) {
        if (eigenvalues(i).real() > max_eigenvalue &&
            !eigenvalues(i).imag()) {
            max_eigenvalue = eigenvalues(i).real();
        }
    }
    return max_eigenvalue;
}

std::vector<Producer *> &Society::get_producers() {
    return producers;
}

double Society::get_busyness() {
    double busyness = 0.0;
    for (Person * person : people) {
        busyness += person->get_busyness();
    }
    return busyness / people.size();
}

double Society::get_total_employment() {
    unsigned int employed = 0;
    for (Person * person : people) {
        employed += (person->get_firm() != nullptr);
    }
    return static_cast<double>(employed) / people.size();
}

void Society::log_total_employment() {
    Logger::get_instance()->log(Logger::SOCIETY, "employment", id, get_total_employment());
}

void Society::adjust_io_matrix(
    Eigen::MatrixXd& io_matrix,
    double max_eigenvalue) {
    io_matrix /= (max_eigenvalue + PRODUCT_INPUT_EPSILON);
    const size_t dim = io_matrix.rows();
    for (std::size_t j = 0; j < dim; ++j) {
        for (std::size_t i = 0; i < dim; ++i) {
            if (io_matrix(i, j) &&
                    products[i]->product_type == Product::ProductType::TYPE_GOOD) {
                products[j]->inputs_per_unit[products[i]] = io_matrix(i, j);
            }
        }
    }
}

Eigen::MatrixXd get_leontief_inverse(
    Eigen::MatrixXd io_matrix) {
    const std::size_t dim = io_matrix.rows();
    Eigen::MatrixXd identity_matrix = Eigen::MatrixXd::Identity(dim, dim);
    Eigen::MatrixXd leontief_matrix = identity_matrix - io_matrix;
    return leontief_matrix.inverse();
}

void Society::set_product_prices_production_consumption() {
    const size_t dim = products.size();
    Eigen::MatrixXd A(dim, dim);
    Eigen::VectorXd l(dim);
    populate_io_matrix_and_labor_vector(product_to_index, A, l);
    double max_eigenvalue = get_max_eigenvalue(A);
    if (max_eigenvalue >= 1.0) {
        adjust_io_matrix(A, max_eigenvalue);
    }
    log_io_matrix(A, dim);
    log_labor_vector(l, dim);
    Eigen::MatrixXd leontief_inverse = get_leontief_inverse(A);
    Eigen::VectorXd values = leontief_inverse.transpose() * l;
    for (std::size_t i = 0; i < dim; ++i) {
        if (values(i) <= 0.0) {
            std::stringstream message;
            message << "Value of item " << i << " <= 0.";
            throw std::domain_error(message.str());
        }
        products[i]->price_per_unit = values(i);
    }
    double consumption_scalar = 0.0;
    for (Product * product : products) {
        consumption_scalar += product->price_per_unit * product->mean_consumption_frequency;
    }
    const unsigned int initial_work_week =
        Sim::get_work_hours_daily() * Sim::get_work_days_weekly();
    consumption_scalar = PRODUCT_CONSUMPTION_MULT * initial_work_week / WEEK / consumption_scalar;
    for (Product *product : products) {
        product->mean_consumption_frequency *= consumption_scalar;
        std::cerr << "mean consumption frequency for product " << product << " = " << product->mean_consumption_frequency << std::endl;
    }
    for (Product *product : products) {
        product->mean_consumption_period = static_cast<int>(std::ceil(1 / product->mean_consumption_frequency));
    }
    Eigen::VectorXd demands(dim);
    for (Product *product : products) {
        demands[product_to_index[product]] = product->mean_consumption_frequency;
    }
    Eigen::VectorXd production = leontief_inverse * demands;
    for (std::size_t i = 0; i < dim; ++i) {
        initial_production[products[i]] = production(i);
    }
}

std::vector<Product *> &Society::get_products() {
    return products;
}

std::vector<Product *> &Society::get_goods() {
    return goods;
}

ConsumerGood * Society::get_consumer_good(Product *product) {
    if (consumer_goods.count(product)) {
        return consumer_goods[product];
    }
    else {
        return NULL;
    }
}

void Society::add_consumer_good(Product *product) {
    if (!consumer_goods.count(product)) {
        consumer_goods[product] = new ConsumerGood(product);
    }
}

std::vector<Distributor *> &Society::get_distributors() {
    return distributors;
}

std::unordered_set<Person *>& Society::get_unemployed_people() {
    return unemployed_people;
}

unsigned int Society::get_current_work_hours_daily() {
    return current_work_hours_daily;
}

unsigned int Society::get_current_work_days_weekly() {
	return current_work_days_weekly;
}

void Society::set_initial_account() {
    initial_account = 0.0;
    for (Product * good : goods) {
        ConsumerGood * consumer_good = get_consumer_good(good);
        if (!consumer_good) {
            std::cerr << "consumer good does not exist: "
                      << good->product_name << std::endl;
            exit(EXIT_FAILURE);
        }
        initial_account += consumer_good->price_per_unit *
                           consumer_good->mean_consumption_frequency;
    }
    initial_account *= INITIAL_ACCOUNT_DURATION;
}

int Society::get_initial_account() {
    return initial_account;
}

std::unordered_map<Product *, double> &Society::get_initial_production() {
    return initial_production;
}

void Society::update_work_hours_daily() {
    current_work_hours_daily = std::ceil(get_busyness() * INEFFICIENCY_OF_WORK * 
            WEEK / Sim::get_work_days_weekly());
    current_work_hours_daily = std::min(DAY, current_work_hours_daily);
}

Person * Society::birth_person() {
    Person * person = new Person(this);
    people.push_back(person);
    unemployed_people.insert(person);
    return person;
}

void Society::retire_person(Person *person) {
    // unimplemented until hiring/reallocation is done
}

void Society::log_io_matrix(Eigen::MatrixXd& A, size_t dim) {
    Logger::get_instance()->log(Logger::SOCIETY, "A_dim", id, static_cast<int>(dim));
    for (size_t i = 0; i < dim; ++i) {
        for (size_t j = 0; j < dim; ++j) {
            if (A(i, j)) {
                Logger::get_instance()->log(
                        Logger::SOCIETY,
                        "A",
                        id,
                        std::make_pair(i, j),
                        A(i, j)
                        );
            }
        }
    }
}

void Society::log_labor_vector(Eigen::VectorXd& l, size_t dim) {
    Logger::get_instance()->log(Logger::SOCIETY, "l_dim", id, static_cast<int>(dim));
    for (size_t i = 0; i < dim; ++i) {
        if (l(i)) {
            Logger::get_instance()->log(Logger::SOCIETY, "l", id, i, l(i));
        }
    }
}

void Society::log_consumption_frequencies() {
    Logger * logger = Logger::get_instance();
    for (const Product * product : goods) {
        logger->log(
                Logger::SOCIETY,
                "mean_consumption_frequency",
                id,
                product->product_name,
                product->mean_consumption_frequency
                );
    }
}

void Society::log_consumption_periods() {
    Logger * logger = Logger::get_instance();
    for (const Product * product : goods) {
        logger->log(
                Logger::SOCIETY,
                "mean_consumption_period",
                id,
                product->product_name,
                product->mean_consumption_period
                );
    }
}
