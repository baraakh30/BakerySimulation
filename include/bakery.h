#ifndef BAKERY_H
#define BAKERY_H

#include "shared.h"
#include "config.h"

// Bakery management function prototypes
void stop_simulation(void);
void check_simulation_end_conditions(const BakeryConfig *config);
void reassign_chefs(TeamType from_team, TeamType to_team, int num_chefs);
int check_item_availability(ItemType item_type, int flavor);
double calculate_profit(ItemType item_type, int flavor);
int can_produce_item(TeamType team);
void adjust_production_priorities(void);
void print_bakery_status(void);

#endif