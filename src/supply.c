#include "../include/supply.h"

// Start supply chain employee process
void start_supply_process(int id, const BakeryConfig *config)
{
    srand(time(NULL) ^ getpid());
    simulate_supply_employee(id, config);
}

// Main supply employee simulation loop
void simulate_supply_employee(int id, const BakeryConfig *config)
{
    log_message("Supply employee %d started", id);

    while (bakery_state->is_running)
    {
        // Process any messages
        process_supply_messages(id);

        // Check if we need to purchase supplies
        purchase_supplies(id, config);

        // Sleep for a while before next purchase cycle
        sleep(random_range(1, 10));
    }

    log_message("Supply employee %d ending", id);
}

// Purchase supplies
int purchase_supplies(int employee_id, const BakeryConfig *config)
{
    sem_lock(SUPPLY_COUNT); // Lock supplies

    // Check each supply type
    for (int i = 0; i < SUPPLY_COUNT; i++)
    {
        // If supply is below threshold, purchase more
        if (bakery_state->supplies[i] < config->supply_min[i])
        {
            int amount = random_range(config->supply_min[i], config->supply_max[i]);
            bakery_state->supplies[i] += amount;

            log_message("Supply employee %d purchased %d units of supply type %d",
                        employee_id, amount, i);
        }
    }

    sem_unlock(SUPPLY_COUNT); // Unlock supplies
    return 0;
}

// Process supply-specific messages
void process_supply_messages(int employee_id)
{
    Message msg;

    // Check for low inventory alerts
    while (msgrcv(msg_id, &msg, sizeof(Message) - sizeof(long), 1, IPC_NOWAIT) != -1)
    {
        if (msg.msg_type == MSG_ITEM_SOLD || msg.msg_type == MSG_ITEM_PRODUCED)
        {
            // Check if any supplies are critically low and need immediate attention
            sem_lock(SUPPLY_COUNT);
            for (int i = 0; i < SUPPLY_COUNT; i++)
            {
                if (bakery_state->supplies[i] < 5)
                { // Critical threshold
                    log_message("Supply employee %d detected critical shortage of supply %d",
                                employee_id, i);
                    sem_unlock(SUPPLY_COUNT);

                    // Immediately purchase this supply
                    purchase_specific_supply(employee_id, i);
                    break;
                }
            }
            sem_unlock(SUPPLY_COUNT);
        }
    }

    // Clear errno if we only got ENOMSG (no message available)
    if (errno == ENOMSG)
    {
        errno = 0;
    }
}

// Purchase a specific supply
void purchase_specific_supply(int employee_id, int supply_type)
{
    sem_lock(SUPPLY_COUNT);

    // Get the config values from bakery state
    int min_amount = 10; // Default value if config not available
    int max_amount = 50;

    int amount = random_range(min_amount, max_amount);
    bakery_state->supplies[supply_type] += amount;

    log_message("Supply employee %d urgently purchased %d units of supply type %d",
                employee_id, amount, supply_type);

    sem_unlock(SUPPLY_COUNT);
}