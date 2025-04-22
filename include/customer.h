#ifndef CUSTOMER_H
#define CUSTOMER_H

#include "shared.h"
#include "config.h"

// Customer states
typedef enum {
    CUSTOMER_ARRIVING,
    CUSTOMER_WAITING,
    CUSTOMER_BEING_SERVED,
    CUSTOMER_LEAVING_SATISFIED,
    CUSTOMER_LEAVING_FRUSTRATED,
    CUSTOMER_COMPLAINING
} CustomerState;

// Customer structure
typedef struct {
    pid_t pid;
    int id;
    CustomerState state;
    time_t arrival_time;
    time_t service_start_time;
    ItemType wanted_item_type;
    int wanted_flavor;
    int num_items;
} Customer;

// Customer function prototypes
void start_customer_generator(const BakeryConfig *config);
void simulate_customer_generator(const BakeryConfig *config);
void simulate_customer(int id, const BakeryConfig *config);
int handle_customer(Customer *customer, const BakeryConfig *config);

#endif