#include "../include/chef.h"

// Start chef process
void start_chef_process(int id, TeamType team, const BakeryConfig *config) {
    srand(time(NULL) ^ getpid());
    simulate_chef(id, team, config);
    // Parent process continues...
}

// Main chef simulation loop
void simulate_chef(int id, TeamType team, const BakeryConfig *config) {
    log_message("Chef %d on team %d started", id, team);
    
    while (bakery_state->is_running) {
        // Check if we have the ingredients we need
        if (!check_ingredients(team)) {
            // No ingredients, wait and try again
            sleep(1);
            continue;
        }
        
        // Produce an item
        int result = produce_item(team, id, config);
        
        if (result == 0) {
            // Successfully produced an item
            // Sleep for a random time to simulate production time
            int production_time = random_range(config->chef_production_time_min, 
                                              config->chef_production_time_max);
            sleep(production_time);
        } else {
            // Failed to produce, wait a bit
            sleep(1);
        }
    }
    
    log_message("Chef %d on team %d ending", id, team);
}

// Check if we have the necessary ingredients for the chef's team
int check_ingredients(TeamType team) {
    int result = 0;
    
    sem_lock(SUPPLY_COUNT); // Lock the supplies semaphore
    
    switch (team) {
        case TEAM_PASTE:
            // Need wheat, yeast, butter, milk
            result = (bakery_state->supplies[SUPPLY_WHEAT] > 0 &&
                     bakery_state->supplies[SUPPLY_YEAST] > 0 &&
                     bakery_state->supplies[SUPPLY_BUTTER] > 0 &&
                     bakery_state->supplies[SUPPLY_MILK] > 0);
            break;
            
        case TEAM_CAKE:
            // Need wheat, butter, milk, sugar, sweet items
            result = (bakery_state->supplies[SUPPLY_WHEAT] > 0 &&
                     bakery_state->supplies[SUPPLY_BUTTER] > 0 &&
                     bakery_state->supplies[SUPPLY_MILK] > 0 &&
                     bakery_state->supplies[SUPPLY_SUGAR_SALT] > 0 &&
                     bakery_state->supplies[SUPPLY_SWEET_ITEMS] > 0);
            break;
            
        case TEAM_SANDWICH:
            // Need bread, cheese, salami
            result = (bakery_state->inventory[ITEM_BREAD][0] > 0 &&
                     bakery_state->supplies[SUPPLY_CHEESE_SALAMI] > 0);
            break;
            
        case TEAM_SWEETS:
            // Need sugar, milk, butter, sweet items
            result = (bakery_state->supplies[SUPPLY_SUGAR_SALT] > 0 &&
                     bakery_state->supplies[SUPPLY_MILK] > 0 &&
                     bakery_state->supplies[SUPPLY_BUTTER] > 0 &&
                     bakery_state->supplies[SUPPLY_SWEET_ITEMS] > 0);
            break;
            
        case TEAM_SWEET_PATISSERIE:
        case TEAM_SAVORY_PATISSERIE:
            // Need paste, and for sweet also need sweet items
            if (bakery_state->inventory[ITEM_PASTE][0] > 0) {
                if (team == TEAM_SWEET_PATISSERIE) {
                    result = bakery_state->supplies[SUPPLY_SWEET_ITEMS] > 0;
                } else {
                    result = bakery_state->supplies[SUPPLY_CHEESE_SALAMI] > 0;
                }
            }
            break;
            
        default:
            result = 0;
    }
    
    sem_unlock(SUPPLY_COUNT); // Unlock the supplies semaphore
    return result;
}

// Get the item type that this chef team produces
ItemType get_chef_item_type(TeamType team) {
    switch (team) {
        case TEAM_PASTE:
            return ITEM_PASTE;
            case TEAM_BREAD:
            return ITEM_BREAD;
        case TEAM_CAKE:
            return ITEM_CAKE;
        case TEAM_SANDWICH:
            return ITEM_SANDWICH;
        case TEAM_SWEETS:
            return ITEM_SWEETS;
        case TEAM_SWEET_PATISSERIE:
            return ITEM_SWEET_PATISSERIE;
        case TEAM_SAVORY_PATISSERIE:
            return ITEM_SAVORY_PATISSERIE;
        default:
            return -1;
    }
}

// Produce an item based on team type
int produce_item(TeamType team, int chef_id, const BakeryConfig *config) {
    ItemType item_type = get_chef_item_type(team);
    if (item_type == -1) {
        return -1; // Invalid team type
    }
    
    int flavor = 0;
    int max_flavor = 0;
    
    // Determine appropriate flavor range based on item type
    switch (item_type) {
        case ITEM_BREAD:
            max_flavor = config->num_bread_categories;
            break;
        case ITEM_CAKE:
            max_flavor = config->num_cake_flavors;
            break;
        case ITEM_SANDWICH:
            max_flavor = config->num_sandwich_types;
            break;
        case ITEM_SWEETS:
            max_flavor = config->num_sweets_flavors;
            break;
        case ITEM_SWEET_PATISSERIE:
            max_flavor = config->num_sweet_patisseries; 
            break;
        case ITEM_SAVORY_PATISSERIE:
            max_flavor = config->num_savory_patisseries;
            break;
        default:
            max_flavor = 1; // Default for ITEM_PASTE
    }
    
    if (max_flavor > 0) {
        flavor = random_range(0, max_flavor - 1);
    }
    
    sem_lock(SUPPLY_COUNT); // Lock supplies
    
    // Consume ingredients based on team type
    switch (team) {
        case TEAM_PASTE:
            // Consume wheat, yeast, butter, milk
            if (bakery_state->supplies[SUPPLY_WHEAT] > 0 &&
                bakery_state->supplies[SUPPLY_YEAST] > 0 &&
                bakery_state->supplies[SUPPLY_BUTTER] > 0 &&
                bakery_state->supplies[SUPPLY_MILK] > 0) {
                
                bakery_state->supplies[SUPPLY_WHEAT]--;
                bakery_state->supplies[SUPPLY_YEAST]--;
                bakery_state->supplies[SUPPLY_BUTTER]--;
                bakery_state->supplies[SUPPLY_MILK]--;
            } else {
                sem_unlock(SUPPLY_COUNT);
                return -1;
            }
            break;
            
        case TEAM_BREAD:
            // Consume wheat, yeast, water (always available)
            if (bakery_state->supplies[SUPPLY_WHEAT] > 0 &&
                bakery_state->supplies[SUPPLY_YEAST] > 0) {
                
                bakery_state->supplies[SUPPLY_WHEAT]--;
                bakery_state->supplies[SUPPLY_YEAST]--;
            } else {
                sem_unlock(SUPPLY_COUNT);
                return -1;
            }
            break;
            
        case TEAM_CAKE:
            // Consume wheat, butter, milk, sugar, sweet items
            if (bakery_state->supplies[SUPPLY_WHEAT] > 0 &&
                bakery_state->supplies[SUPPLY_BUTTER] > 0 &&
                bakery_state->supplies[SUPPLY_MILK] > 0 &&
                bakery_state->supplies[SUPPLY_SUGAR_SALT] > 0 &&
                bakery_state->supplies[SUPPLY_SWEET_ITEMS] > 0) {
                
                bakery_state->supplies[SUPPLY_WHEAT]--;
                bakery_state->supplies[SUPPLY_BUTTER]--;
                bakery_state->supplies[SUPPLY_MILK]--;
                bakery_state->supplies[SUPPLY_SUGAR_SALT]--;
                bakery_state->supplies[SUPPLY_SWEET_ITEMS]--;
            } else {
                sem_unlock(SUPPLY_COUNT);
                return -1;
            }
            break;
            
        case TEAM_SANDWICH:
            // Consume bread, cheese, salami
            if (bakery_state->inventory[ITEM_BREAD][0] > 0 &&
                bakery_state->supplies[SUPPLY_CHEESE_SALAMI] > 0) {
                
                bakery_state->inventory[ITEM_BREAD][0]--;
                bakery_state->supplies[SUPPLY_CHEESE_SALAMI]--;
            } else {
                sem_unlock(SUPPLY_COUNT);
                return -1;
            }
            break;
            
        case TEAM_SWEETS:
            // Consume sugar, milk, butter, sweet items
            if (bakery_state->supplies[SUPPLY_SUGAR_SALT] > 0 &&
                bakery_state->supplies[SUPPLY_MILK] > 0 &&
                bakery_state->supplies[SUPPLY_BUTTER] > 0 &&
                bakery_state->supplies[SUPPLY_SWEET_ITEMS] > 0) {
                
                bakery_state->supplies[SUPPLY_SUGAR_SALT]--;
                bakery_state->supplies[SUPPLY_MILK]--;
                bakery_state->supplies[SUPPLY_BUTTER]--;
                bakery_state->supplies[SUPPLY_SWEET_ITEMS]--;
            } else {
                sem_unlock(SUPPLY_COUNT);
                return -1;
            }
            break;
            
        case TEAM_SWEET_PATISSERIE:
            // Consume paste and sweet items
            if (bakery_state->inventory[ITEM_PASTE][0] > 0 &&
                bakery_state->supplies[SUPPLY_SWEET_ITEMS] > 0) {
                
                bakery_state->inventory[ITEM_PASTE][0]--;
                bakery_state->supplies[SUPPLY_SWEET_ITEMS]--;
            } else {
                sem_unlock(SUPPLY_COUNT);
                return -1;
            }
            break;
            
        case TEAM_SAVORY_PATISSERIE:
            // Consume paste and cheese/salami
            if (bakery_state->inventory[ITEM_PASTE][0] > 0 &&
                bakery_state->supplies[SUPPLY_CHEESE_SALAMI] > 0) {
                
                bakery_state->inventory[ITEM_PASTE][0]--;
                bakery_state->supplies[SUPPLY_CHEESE_SALAMI]--;
            } else {
                sem_unlock(SUPPLY_COUNT);
                return -1;
            }
            break;
            
        default:
            sem_unlock(SUPPLY_COUNT);
            return -1;
    }
    
    sem_unlock(SUPPLY_COUNT); // Unlock supplies
    
    // Generate quality score for the item
    int quality = random_range(50, 100);
    
    // Create a message for the item produced
    Message msg;
    msg.mtype = 1; // General message queue
    msg.msg_type = MSG_ITEM_PRODUCED;
    msg.sender_pid = getpid();
    msg.data.item.item_type = item_type;
    msg.data.item.flavor = flavor;
    msg.data.item.quantity = 1;
    
    // Add the item to the inventory
    sem_lock(SUPPLY_COUNT + item_type + 1); // Lock the specific item type
    bakery_state->inventory[item_type][flavor]++;
    bakery_state->items_produced[item_type]++;
    sem_unlock(SUPPLY_COUNT + item_type + 1); // Unlock
    
    // Send the message
    if (send_message(&msg) == -1) {
        perror("Failed to send chef production message");
        return -1;
    }
    
    log_message("Chef %d produced item type %d flavor %d with quality %d", 
                chef_id, item_type, flavor, quality);
    
    return 0;
}