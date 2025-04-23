#ifndef SELLER_H
#define SELLER_H

#include "shared.h"
#include "config.h"

// Seller states
typedef enum
{
    SELLER_IDLE,
    SELLER_SERVING,
    SELLER_ON_BREAK
} SellerState;

// Seller structure
typedef struct
{
    int id;
    pid_t pid;
    SellerState state;
    time_t last_break;
    int served_customers;
    int current_customer_id; 
    time_t service_start;   
} Seller;

void start_seller_process(int id, const BakeryConfig *config);
void simulate_seller(int id, const BakeryConfig *config);
void process_seller_messages(int seller_id, Seller *seller);
void take_seller_break(int seller_id, const BakeryConfig *config);
void update_seller_availability(int seller_id, SellerState new_state);

#endif // SELLER_H