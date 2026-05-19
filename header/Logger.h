#pragma once

#include <cstdarg>
#include <fstream>
#include <iostream>
#include <map>
#include <tuple>
#include <variant>
#include <unordered_map>

#include "Sim.h"

struct LogPair {
    std::string key;
    double value;
    LogPair(std::string, double);
    friend std::ostream& operator<<(std::ostream& os, const LogPair& lp);
};

struct LogPairS {
    std::string key;
    std::string value;
    LogPairS(std::string, std::string);
    friend std::ostream& operator<<(std::ostream& os, const LogPairS& lp);
};

class Logger {
    public:
        enum Client {
            FIRM,
            DISTRIBUTOR,
            PERSON,
            PRODUCER,
            PRODUCT,
            SOCIETY,
            ERROR
        };

        template <typename... Pairs>
        static void log(
                const Client client,
                const unsigned int id,
                const std::string label,
                Pairs... pairs
                );
};

template <typename... Pairs>
void Logger::log(
        const Logger::Client client,
        const unsigned int id,
        const std::string label,
        Pairs... pairs
        ) {
    if (!Sim::does_json()) {
        return;
    }
    if (client >= Logger::Client::ERROR) {
        throw std::invalid_argument("Logging client does not exist");
    }
    const char * clients[] = {
        "Firm",
        "Distributor",
        "Person",
        "Producer",
        "Product",
        "Society"
    };
    int time_step = Sim::get_current_time_step();
    std::cout << "{\"t\":" << time_step << ","
        << "\"client\":\"" << clients[client] << "\","
        << "\"id\":" << id << ","
        << "\"label\":\"" << label << "\"";
    (std::cout << ... << pairs) << "}" << std::endl;
}


