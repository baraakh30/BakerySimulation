#include "../include/baker.h"

// Start baker process
void start_baker_process(int id, TeamType team, const BakeryConfig *config) {
    srand(time(NULL) ^ getpid());
    simulate_baker(id, team, config);
    // Parent process continues...
}

// Main baker simulation loop
void simulate_baker(int id, TeamType team, const BakeryConfig *config) {
    log_message("Baker %d on team %d started", id, team);
    
    while (bakery_state->is_running) {
        ItemType item_type;
        int flavor;
        
        // Check if there are items to bake
        if (check_items_to_bake(team, &item_type, &flavor)) {
            // Bake the item
            if (bake_item(team, item_type, flavor, id, config) == 0) {
                // Successfully baked an item
                // Sleep for a random time to simulate baking time
                int baking_time = random_range(config->baker_time_min, 
                                              config->baker_time_max);
                sleep(baking_time);
            }
        } else {
            // No items to bake, wait a bit
            sleep(1);
        }
    }
    
    log_message("Baker %d on team %d ending", id, team);
}

// Check if there are items that need baking for this baker's team
int check_items_to_bake(TeamType team, ItemType *item_type, int *flavor) {
    int found = 0;
    
    // Lock all item inventories to check
    for (int i = 0; i < ITEM_COUNT; i++) {
        sem_lock(SUPPLY_COUNT + i + 1);
    }
    
    // Check for items based on the baker's team
    switch (team) {
        case TEAM_BAKE_CAKES_SWEETS:
            // Check for cakes or sweets to bake
            for (int i = 0; i < 100 && !found; i++) {
                if (bakery_state->inventory[ITEM_CAKE][i] > 0) {
                    *item_type = ITEM_CAKE;
                    *flavor = i;
                    found = 1;
                    break;
                }
            }
            
            if (!found) {
                for (int i = 0; i < 100 && !found; i++) {
                    if (bakery_state->inventory[ITEM_SWEETS][i] > 0) {
                        *item_type = ITEM_SWEETS;
                        *flavor = i;
                        found = 1;
                        break;
                    }
                }
            }
            break;
            
        case TEAM_BAKE_PATISSERIES:
            // Check for patisseries to bake
            for (int i = 0; i < 100 && !found; i++) {
                if (bakery_state->inventory[ITEM_SWEET_PATISSERIE][i] > 0) {
                    *item_type = ITEM_SWEET_PATISSERIE;
                    *flavor = i;
                    found = 1;
                    break;
                }
            }
            
            if (!found) {
                for (int i = 0; i < 100 && !found; i++) {
                    if (bakery_state->inventory[ITEM_SAVORY_PATISSERIE][i] > 0) {
                        *item_type = ITEM_SAVORY_PATISSERIE;
                        *flavor = i;
                        found = 1;
                        break;
                    }
                }
            }
            break;
            
        case TEAM_BAKE_BREAD:
            // Check for bread to bake
            for (int i = 0; i < 100 && !found; i++) {
                if (bakery_state->inventory[ITEM_BREAD][i] > 0) {
                    *item_type = ITEM_BREAD;
                    *flavor = i;
                    found = 1;
                    break;
                }
            }
            break;
            
        default:
            found = 0;
    }
    
    // Unlock all item inventories
    for (int i = 0; i < ITEM_COUNT; i++) {
        sem_unlock(SUPPLY_COUNT + i + 1);
    }
    
    return found;
}

// Bake an item
int bake_item(TeamType team, ItemType item_type, int flavor, int baker_id, const BakeryConfig *config) {
    sem_lock(SUPPLY_COUNT + item_type + 1); // Lock this item type
    
    // Check if item is still available
    if (bakery_state->inventory[item_type][flavor] <= 0) {
        sem_unlock(SUPPLY_COUNT + item_type + 1);
        return -1;
    }
    
    // Remove the unbaked item
    bakery_state->inventory[item_type][flavor]--;
    
    sem_unlock(SUPPLY_COUNT + item_type + 1);
    
    // Generate quality score for the baked item
    int quality = random_range(50, 100);
    
    // Create a message for the item baked
    Message msg;
    msg.mtype = 1; // General message queue
    msg.msg_type = MSG_ITEM_BAKED;
    msg.sender_pid = getpid();
    msg.data.item.item_type = item_type;
    msg.data.item.flavor = flavor;
    msg.data.item.quantity = 1;
    
    // Add the baked item back to inventory
    sem_lock(SUPPLY_COUNT + item_type + 1);
    bakery_state->inventory[item_type][flavor]++;
    sem_unlock(SUPPLY_COUNT + item_type + 1);
    
    // Send the message
    if (send_message(&msg) == -1) {
        perror("Failed to send baker baked message");
        return -1;
    }
    
    log_message("Baker %d baked item type %d flavor %d with quality %d", 
                baker_id, item_type, flavor, quality);
    
    return 0;
}