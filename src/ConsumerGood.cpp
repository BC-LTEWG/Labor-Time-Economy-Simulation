#include "Constants.h"
#include "ConsumerGood.h"

ConsumerGood::ConsumerGood(Product * product) :
    Product(product->id, product->product_name)
{
    product_type = Product::ProductType::TYPE_CONSUMER_GOOD;
    order_size = 1;
    living_labor_per_unit = DISTRIBUTION_LABOR_PER_UNIT;
    price_per_unit = product->price_per_unit + DISTRIBUTION_LABOR_PER_UNIT;
    mean_consumption_frequency = product->mean_consumption_frequency;
}

