#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#define private public
#define protected public
#include "Society.h" 
#include "Person.h"
#undef protected
#undef private

#include "doctest.h"
#include "Distributor.h"
#include "ConsumerGood.h"
#include "Good.h"
#include "Logger.h"
#include <vector>

TEST_CASE("Distributor Logic Testing") {
    std::vector<Distributor*> distributors = Society::get_instance()->get_distributors(); //find distributor/consumer_good
    std::vector<ConsumerGood*> consumer_goods = Society::get_instance()->get_consumer_goods();
    
    REQUIRE(distributors.size() > 0);
    REQUIRE(consumer_goods.size() > 0);
    
    Distributor* test_distributor = distributors[0];
    ConsumerGood* test_consumerGood = consumer_goods[0];

    SUBCASE("get_client_type() identifies distributor") {
        CHECK(test_distributor->get_client_type() == Logger::DISTRIBUTOR);
    }

    SUBCASE("add_to_catalog() calculates inventory") {
        Good* base_good = test_consumerGood->corresponding_good;
        CHECK(test_distributor->get_input_inventory_level(test_consumerGood) > 0); //inventory should be > 0
        CHECK(test_distributor->get_input_inventory_level(base_good) > 0); //automatically called (no need to do here)
    }

    SUBCASE("try_sell_goods() deducts inventory on purchase") {
        std::vector<Person*> people = Society::get_instance()->people; //find person from sim
        REQUIRE(people.size() > 0);
        Person* buyer = people[0];
        double initial_inventory = test_distributor->get_input_inventory_level(test_consumerGood); 
        int quantity_to_buy = 1;

        bool original_sector_status = test_consumerGood->public_sector; //good defined as public sector item (avoids $0 edgecase)
        test_consumerGood->public_sector = true; 

        int amount_sold = test_distributor->try_sell_goods(test_consumerGood, quantity_to_buy, buyer); //sale made
        if (initial_inventory >= 1.0) {
            CHECK(amount_sold == 1); 
            CHECK(test_distributor->get_input_inventory_level(test_consumerGood) == doctest::Approx(initial_inventory - 1.0)); //check if reduced by 1
        } else {
            CHECK(amount_sold == 0);
        }
        test_consumerGood->public_sector = original_sector_status; //good back to original
    }
}
