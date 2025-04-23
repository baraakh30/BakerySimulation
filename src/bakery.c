#include "../include/bakery.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>


// Stop the simulation and signal all processes to terminate
void stop_simulation(void) {
    sem_lock(0); // Lock shared memory access
    bakery_state->is_running = 0;
    log_message("Simulation stopped");
    sem_unlock(0);
}

// Check if any end condition has been met
void check_simulation_end_conditions(const BakeryConfig *config) {
    sem_lock(0);
    int should_stop = 0;
    char reason[100] = "";
    
    // Check complaints threshold
    if (bakery_state->customer_complaints >= bakery_state->max_complaints) {
        should_stop = 1;
        strcpy(reason, "too many customer complaints");
    }
    
    // Check frustrated customers threshold
    else if (bakery_state->frustrated_customers >= bakery_state->max_frustrated_customers) {
        should_stop = 1;
        strcpy(reason, "too many frustrated customers");
    }
    
    // Check missing items requests threshold
    else if (bakery_state->missing_items_requests >= bakery_state->max_missing_items_requests) {
        should_stop = 1;
        strcpy(reason, "too many missing items requests");
    }
    
    // Check profit threshold
    else if (bakery_state->daily_profit >= bakery_state->profit_threshold) {
        should_stop = 1;
        strcpy(reason, "profit threshold reached");
    }
    
    // Check simulation time
    time_t current_time = time(NULL);
    int elapsed_minutes = (current_time - bakery_state->start_time) / 60;
    if (elapsed_minutes >= bakery_state->simulation_time_minutes) {
        should_stop = 1;
        strcpy(reason, "simulation time exceeded");
    }
    
    if (should_stop) {
        log_message("Simulation ending: %s", reason);
        bakery_state->is_running = 0;
    }
    
    sem_unlock(0);
}

// Reassign chefs from one team to another
void reassign_chefs(TeamType from_team, TeamType to_team, int num_chefs) {
    sem_lock(0);
    
    if (bakery_state->chefs_per_team[from_team] >= num_chefs) {
        bakery_state->chefs_per_team[from_team] -= num_chefs;
        bakery_state->chefs_per_team[to_team] += num_chefs;
        
        // Send message to notify about reassignment
        Message msg;
        msg.mtype = 1;
        msg.msg_type = MSG_CHEF_REASSIGNMENT;
        msg.sender_pid = getpid();
        msg.data.reassignment.from_team = from_team;
        msg.data.reassignment.to_team = to_team;
        msg.data.reassignment.num_chefs = num_chefs;
        send_message(&msg);
        
        log_message("Reassigned %d chefs from team %d to team %d", num_chefs, from_team, to_team);
    } else {
        log_message("Cannot reassign %d chefs from team %d (only %d available)", 
            num_chefs, from_team, bakery_state->chefs_per_team[from_team]);
    }
    
    sem_unlock(0);
}



// Check if an item is available
int check_item_availability(ItemType item_type, int flavor) {
    sem_lock(0);
    int available = bakery_state->inventory[item_type][flavor] > 0;
    sem_unlock(0);
    return available;
}

// Calculate profit from selling an item
double calculate_profit(ItemType item_type, int flavor) {
    // This would normally involve subtracting production costs from the selling price
    // For simplicity, we'll just return the price as profit
    return bakery_state->inventory[item_type][flavor];
}

// Check if a team can produce its items based on ingredient availability
// Fix the can_produce_item function in bakery.c to remove the undefined SUPPLY_WATER
int can_produce_item(TeamType team) {
    sem_lock(0);
    int can_produce = 1;
    
    // Check required supplies for each team
    switch (team) {
        case TEAM_PASTE:
            can_produce = (bakery_state->supplies[SUPPLY_WHEAT] > 0 && 
                          bakery_state->supplies[SUPPLY_YEAST] > 0 &&
                          bakery_state->supplies[SUPPLY_BUTTER] > 0 &&
                          bakery_state->supplies[SUPPLY_MILK] > 0);
            break;
        case TEAM_BREAD:
            can_produce = (bakery_state->supplies[SUPPLY_WHEAT] > 0 && 
                          bakery_state->supplies[SUPPLY_YEAST] > 0);
            break;
        case TEAM_CAKE:
            can_produce = (bakery_state->supplies[SUPPLY_WHEAT] > 0 && 
                          bakery_state->supplies[SUPPLY_SUGAR_SALT] > 0 &&
                          bakery_state->supplies[SUPPLY_BUTTER] > 0 &&
                          bakery_state->supplies[SUPPLY_MILK] > 0);
            break;
        case TEAM_SANDWICH:
            can_produce = (bakery_state->inventory[ITEM_BREAD][0] > 0 &&
                          bakery_state->supplies[SUPPLY_CHEESE_SALAMI] > 0);
            break;
        case TEAM_SWEETS:
            can_produce = (bakery_state->supplies[SUPPLY_SWEET_ITEMS] > 0 &&
                          bakery_state->supplies[SUPPLY_SUGAR_SALT] > 0);
            break;
        case TEAM_SWEET_PATISSERIE:
        case TEAM_SAVORY_PATISSERIE:
            can_produce = (bakery_state->inventory[ITEM_PASTE][0] > 0);
            break;
        default:
            break;
    }
    
    sem_unlock(0);
    return can_produce;
}
// Adjust production priorities based on inventory and customer demands
void adjust_production_priorities(void) {
    sem_lock(0);
    
    // Check inventory levels
    int sweet_patisseries_total = 0;
    int savory_patisseries_total = 0;
    
    for (int i = 0; i < 100; i++) {
        sweet_patisseries_total += bakery_state->inventory[ITEM_SWEET_PATISSERIE][i];
        savory_patisseries_total += bakery_state->inventory[ITEM_SAVORY_PATISSERIE][i];
    }
    
    // If there's a significant difference, reassign chefs
    if (sweet_patisseries_total < savory_patisseries_total * 0.5 && 
        bakery_state->chefs_per_team[TEAM_SAVORY_PATISSERIE] > 1) {
        // Move a chef from savory to sweet patisseries
        reassign_chefs(TEAM_SAVORY_PATISSERIE, TEAM_SWEET_PATISSERIE, 1);
        log_message("Adjusted production: moved chef from savory to sweet patisseries");
    } 
    else if (savory_patisseries_total < sweet_patisseries_total * 0.5 && 
             bakery_state->chefs_per_team[TEAM_SWEET_PATISSERIE] > 1) {
        // Move a chef from sweet to savory patisseries
        reassign_chefs(TEAM_SWEET_PATISSERIE, TEAM_SAVORY_PATISSERIE, 1);
        log_message("Adjusted production: moved chef from sweet to savory patisseries");
    }
    
    // Similarly check other product types and adjust as needed
    // ...
    
    sem_unlock(0);
}

// Print current bakery status
void print_bakery_status(void) {
    sem_lock(0);
    
    printf("\n===== BAKERY STATUS =====\n");
    printf("Running time: %ld seconds\n", time(NULL) - bakery_state->start_time);
    printf("Daily profit: $%.2f\n", bakery_state->daily_profit);
    printf("Complaints: %d/%d\n", bakery_state->customer_complaints, bakery_state->max_complaints);
    printf("Frustrated customers: %d/%d\n", bakery_state->frustrated_customers, bakery_state->max_frustrated_customers);
    printf("Missing items requests: %d/%d\n", bakery_state->missing_items_requests, bakery_state->max_missing_items_requests);
    
    printf("\n--- Inventory ---\n");
    for (int i = 0; i < ITEM_COUNT; i++) {
        int total = 0;
        for (int j = 0; j < 100; j++) {
            total += bakery_state->inventory[i][j];
        }
        printf("Item type %d: %d\n", i, total);
    }
    
    printf("\n--- Supplies ---\n");
    for (int i = 0; i < SUPPLY_COUNT; i++) {
        printf("Supply type %d: %d\n", i, bakery_state->supplies[i]);
    }
    
    printf("\n--- Staff ---\n");
    for (int i = 0; i < TEAM_COUNT; i++) {
        if (i <= TEAM_BREAD) {
            printf("Chef team %d: %d chefs\n", i, bakery_state->chefs_per_team[i]);
        } else if (i >= TEAM_BAKE_CAKES_SWEETS && i <= TEAM_BAKE_BREAD) {
            printf("Baker team %d: %d bakers\n", i, bakery_state->bakers_per_team[i]);
        }
    }
    printf("Supply employees: %d\n", bakery_state->supply_employees);
    printf("Sellers: %d\n", bakery_state->sellers);
    
    printf("=======================\n\n");
    
    sem_unlock(0);
}