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

std::ostream& operator<<(std::ostream& os, const LogPair& kvp) {
    os << "{" << kvp.key << ":" << kvp.value << "}";
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
        const std::string& label
        ) {
    if (client >= Logger::Client::ERROR) {
        throw std::invalid_argument("Logging client does not exist");
    }

    int time_step = Sim::get_current_time_step();

    std::cout << "{\"t\":" << time_step << ","
        << "\"client\":\"" << clients[client] << "\","
        << "\"id\":" << id << ","
        << "\"label\":\"" << label << "\",";
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
    std::cout << "\"" << pair1.key << "\":" << pair1.value << "}" << std::endl;
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
    std::cout << "\"" << pair1.key << "\":" << pair1.value << "," <<
    "\"" << pair2.key << "\":" << pair2.value << "}" << std::endl;
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
    std::cout << "\"" << pair1.key << "\":" << pair1.value << "," <<
    "\"" << pair2.key << "\":" << pair2.value << "," << 
    "\"" << pair3.key << "\":" << pair3.value << "}" << std::endl;
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
    std::cout << "\"" << pair1.key << "\":" << pair1.value << "," <<
    "\"" << pair2.key << "\":" << pair2.value << "," << 
    "\"" << pair3.key << "\":" << pair3.value << "," <<
    "\"" << pair4.key << "\":" << pair4.value << "}" << std::endl;
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
    std::cout << "\"" << pair1.key << "\":" << pair1.value << "," <<
    "\"" << pair2.key << "\":" << pair2.value << "," << 
    "\"" << pair3.key << "\":" << pair3.value << "," <<
    "\"" << pair4.key << "\":" << pair4.value << "," <<
    "\"" << pair5.key << "\":" << pair5.value << "}" << std::endl;
}

