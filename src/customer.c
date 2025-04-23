#include "../include/customer.h"

// Start customer generator process
void start_customer_generator(const BakeryConfig *config)
{
    srand(time(NULL) ^ getpid());
    simulate_customer_generator(config);

    // Parent process continues...
}

// Main customer generator simulation loop
void simulate_customer_generator(const BakeryConfig *config)
{
    log_message("Customer generator started");

    int customer_id = 0;

    while (bakery_state->is_running)
    {
        // Generate a new customer
        pid_t pid = fork();

        if (pid < 0)
        {
            perror("Failed to fork customer process");
        }
        else if (pid == 0)
        {
            // Child process (customer)
            // Set default signal handler for customers
            signal(SIGINT, SIG_DFL);

            srand(time(NULL) ^ getpid());
            simulate_customer(customer_id, config);
            exit(EXIT_SUCCESS);
        }
        else
        {
            // Parent process - track this customer PID in shared memory
            sem_lock(SEM_CUSTOMER_PIDS); // Use an appropriate semaphore index
            if (bakery_state->num_customers < MAX_CUSTOMERS)
            {
                bakery_state->customer_pids[bakery_state->num_customers++] = pid;
            }
            sem_unlock(SEM_CUSTOMER_PIDS);
        }

        customer_id++;

        // Sleep for random time before next customer arrives
        int wait_time = random_range(config->customer_arrival_min,
                                     config->customer_arrival_max);
        sleep(wait_time);
    }

    log_message("Customer generator ending");
}

// Customer generator signal handler - simplified since main process will handle termination
void customer_generator_sigint_handler(int signum)
{
    log_message("Customer generator received signal %d, exiting...", signum);
    exit(EXIT_SUCCESS);
}

// Simulate a customer
void simulate_customer(int id, const BakeryConfig *config)
{
    Customer customer;

    customer.id = id;
    customer.pid = getpid();
    customer.state = CUSTOMER_ARRIVING;
    customer.arrival_time = time(NULL);
    customer.service_start_time = 0;

    // Randomly select what the customer wants
    ItemType available_items[] = {
        ITEM_BREAD, ITEM_CAKE, ITEM_SANDWICH,
        ITEM_SWEETS, ITEM_SWEET_PATISSERIE, ITEM_SAVORY_PATISSERIE};
    int num_item_types = sizeof(available_items) / sizeof(available_items[0]);

    customer.wanted_item_type = available_items[random_range(0, num_item_types - 1)];

    // Determine max flavor based on item type
    int max_flavor = 0;
    switch (customer.wanted_item_type)
    {
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
    sem_lock(SEM_WAITING_CUSTOMERS);
    bakery_state->waiting_customers++;
    sem_unlock(SEM_WAITING_CUSTOMERS);

    customer.state = CUSTOMER_WAITING;

    // Check if there's an active complaint happening - if so, customer may leave immediately
    sem_lock(SEM_ACTIVE_COMPLAINT);
    int active_complaint = bakery_state->active_complaint;
    sem_unlock(SEM_ACTIVE_COMPLAINT);

    if (active_complaint && random_float() < config->leave_on_complaint_probability)
    {
        log_message("Customer %d saw a complaint and decided to leave immediately", customer.id);

        // Customer leaves without being served
        sem_lock(SEM_WAITING_CUSTOMERS);
        bakery_state->waiting_customers--;
        sem_unlock(SEM_WAITING_CUSTOMERS);

        // Remove yourself from customer tracking
        sem_lock(SEM_CUSTOMER_PIDS);
        for (int i = 0; i < bakery_state->num_customers; i++)
        {
            if (bakery_state->customer_pids[i] == getpid())
            {
                // Replace this entry with the last one and decrement count
                bakery_state->customer_pids[i] = 0;
                bakery_state->num_customers--;
                break;
            }
        }
        sem_unlock(SEM_CUSTOMER_PIDS);

        // Simply exit the process
        exit(EXIT_SUCCESS);
    }

    // Wait for service
    int result = handle_customer(&customer, config);

    // Customer leaves
    sem_lock(SEM_WAITING_CUSTOMERS);
    bakery_state->waiting_customers--;
    sem_unlock(SEM_WAITING_CUSTOMERS);

    if (result == 0)
    {
        log_message("Customer %d served successfully and left satisfied", customer.id);
    }
    else if (result == 1)
    {
        log_message("Customer %d left frustrated due to long wait", customer.id);
        // Note: frustrated_customers counter is now updated within handle_customer
    }
    else if (result == 2)
    {
        log_message("Customer %d left after complaining about item quality", customer.id);

        sem_lock(SEM_CUSTOMER_STATS);
        bakery_state->customer_complaints++;
        sem_unlock(SEM_CUSTOMER_STATS);

        // Set the active complaint flag to trigger other customers to potentially leave
        sem_lock(SEM_ACTIVE_COMPLAINT);
        bakery_state->active_complaint = 1;
        sem_unlock(SEM_ACTIVE_COMPLAINT);

        // Reset the active complaint flag after a short time
        sleep(2); // Keep active for 2 seconds to give other processes a chance to see it

        sem_lock(SEM_ACTIVE_COMPLAINT);
        bakery_state->active_complaint = 0;
        sem_unlock(SEM_ACTIVE_COMPLAINT);
    }
    else if (result == 3)
    {
        log_message("Customer %d left due to missing items", customer.id);

        sem_lock(SEM_CUSTOMER_STATS);
        bakery_state->missing_items_requests++;
        sem_unlock(SEM_CUSTOMER_STATS);
    }

    else if (result == 4)
    {
        log_message("Customer %d saw a complaint and decided to leave immediately", customer.id);

        // Customer leaves without being served
        sem_lock(SEM_WAITING_CUSTOMERS);
        bakery_state->waiting_customers--;
        sem_unlock(SEM_WAITING_CUSTOMERS);

        // Remove yourself from customer tracking
        sem_lock(SEM_CUSTOMER_PIDS);
        for (int i = 0; i < bakery_state->num_customers; i++)
        {
            if (bakery_state->customer_pids[i] == getpid())
            {
                // Replace this entry with the last one and decrement count
                bakery_state->customer_pids[i] = 0;
                bakery_state->num_customers--;
                break;
            }
        }
        sem_unlock(SEM_CUSTOMER_PIDS);

        // Simply exit the process
        exit(EXIT_SUCCESS);
    }
    // Remove yourself from customer tracking when leaving
    sem_lock(SEM_CUSTOMER_PIDS);
    for (int i = 0; i < bakery_state->num_customers; i++)
    {
        if (bakery_state->customer_pids[i] == getpid())
        {
            // Replace this entry with the last one and decrement count
            bakery_state->customer_pids[i] = 0;
            bakery_state->num_customers--;
            break;
        }
    }
    sem_unlock(SEM_CUSTOMER_PIDS);
}

// Handle a customer's service
int handle_customer(Customer *customer, const BakeryConfig *config)
{
    time_t start_wait = time(NULL);
    time_t current_time;
    int seller_id = -1;
    int attempts = 0;

    // Wait until there's an available seller or we get too frustrated
    while (1)
    {
        current_time = time(NULL);

        // Check if we've been waiting too long
        if (current_time - start_wait > config->customer_patience)
        {
            customer->state = CUSTOMER_LEAVING_FRUSTRATED;
            log_message("Customer %d has been waiting too long and is leaving frustrated", customer->id);

            // Update customer stats when leaving frustrated
            sem_lock(SEM_CUSTOMER_STATS);
            bakery_state->frustrated_customers++;
            sem_unlock(SEM_CUSTOMER_STATS);

            return 1; // Customer left frustrated
        }

        // Check if a complaint just happened, and if so, maybe leave
        sem_lock(SEM_ACTIVE_COMPLAINT);
        int active_complaint = bakery_state->active_complaint;
        sem_unlock(SEM_ACTIVE_COMPLAINT);

        if (active_complaint && random_float() < config->leave_on_complaint_probability)
        {
            customer->state = CUSTOMER_LEAVING_FRUSTRATED;
            log_message("Customer %d saw a complaint during wait and decided to leave", customer->id);

            return 4;
        }

        // Check if there's an available seller
        sem_lock(SEM_AVAILABLE_SELLERS);
        int available_sellers = bakery_state->available_sellers;
        sem_unlock(SEM_AVAILABLE_SELLERS);

        if (available_sellers > 0)
        {
            // Find an available seller using round-robin through all sellers
            // Try each seller starting from a position based on customer ID
            int seller_found = 0;
            int start_seller = customer->id % bakery_state->sellers;

            for (int i = 0; i < bakery_state->sellers && !seller_found; i++)
            {
                seller_id = (start_seller + i) % bakery_state->sellers;

                // Send a service request message to this seller
                Message msg;
                msg.mtype = seller_id + 100; // Seller queue ID = 100 + seller_id
                msg.msg_type = MSG_START_SERVING;
                msg.sender_pid = getpid();
                msg.data.service.customer_id = customer->id;
                msg.data.service.item_type = customer->wanted_item_type;
                msg.data.service.flavor = customer->wanted_flavor;
                msg.data.service.quantity = customer->num_items;

                if (send_message(&msg) == -1)
                {
                    perror("Failed to send service request message");
                    continue;
                }

                // Wait for acknowledgment or rejection
                Message response;
                int got_response = 0;
                time_t ack_wait_start = time(NULL);

                while (!got_response && time(NULL) - ack_wait_start < 2)
                { // Wait max 2 seconds for response
                    if (msgrcv(msg_id, &response, sizeof(Message) - sizeof(long), getpid(), IPC_NOWAIT) != -1)
                    {
                        if (response.msg_type == MSG_SERVICE_ACKNOWLEDGED &&
                            response.data.service.seller_id == seller_id)
                        {
                            seller_found = 1;
                            got_response = 1;
                            log_message("Customer %d acknowledged by seller %d", customer->id, seller_id);
                        }
                        else if (response.msg_type == MSG_SERVICE_REJECTED &&
                                 response.data.service.seller_id == seller_id)
                        {
                            got_response = 1;
                            log_message("Customer %d rejected by seller %d, will try another", customer->id, seller_id);
                        }
                    }
                    usleep(100000); // 0.1 seconds between checks
                }

                if (seller_found)
                {
                    break;
                }
            }

            if (seller_found)
            {
                break; // Found and confirmed an available seller
            }

            // If we tried all sellers but none confirmed, wait a bit before trying again
            attempts++;
            if (attempts >= 3)
            {                    // After 3 complete attempts through all sellers
                usleep(1000000); // Wait 1 second before next round of attempts
                attempts = 0;
            }
            else
            {
                usleep(200000); // 0.2 seconds between seller attempts
            }
        }
        else
        {
            // No sellers available according to shared memory
            usleep(500000); // 0.5 seconds
        }
    }

    // Start being served
    customer->state = CUSTOMER_BEING_SERVED;
    customer->service_start_time = time(NULL);

    log_message("Customer %d is now being served by seller %d", customer->id, seller_id);

    // Wait for service completion message from seller
    Message response;
    int service_complete = 0;
    start_wait = time(NULL);

    while (!service_complete)
    {
        current_time = time(NULL);

        // Check if service is taking too long
        if (current_time - start_wait > config->customer_patience)
        {
            customer->state = CUSTOMER_LEAVING_FRUSTRATED;
            log_message("Customer %d got tired of waiting for service completion and is leaving", customer->id);

            // Need to tell the seller that the customer left
            Message cancel_msg;
            cancel_msg.mtype = seller_id + 100;
            cancel_msg.msg_type = MSG_CUSTOMER_LEFT;
            cancel_msg.sender_pid = getpid();
            cancel_msg.data.service.customer_id = customer->id;

            if (send_message(&cancel_msg) == -1)
            {
                perror("Failed to send customer cancellation message");
            }

            // Update frustrated customers count
            sem_lock(SEM_CUSTOMER_STATS);
            bakery_state->frustrated_customers++;
            sem_unlock(SEM_CUSTOMER_STATS);

            return 1; // Customer left frustrated
        }

        // Try to receive response from seller
        if (msgrcv(msg_id, &response, sizeof(Message) - sizeof(long), getpid(), IPC_NOWAIT) != -1)
        {
            if (response.msg_type == MSG_SERVICE_COMPLETE)
            {
                service_complete = 1;
            }
        }

        // If no message yet, wait a bit before trying again
        if (!service_complete)
        {
            usleep(100000); // 0.1 seconds
        }
    }

    // Check if the requested item is available
    sem_lock(SUPPLY_COUNT + customer->wanted_item_type + 1);
    int items_available = bakery_state->inventory[customer->wanted_item_type][customer->wanted_flavor];
    sem_unlock(SUPPLY_COUNT + customer->wanted_item_type + 1);

    if (items_available < customer->num_items)
    {
        // Not enough items available, but check if partial quantity can be offered
        if (items_available > 0)
        {
            // Offer partial quantity based on configuration probability
            if (random_float() < config->accept_partial_probability)
            {
                log_message("Customer %d accepted partial quantity (%d instead of requested %d)",
                            customer->id, items_available, customer->num_items);
                
                // Process the partial purchase
                sem_lock(SUPPLY_COUNT + customer->wanted_item_type + 1);
                bakery_state->inventory[customer->wanted_item_type][customer->wanted_flavor] -= items_available;
                bakery_state->items_sold[customer->wanted_item_type] += items_available;
                sem_unlock(SUPPLY_COUNT + customer->wanted_item_type + 1);
                
                // Calculate price for the partial quantity and add to profit
                double item_price = config->prices[customer->wanted_item_type][customer->wanted_flavor];
                double total_price = item_price * items_available;
                
                sem_lock(SEM_PROFIT_STATS);
                bakery_state->daily_profit += total_price;
                bakery_state->customers_served++;
                sem_unlock(SEM_PROFIT_STATS);
                
                // Create a message for the item sold
                Message sold_msg;
                sold_msg.mtype = 1; // General message queue
                sold_msg.msg_type = MSG_ITEM_SOLD;
                sold_msg.sender_pid = getpid();
                sold_msg.data.item.item_type = customer->wanted_item_type;
                sold_msg.data.item.flavor = customer->wanted_flavor;
                sold_msg.data.item.quantity = items_available;
                
                if (send_message(&sold_msg) == -1)
                {
                    perror("Failed to send item sold message");
                }
                
                // Tell the seller the transaction is complete
                Message complete_msg;
                complete_msg.mtype = seller_id + 100;
                complete_msg.msg_type = MSG_TRANSACTION_COMPLETE;
                complete_msg.sender_pid = getpid();
                complete_msg.data.service.customer_id = customer->id;
                
                if (send_message(&complete_msg) == -1)
                {
                    perror("Failed to send transaction completion message");
                }
                
                customer->state = CUSTOMER_LEAVING_SATISFIED;
                return 0; // Success with partial quantity
            }
            else {
                log_message("Customer %d rejected partial quantity offer (%d instead of %d)",
                            customer->id, items_available, customer->num_items);
            }
        }
        
        // Not enough items available and customer didn't accept partial quantity
        log_message("Customer %d couldn't be served because there are not enough items (requested: %d, available: %d)",
                    customer->id, customer->num_items, items_available);

        // Tell the seller the transaction is complete (even though items are missing)
        Message complete_msg;
        complete_msg.mtype = seller_id + 100;
        complete_msg.msg_type = MSG_TRANSACTION_COMPLETE;
        complete_msg.sender_pid = getpid();
        complete_msg.data.service.customer_id = customer->id;

        if (send_message(&complete_msg) == -1)
        {
            perror("Failed to send transaction completion message");
        }

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

    sem_lock(SEM_PROFIT_STATS);
    bakery_state->daily_profit += total_price;
    bakery_state->customers_served++;
    sem_unlock(SEM_PROFIT_STATS);

    // Create a message for the item sold
    Message sold_msg;
    sold_msg.mtype = 1; // General message queue
    sold_msg.msg_type = MSG_ITEM_SOLD;
    sold_msg.sender_pid = getpid();
    sold_msg.data.item.item_type = customer->wanted_item_type;
    sold_msg.data.item.flavor = customer->wanted_flavor;
    sold_msg.data.item.quantity = customer->num_items;

    if (send_message(&sold_msg) == -1)
    {
        perror("Failed to send item sold message");
    }

    // Tell the seller the transaction is complete
    Message complete_msg;
    complete_msg.mtype = seller_id + 100;
    complete_msg.msg_type = MSG_TRANSACTION_COMPLETE;
    complete_msg.sender_pid = getpid();
    complete_msg.data.service.customer_id = customer->id;

    if (send_message(&complete_msg) == -1)
    {
        perror("Failed to send transaction completion message");
    }

    // Random chance for customer to complain about quality
    if (random_float() < config->complaint_probability)
    {
        customer->state = CUSTOMER_COMPLAINING;

        // Refund the purchase
        sem_lock(SEM_PROFIT_STATS);
        bakery_state->daily_profit -= total_price;
        sem_unlock(SEM_PROFIT_STATS);

        // Set active complaint flag to trigger other customers to potentially leave
        sem_lock(SEM_ACTIVE_COMPLAINT);
        bakery_state->active_complaint = 1;
        sem_unlock(SEM_ACTIVE_COMPLAINT);

        return 2; // Customer complained
    }

    // Successful transaction
    customer->state = CUSTOMER_LEAVING_SATISFIED;
    return 0; // Success
}