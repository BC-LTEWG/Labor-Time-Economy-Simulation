#pragma once

#include <cstdarg>
#include <fstream>
#include <iostream>
#include <map>
#include <tuple>
#include <variant>
#include <unordered_map>

#include "Sim.h"

using TupleNone = std::tuple<>;
using TupleInt = std::tuple<int>;
using TupleIntInt = std::tuple<int, int>;
using TupleDouble = std::tuple<double>;
using TupleDoubleDoubleInt = std::tuple<double, double, int>;
using TupleIntDoubleInt = std::tuple<int, double, int>;
using TupleString = std::tuple<std::string>;
using TupleStringStringInt = std::tuple<std::string, std::string, int>;
using TupleIntIntInt = std::tuple<unsigned int, int, int>;
using Tuple = std::variant<TupleNone,
      TupleInt,
      TupleIntInt,
      TupleDouble,
      TupleDoubleDoubleInt,
      TupleString,
      TupleIntDoubleInt,
      TupleIntIntInt
      >;

struct Product;

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
        void log(
                const Client client,
                const std::string label,
                const unsigned int id
                );
        void log(
                const Client client,
                const std::string label,
                const unsigned int id,
                const int value
                );
        void log(
                const Client client,
                const std::string label,
                const unsigned int id,
                const double measure
                );
        void log(
                const Client client,
                const std::string label,
                const unsigned int id,
                const std::string value
                );
        void log(
                const Client client,
                const std::string label,
                const unsigned int id,
                const std::string name,
                const int quantity
                );
        void log(
                const Client client,
                const std::string label,
                const unsigned int id,
                const std::string name,
                const double measure
                );
        void log(
                const Client client,
                const std::string label,
                const unsigned int id,
                const unsigned int index,
                const double value
                );
        void log(
                const Client client,
                const std::string label,
                const unsigned int id,
                const double value1,
                const double value2,
                const int value3
                );
        void log(
                const Client client,
                const std::string label,
                const unsigned int id,
                const unsigned int id2,
                const int id3,
                const int value
                );
        void log(
            const Client client,
            const std::string label,
            const unsigned int id,
            const int value1,
            const int value2
                );
        void log(
            const Client client,
            const std::string label,
            const unsigned int id,
            const std::vector<int>& values
        );
        void log(
                const Client client,
                const std::string label,
                const unsigned int id,
                const std::pair<int, int> coords,
                const double value
                );
        template <typename T>
            void log(
                    const Client client,
                    const std::string label1,
                    const unsigned int id1,
                    const std::string label2,
                    const unsigned int id2,
                    const std::string label3,
                    const T value
                    ) {
                if (!Sim::does_json()) {
                    return;
                }
                if (client >= ERROR) {
                    throw std::invalid_argument("Logging client does not exist");
                }
                int time_step = Sim::get_current_time_step();
                std::cout <<
                    "{\"t\":" << time_step << "," <<
                    "\"client\":\"" << clients[client] << "\"," <<
                    "\"id\":" << id1 << "," <<
                    "\"label\":\"" << label1 << "\"," <<
                    "\"" << label2 << "\":" << id2 << "," <<
                    "\"" << label3 << "\":" << value << "}" << std::endl;
            }
    private:
        void log_impl(
                const Client client,
                const std::string label,
                const unsigned int id,
                const Tuple& values
                );
        template <typename T>
        void log_impl(
                const Client client,
                const std::string label,
                const unsigned int id,
                const std::string& key,
                const T value
                );
        static void json(
                const int time_step,
                const Client client,
                std::string label,
                unsigned int id,
                const Tuple& values
                );
        template <typename T>
        static void json(
                const int time_step,
                const Client client,
                std::string label,
                unsigned int id,
                const std::string key,
                const T value
                );
        template<typename TupleT>
            static void trace_tuple(const TupleT& values);
        static const char * clients[];
};


