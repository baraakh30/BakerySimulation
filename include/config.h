#ifndef CONFIG_H
#define CONFIG_H

#include "shared.h"

// Configuration structure
typedef struct {
    // Number of different types of items
    int num_bread_categories;
    int num_sandwich_types;
    int num_cake_flavors;
    int num_sweets_flavors;
    int num_sweet_patisseries;
    int num_savory_patisseries;
    
    // Staff counts
    int num_chefs;
    int num_bakers;
    int num_sellers;
    int num_supply_chain;
    
    // Item prices
    double prices[ITEM_COUNT][100];  // [item_type][flavor]
    
    // Supply quantities ranges
    int supply_min[SUPPLY_COUNT];
    int supply_max[SUPPLY_COUNT];
    
    // Simulation thresholds
    int max_complaints;
    int max_frustrated_customers;
    int max_missing_items_requests;
    double profit_threshold;
    int simulation_time_minutes;
    
    // Production and customer behavior parameters
    int chef_production_time_min;
    int chef_production_time_max;
    int baker_time_min;
    int baker_time_max;
    int customer_arrival_min;
    int customer_arrival_max;
    int purchase_quantity_min;
    int purchase_quantity_max;
    int customer_batch_min;
    int customer_batch_max;
    int customer_patience;
    int quality_threshold;
    double complaint_probability;
    double leave_on_complaint_probability;
    double accept_partial_probability;
} BakeryConfig;

int load_config(const char *filename, BakeryConfig *config);
void init_bakery_state(const BakeryConfig *config);

#endif