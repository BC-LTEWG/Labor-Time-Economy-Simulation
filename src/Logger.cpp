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
    os << ",\"" << lp.key << "\":" << lp.value;
    return os;
}

std::ostream& operator<<(std::ostream& os, const LogPairS& lp) {
    os << ",\"" << lp.key << "\":\"" << lp.value << "\"";
    return os;
}

