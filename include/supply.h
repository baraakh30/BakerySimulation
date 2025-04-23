#ifndef SUPPLY_H
#define SUPPLY_H

#include "shared.h"
#include "config.h"

// Supply chain employee structure
typedef struct {
    pid_t pid;
    int id;
} SupplyEmployee;

// Supply function prototypes
void start_supply_process(int id, const BakeryConfig *config);
void simulate_supply_employee(int id, const BakeryConfig *config);
int purchase_supplies(int employee_id, const BakeryConfig *config);
void purchase_specific_supply(int employee_id, int i);
void process_supply_messages(int employee_id);
#endif