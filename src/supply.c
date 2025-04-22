#include "../include/supply.h"

// Start supply chain employee process
void start_supply_process(int id, const BakeryConfig *config) {
    srand(time(NULL) ^ getpid());
    simulate_supply_employee(id, config);
}

// Main supply employee simulation loop
void simulate_supply_employee(int id, const BakeryConfig *config) {
    log_message("Supply employee %d started", id);
    
    while (bakery_state->is_running) {
        // Check if we need to purchase supplies
        purchase_supplies(id, config);
        
        // Sleep for a while before next purchase cycle
        sleep(random_range(10, 30));
    }
    
    log_message("Supply employee %d ending", id);
}

// Purchase supplies
int purchase_supplies(int employee_id, const BakeryConfig *config) {
    sem_lock(SUPPLY_COUNT); // Lock supplies
    
    // Check each supply type
    for (int i = 0; i < SUPPLY_COUNT; i++) {
        // If supply is below threshold, purchase more
        if (bakery_state->supplies[i] < config->supply_min[i]) {
            int amount = random_range(config->supply_min[i], config->supply_max[i]);
            bakery_state->supplies[i] += amount;
            
            // Create a message for the supply purchase
            Message msg;
            msg.mtype = 1; // General message queue
            msg.msg_type = MSG_SUPPLY_PURCHASED;
            msg.sender_pid = getpid();
            msg.data.supply.supply_type = i;
            msg.data.supply.quantity = amount;
            
            // Send the message
            if (send_message(&msg) == -1) {
                perror("Failed to send supply purchase message");
            }
            
            log_message("Supply employee %d purchased %d units of supply type %d", 
                      employee_id, amount, i);
        }
    }
    
    sem_unlock(SUPPLY_COUNT); // Unlock supplies
    return 0;
}