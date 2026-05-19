#include <stdexcept>

#include "Constants.h"
#include "Firm.h"
#include "Logger.h"
#include "Person.h"
#include "Product.h"
#include "Sim.h"
#include "sqlite3.h"

LogPair::LogPair(std::string key, double value) :
    key{key}, value{value}
{}

LogPairS::LogPairS(std::string key, std::string value) :
    key{key}, value{value}
{}

std::ostream& operator<<(std::ostream& os, const LogPair& lp) {
    os << "{\"" << lp.key << "\":" << lp.value << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const LogPairS& lp) {
    os << "{\"" << lp.key << "\":\"" << lp.value << "\"}";
    return os;
}

static const char * clients[] = {
    "Firm",
    "Distributor",
    "Person",
    "Producer",
    "Product",
    "Society"
};

void log_base(
        const Logger::Client client,
        const unsigned int id,
        const std::string label
        ) {
    if (client >= Logger::Client::ERROR) {
        throw std::invalid_argument("Logging client does not exist");
    }

    int time_step = Sim::get_current_time_step();

    std::cout << "{\"t\":" << time_step << ","
        << "\"client\":\"" << clients[client] << "\","
        << "\"id\":" << id << ","
        << "\"label\":\"" << label << "\"";
}

void Logger::log(
        const Logger::Client client,
        const unsigned int id,
        const std::string label
        ) {
    if (!Sim::does_json()) {
        return;
    }
    log_base(client, id, label);
    std::cout << "}" << std::endl;
}

void Logger::log(
        const Client client,
        const unsigned int id,
        const std::string label,
        const LogPair pair1
        ) {
    if (!Sim::does_json()) {
        return;
    }
    log_base(client, id, label);
    std::cout << "," << pair1 << std::endl;
}

void Logger::log(
        const Client client,
        const unsigned int id,
        const std::string label,
        const LogPairS pair1
        ) {
    if (!Sim::does_json()) {
        return;
    }
    log_base(client, id, label);
    std::cout << "," << pair1 << std::endl;
}

void Logger::log(
        const Client client,
        const unsigned int id,
        const std::string label,
        const LogPair pair1,
        const LogPair pair2
        ) {
    if (!Sim::does_json()) {
        return;
    }
    log_base(client, id, label);
    std::cout << "," << pair1 << "," << pair2 << std::endl;
}

void Logger::log(
        const Client client,
        const unsigned int id,
        const std::string label,
        const LogPair pair1,
        const LogPair pair2,
        const LogPair pair3
        ) {
    if (!Sim::does_json()) {
        return;
    }
    log_base(client, id, label);
    std::cout << "," << pair1 << "," << pair2 << "," << pair3 << std::endl;
}

void Logger::log(
        const Client client,
        const unsigned int id,
        const std::string label,
        const LogPair pair1,
        const LogPair pair2,
        const LogPair pair3,
        const LogPair pair4
        ) {
    if (!Sim::does_json()) {
        return;
    }
    log_base(client, id, label);
    std::cout << "," << pair1 << "," << pair2 <<
        "," << pair3 << "," << pair4 << std::endl;
}

void Logger::log(
        const Client client,
        const unsigned int id,
        const std::string label,
        const LogPair pair1,
        const LogPair pair2,
        const LogPair pair3,
        const LogPair pair4,
        const LogPair pair5
        ) {
    if (!Sim::does_json()) {
        return;
    }
    log_base(client, id, label);
    std::cout << "," << pair1 << "," << pair2 <<
        "," << pair3 << "," << pair4 << "," << pair5 << std::endl;
}

