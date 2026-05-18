#pragma once

#include <cstdarg>
#include <fstream>
#include <iostream>
#include <map>
#include <tuple>
#include <variant>
#include <unordered_map>

#include "Sim.h"

struct Product;

struct LogPair {
    std::string key;
    double value;
    LogPair(std::string, double);
    friend std::ostream& operator<<(std::ostream& os, const LogPair& kvp);
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
        static Logger * get_instance();
        static void log(
                const Client client,
                const unsigned int id,
                const std::string label
                );
        static void log(
                const Client client,
                const unsigned int id,
                const std::string label,
                const LogPair pair1
                );
        static void log(
                const Client client,
                const unsigned int id,
                const std::string label,
                const LogPair pair1,
                const LogPair pair2
                );
        static void log(
                const Client client,
                const unsigned int id,
                const std::string label,
                const LogPair pair1,
                const LogPair pair2,
                const LogPair pair3
                );
        static void log(
                const Client client,
                const unsigned int id,
                const std::string label,
                const LogPair pair1,
                const LogPair pair2,
                const LogPair pair3,
                const LogPair pair4
                );
        static void log(
                const Client client,
                const unsigned int id,
                const std::string label,
                const LogPair pair1,
                const LogPair pair2,
                const LogPair pair3,
                const LogPair pair4,
                const LogPair pair5
                );
};

