#include "../include/customer.h"

// Start customer generator process
void start_customer_generator(const BakeryConfig *config) {
    srand(time(NULL) ^ getpid());
    simulate_customer_generator(config);
    
    // Parent process continues...
}

// Main customer generator simulation loop
void simulate_customer_generator(const BakeryConfig *config) {
    log_message("Customer generator started");
    
    int customer_id = 0;
    
    while (bakery_state->is_running) {
        // Generate a new customer
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("Failed to fork customer process");
        } else if (pid == 0) {
            // Child process (customer)
            srand(time(NULL) ^ getpid());
            simulate_customer(customer_id, config);
            exit(EXIT_SUCCESS);
        }
        
        customer_id++;
        
        // Sleep for random time before next customer arrives
        int wait_time = random_range(config->customer_arrival_min, 
                                     config->customer_arrival_max);
        sleep(wait_time);
    }
    
    log_message("Customer generator ending");
}

// Simulate a customer
void simulate_customer(int id, const BakeryConfig *config) {
    Customer customer;
    
    customer.id = id;
    customer.pid = getpid();
    customer.state = CUSTOMER_ARRIVING;
    customer.arrival_time = time(NULL);
    customer.service_start_time = 0;
    
    // Randomly select what the customer wants
    ItemType available_items[] = {
        ITEM_BREAD, ITEM_CAKE, ITEM_SANDWICH, 
        ITEM_SWEETS, ITEM_SWEET_PATISSERIE, ITEM_SAVORY_PATISSERIE
    };
    int num_item_types = sizeof(available_items) / sizeof(available_items[0]);
    
    customer.wanted_item_type = available_items[random_range(0, num_item_types - 1)];
    
    // Determine max flavor based on item type
    int max_flavor = 0;
    switch (customer.wanted_item_type) {
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
            max_flavor = 1;
    }
    
    customer.wanted_flavor = random_range(0, max_flavor - 1);
    customer.num_items = random_range(1, 3);
    
    log_message("Customer %d arrived, wants %d of item type %d flavor %d", 
              customer.id, customer.num_items, customer.wanted_item_type, 
              customer.wanted_flavor);
    
    // Increment the waiting customers counter
    sem_lock(SUPPLY_COUNT + ITEM_COUNT + 1);
    bakery_state->waiting_customers++;
    sem_unlock(SUPPLY_COUNT + ITEM_COUNT + 1);
    
    customer.state = CUSTOMER_WAITING;
    
    // Wait for service
    int result = handle_customer(&customer, config);
    
    // Customer leaves
    sem_lock(SUPPLY_COUNT + ITEM_COUNT + 1);
    bakery_state->waiting_customers--;
    sem_unlock(SUPPLY_COUNT + ITEM_COUNT + 1);
    
    if (result == 0) {
        log_message("Customer %d served successfully and left satisfied", customer.id);
    } else if (result == 1) {
        log_message("Customer %d left frustrated due to long wait", customer.id);
        
        sem_lock(SUPPLY_COUNT + ITEM_COUNT + 2);
        bakery_state->frustrated_customers++;
        sem_unlock(SUPPLY_COUNT + ITEM_COUNT + 2);
    } else if (result == 2) {
        log_message("Customer %d left after complaining about item quality", customer.id);
        
        sem_lock(SUPPLY_COUNT + ITEM_COUNT + 2);
        bakery_state->customer_complaints++;
        sem_unlock(SUPPLY_COUNT + ITEM_COUNT + 2);
        
        // Send complaint message
        Message msg;
        msg.mtype = 1; // General message queue
        msg.msg_type = MSG_CUSTOMER_COMPLAINT;
        msg.sender_pid = getpid();
        msg.data.item.item_type = customer.wanted_item_type;
        msg.data.item.flavor = customer.wanted_flavor;
        
        if (send_message(&msg) == -1) {
            perror("Failed to send customer complaint message");
        }
    } else if (result == 3) {
        log_message("Customer %d left due to missing items", customer.id);
        
        sem_lock(SUPPLY_COUNT + ITEM_COUNT + 2);
        bakery_state->missing_items_requests++;
        sem_unlock(SUPPLY_COUNT + ITEM_COUNT + 2);
    }
}

// Handle a customer's service
int handle_customer(Customer *customer, const BakeryConfig *config) {
    time_t start_wait = time(NULL);
    time_t current_time;
    
    // Wait until there's an available seller or we get too frustrated
    while (1) {
        current_time = time(NULL);
        
        // Check if we've been waiting too long
        if (current_time - start_wait > config->customer_patience) {
            customer->state = CUSTOMER_LEAVING_FRUSTRATED;
            return 1; // Customer left frustrated
        }
        
        // Check if there's an available seller
        sem_lock(SUPPLY_COUNT + ITEM_COUNT + 1);
        int available_sellers = bakery_state->sellers - (bakery_state->customers_served % bakery_state->sellers);
        sem_unlock(SUPPLY_COUNT + ITEM_COUNT + 1);
        
        if (available_sellers > 0) {
            break; // Found an available seller
        }
        
        // Wait a bit before checking again
        usleep(500000); // 0.5 seconds
    }
    
    // Start being served
    customer->state = CUSTOMER_BEING_SERVED;
    customer->service_start_time = time(NULL);
    
    // Check if the requested item is available
    sem_lock(SUPPLY_COUNT + customer->wanted_item_type + 1);
    int items_available = bakery_state->inventory[customer->wanted_item_type][customer->wanted_flavor];
    sem_unlock(SUPPLY_COUNT + customer->wanted_item_type + 1);
    
    if (items_available < customer->num_items) {
        // Not enough items available
        customer->state = CUSTOMER_LEAVING_FRUSTRATED;
        return 3; // Missing items request
    }
    
    // Process the purchase
    sem_lock(SUPPLY_COUNT + customer->wanted_item_type + 1);
    bakery_state->inventory[customer->wanted_item_type][customer->wanted_flavor] -= customer->num_items;
    bakery_state->items_sold[customer->wanted_item_type] += customer->num_items;
    sem_unlock(SUPPLY_COUNT + customer->wanted_item_type + 1);
    
    // Calculate price and add to profit
    double item_price = config->prices[customer->wanted_item_type][customer->wanted_flavor];
    double total_price = item_price * customer->num_items;
    
    sem_lock(SUPPLY_COUNT + ITEM_COUNT + 2);
    bakery_state->daily_profit += total_price;
    bakery_state->customers_served++;
    sem_unlock(SUPPLY_COUNT + ITEM_COUNT + 2);
    
    // Create a message for the item sold
    Message msg;
    msg.mtype = 1; // General message queue
    msg.msg_type = MSG_ITEM_SOLD;
    msg.sender_pid = getpid();
    msg.data.item.item_type = customer->wanted_item_type;
    msg.data.item.flavor = customer->wanted_flavor;
    msg.data.item.quantity = customer->num_items;
    
    if (send_message(&msg) == -1) {
        perror("Failed to send item sold message");
    }
    
    // Random chance for customer to complain about quality
    if (random_float() < config->complaint_probability) {
        customer->state = CUSTOMER_COMPLAINING;
        
        // Refund the purchase
        sem_lock(SUPPLY_COUNT + ITEM_COUNT + 2);
        bakery_state->daily_profit -= total_price;
        sem_unlock(SUPPLY_COUNT + ITEM_COUNT + 2);
        
        // Other customers might leave due to the complaint
        sem_lock(SUPPLY_COUNT + ITEM_COUNT + 1);
        int waiting_customers = bakery_state->waiting_customers;
        sem_unlock(SUPPLY_COUNT + ITEM_COUNT + 1);
        
        for (int i = 0; i < waiting_customers; i++) {
            if (random_float() < config->leave_on_complaint_probability) {
                sem_lock(SUPPLY_COUNT + ITEM_COUNT + 2);
                bakery_state->frustrated_customers++;
                sem_unlock(SUPPLY_COUNT + ITEM_COUNT + 2);
            }
        }
        
        return 2; // Customer complained
    }
    
    // Successful transaction
    customer->state = CUSTOMER_LEAVING_SATISFIED;
    return 0; // Success
}