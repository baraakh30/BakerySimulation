#include "../include/seller.h"

// Start seller process
void start_seller_process(int id, const BakeryConfig *config)
{
    srand(time(NULL) ^ getpid());
    simulate_seller(id, config);
}

// Main seller simulation loop
void simulate_seller(int id, const BakeryConfig *config)
{
    log_message("Seller %d started with PID %d", id, getpid());

    Seller seller;
    seller.id = id;
    seller.pid = getpid();
    seller.state = SELLER_IDLE;
    seller.last_break = time(NULL);
    seller.served_customers = 0;
    seller.current_customer_id = -1;
    seller.service_start = 0;

    // Register seller in shared memory
    update_seller_availability(id, SELLER_IDLE);

    while (bakery_state->is_running)
    {
        // Process any messages directed to this seller
        process_seller_messages(id, &seller);

        // Check if it's time for a break, but only if not currently serving a customer
        time_t current_time = time(NULL);

        // If idle, signal availability for serving customers
        if (seller.state == SELLER_IDLE)
        {
            sem_lock(SEM_AVAILABLE_SELLERS);
            if (bakery_state->available_sellers < bakery_state->sellers)
            {
                bakery_state->available_sellers++;
            }
            sem_unlock(SEM_AVAILABLE_SELLERS);
        }
        // If serving a customer for too long, timeout and become available again
        if (seller.state == SELLER_SERVING &&
            seller.service_start > 0 &&
            current_time - seller.service_start > config->customer_patience)
        { // 20 seconds timeout
            log_message("Seller %d timed out while serving customer %d", id, seller.current_customer_id);
            seller.state = SELLER_IDLE;
            seller.current_customer_id = -1;
            seller.service_start = 0;
        }

        // Sleep for a short while before next check
        usleep(200000); // 0.2 seconds
    }

    log_message("Seller %d ending", id);
}

// Process seller-specific messages
void process_seller_messages(int seller_id, Seller *seller)
{
    Message msg;

    // Check for messages directed to this seller
    while (msgrcv(msg_id, &msg, sizeof(Message) - sizeof(long), seller_id + 100, IPC_NOWAIT) != -1)
    {
        switch (msg.msg_type)
        {
        case MSG_START_SERVING:
            log_message("Seller %d received service request from customer %d",
                        seller_id, msg.data.service.customer_id);

            // Only accept new customers if actually idle
            if (seller->state == SELLER_IDLE) {
                seller->state = SELLER_SERVING;
                seller->current_customer_id = msg.data.service.customer_id;
                seller->service_start = time(NULL);

                // Update availability in shared memory
                update_seller_availability(seller_id, SELLER_SERVING);

                // Acknowledge receipt of request to customer
                Message ack;
                ack.mtype = msg.sender_pid;
                ack.msg_type = MSG_SERVICE_ACKNOWLEDGED;
                ack.sender_pid = getpid();
                ack.data.service.seller_id = seller_id;
                ack.data.service.customer_id = msg.data.service.customer_id;
                
                if (send_message(&ack) == -1) {
                    perror("Failed to send service acknowledgment message");
                }

                // Simulate the time it takes to serve a customer
                int service_time = random_range(1, 3); // Reduced service time to prevent timeouts
                sleep(service_time);

                // When service is complete, send a confirmation message back
                Message response;
                response.mtype = msg.sender_pid; // Direct the response to the customer
                response.msg_type = MSG_SERVICE_COMPLETE;
                response.sender_pid = getpid();
                response.data.service.seller_id = seller_id;
                response.data.service.customer_id = msg.data.service.customer_id;

                if (send_message(&response) == -1) {
                    perror("Failed to send service completion message");
                }
            } else {
                // Reject the request if not idle
                Message reject;
                reject.mtype = msg.sender_pid;
                reject.msg_type = MSG_SERVICE_REJECTED;
                reject.sender_pid = getpid();
                reject.data.service.seller_id = seller_id;
                reject.data.service.customer_id = msg.data.service.customer_id;
                
                if (send_message(&reject) == -1) {
                    perror("Failed to send service rejection message");
                }
            }
            break;

        case MSG_CUSTOMER_LEFT:
            // Customer left before completing transaction
            if (seller->current_customer_id == msg.data.service.customer_id)
            {
                log_message("Seller %d: Customer %d left during service",
                            seller_id, msg.data.service.customer_id);

                seller->state = SELLER_IDLE;
                seller->current_customer_id = -1;
                seller->service_start = 0;

                update_seller_availability(seller_id, SELLER_IDLE);
            }
            break;

        case MSG_TRANSACTION_COMPLETE:
            // Transaction was completed (success or complaint)
            if (seller->current_customer_id == msg.data.service.customer_id)
            {
                log_message("Seller %d: Transaction completed for customer %d",
                            seller_id, msg.data.service.customer_id);

                // Atomic update - first complete current transaction then get ready for next
                seller->served_customers++;
                seller->state = SELLER_IDLE;
                seller->current_customer_id = -1;
                seller->service_start = 0;

                update_seller_availability(seller_id, SELLER_IDLE);
            }
            break;

        default:
            log_message("Seller %d received unknown message type: %d",
                        seller_id, msg.msg_type);
            break;
        }
    }

    // Clear errno if we only got ENOMSG (no message available)
    if (errno == ENOMSG)
    {
        errno = 0;
    }
}


// Update the availability of sellers in shared memory
void update_seller_availability(int seller_id, SellerState new_state)
{
    sem_lock(SEM_AVAILABLE_SELLERS);

    // Update seller state in shared memory
    if (new_state == SELLER_IDLE)
    {
        bakery_state->available_sellers++;
        log_message("Seller %d is now IDLE and available", seller_id);
    }
    else if (new_state != SELLER_IDLE)
    {
        if (bakery_state->available_sellers > 0)
        {
            bakery_state->available_sellers--;
            log_message("Seller %d is now BUSY (not available)", seller_id);
        }
    }

    sem_unlock(SEM_AVAILABLE_SELLERS);
}