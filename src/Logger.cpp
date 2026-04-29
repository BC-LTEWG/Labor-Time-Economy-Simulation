#include <stdexcept>

#include "Constants.h"
#include "Firm.h"
#include "Logger.h"
#include "Person.h"
#include "Product.h"
#include "Sim.h"
#include "sqlite3.h"

Logger * Logger::get_instance() {
    static Logger * instance = new Logger;
    return instance;
}

const char * Logger::clients[] = {"Firm", "Distributor", "Person", "Producer", "Product", "Society"};

void Logger::log(
        const Client client,
        const std::string label,
        const unsigned int id
        ) {
    TupleNone tuple;
    log_impl(client, label, id, tuple);
}

void Logger::log(const Client client,
        const std::string label,
        const unsigned int id,
        const int value
        ) {
    TupleInt tuple = std::make_tuple(value);
    log_impl(client, label, id, tuple);
}

void Logger::log(const Client client,
        const std::string label,
        const unsigned int id,
        const double measure
        ) {
    TupleDouble tuple = std::make_tuple(measure);
    log_impl(client, label, id, tuple);
}

void Logger::log(
        const Client client,
        const std::string label,
        const unsigned int id,
        const std::string value
        ) {
    TupleString tuple = std::make_tuple(value);
    log_impl(client, label, id, tuple);
}

void Logger::log(
        const Client client,
        const std::string label,
        const unsigned int id,
        const std::string name,
        const int quantity
        ) {
    log_impl<int>(client, label, id, name, quantity);
}

void Logger::log(
    const Client client,
    const std::string label,
    const unsigned int id,
    const std::vector<int>& values
) {
    if (!Sim::does_json()) {
        return;
    }
    if (client >= ERROR) {
        throw std::invalid_argument("Logging client does not exist");
    }

    int time_step = Sim::get_current_time_step();

    std::cout << "{\"t\":" << time_step << ","
              << "\"client\":\"" << clients[client] << "\","
              << "\"id\":" << id << ","
              << "\"label\":\"" << label << "\","
              << "\"values\":[";

    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) std::cout << ",";
        std::cout << values[i];
    }

    std::cout << "]}" << std::endl;
}

void Logger::log(
        const Client client,
        const std::string label,
        const unsigned int id,
        const std::string name,
        const double measure
        ) {
    log_impl<double>(client, label, id, name, measure);
}

void Logger::log(
        const Client client,
        const std::string label,
        const unsigned int id,
        const unsigned int index,
        const double value
        ) {
    if (!Sim::does_json()) {
        return;
    }
    if (client >= ERROR) {
        throw std::invalid_argument("Logging client does not exist");
    }
    int time_step = Sim::get_current_time_step();
    std::cout << "{\"t\":" << time_step << "," <<
        "\"client\":\"" << clients[client] << "\"," <<
        "\"id\":" << id << "," <<
        "\"label\":\"" << label << "\"," <<
        "\"index\":" << index << "," <<
        "\"value\":" << value << "}" << std::endl;
}

void Logger::log(
        const Client client,
        const std::string label,
        const unsigned int id,
        const std::pair<int, int> coords,
        const double value
        ) {
    if (!Sim::does_json()) {
        return;
    }
    if (client >= ERROR) {
        throw std::invalid_argument("Logging client does not exist");
    }
    int time_step = Sim::get_current_time_step();
    std::cout << "{\"t\":" << time_step << "," <<
        "\"client\":\"" << clients[client] << "\"," <<
        "\"id\":" << id << "," <<
        "\"label\":\"" << label << "\"," <<
        "\"coords\":[" << coords.first << "," << coords.second << "]," <<
        "\"value\":" << value << "}" << std::endl;
}

void Logger::log_impl(
        const Client client,
        const std::string label,
        const unsigned int id,
        const Tuple& values
        ) {
    if (!Sim::does_json()) {
        return;
    }
    int time_step = Sim::get_current_time_step();
    Logger::json(time_step, client, label, id, values);
}

void Logger::log(
    const Client client,
    const std::string label,
    const unsigned int id,
    const int value1,
    const int value2
) {
    TupleIntInt tuple = std::make_tuple(value1, value2);
    log_impl(client, label, id, tuple);
}

void Logger::log(
    const Client client,
    const std::string label,
    const unsigned int id,
    const double value1,
    const double value2,
    const int value3
) {
    TupleDoubleDoubleInt tuple = std::make_tuple(value1, value2, value3);
    log_impl(client, label, id, tuple);
}

void Logger::log(
    const Client client,
    const std::string label,
    const unsigned int id, // producer id
    const unsigned int id2, // customer id
    const int id3, // product id
    const int value //quantity
) {
    Tuple tuple = std::make_tuple(id2, id3, value);
    log_impl(client, label, id, tuple);
}

template <typename T>
void Logger::log_impl(
        const Client client,
        const std::string label,
        const unsigned int id,
        const std::string& key,
        const T value
        ) {
    if (!Sim::does_json()) {
        return;
    }
    int time_step = Sim::get_current_time_step();
    Logger::json<T>(time_step, client, label, id, key, value);
}

void Logger::json(
        const int time_step,
        const Client client,
        std::string label,
        unsigned int id,
        const Tuple& values
        ) {
    if (client >= ERROR) {
        throw std::invalid_argument("Logging client does not exist");
    }
    std::cout << "{\"t\":" << time_step << "," <<
        "\"client\":\"" << clients[client] << "\"," <<
        "\"id\":" << id << "," <<
        "\"label\":\"" << label << "\"," <<
        "\"values\":[";
    auto visitor = [](auto&& arg) {
        Logger::trace_tuple(arg);
    };
    std::visit(visitor, values);
    std::cout << "]}" << std::endl;
}

template <typename T>
void Logger::json(
        const int time_step,
        const Client client,
        std::string label,
        unsigned int id,
        const std::string key,
        const T value
        ) {
    if (client >= ERROR) {
        throw std::invalid_argument("Logging client does not exist");
    }
    std::cout << "{\"t\":" << time_step << "," <<
        "\"client\":\"" << clients[client] << "\"," <<
        "\"id\":" << id << "," <<
        "\"label\":\"" << label << "\"," <<
        "\"" << key << "\":" << value << "}" << std::endl;
}

template<typename TupleT>
void Logger::trace_tuple(const TupleT& values) {
    static int count = 0;
    std::apply([](auto&& ... arg) {
            ((std::cout << (count++ ? "," : "") << arg), ...); count++;
            }, values);
    count = 0;
}

