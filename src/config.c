#include "../include/config.h"
#include <string.h>

// Load configuration from file
int load_config(const char *filename, BakeryConfig *config)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Failed to open config file");
        return -1;
    }

    char line[256];
    char key[64];
    char value[192];

    // Set default values
    memset(config, 0, sizeof(BakeryConfig));
    config->num_bread_categories = 3;
    config->num_sandwich_types = 5;
    config->num_cake_flavors = 4;
    config->num_sweets_flavors = 6;
    config->num_sweet_patisseries = 4;
    config->num_savory_patisseries = 3;
    config->num_chefs = 10;
    config->num_bakers = 8;
    config->num_sellers = 3;
    config->num_supply_chain = 2;
    config->max_complaints = 10;
    config->max_frustrated_customers = 15;
    config->max_missing_items_requests = 20;
    config->profit_threshold = 5000.0;
    config->simulation_time_minutes = 30;
    config->chef_production_time_min = 2;
    config->chef_production_time_max = 10;
    config->baker_time_min = 5;
    config->baker_time_max = 15;
    config->customer_arrival_min = 5;
    config->customer_arrival_max = 20;
    config->customer_patience = 60;
    config->quality_threshold = 70;
    config->complaint_probability = 0.2;
    config->leave_on_complaint_probability = 0.5;

    // Set default supply ranges
    for (int i = 0; i < SUPPLY_COUNT; i++)
    {
        config->supply_min[i] = 10;
        config->supply_max[i] = 50;
    }

    // Set default prices
    for (int i = 0; i < ITEM_COUNT; i++)
    {
        for (int j = 0; j < 100; j++)
        {
            // Default prices by item type
            switch (i)
            {
            case ITEM_BREAD:
                config->prices[i][j] = 2.50;
                break;
            case ITEM_CAKE:
                config->prices[i][j] = 15.00;
                break;
            case ITEM_SANDWICH:
                config->prices[i][j] = 5.00;
                break;
            case ITEM_SWEETS:
                config->prices[i][j] = 3.50;
                break;
            case ITEM_SWEET_PATISSERIE:
                config->prices[i][j] = 4.50;
                break;
            case ITEM_SAVORY_PATISSERIE:
                config->prices[i][j] = 4.00;
                break;
            default:
                config->prices[i][j] = 1.00;
            }
        }
    }

    // Parse configuration file
    while (fgets(line, sizeof(line), file))
    {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n')
        {
            continue;
        }

        // Parse key-value pair
        if (sscanf(line, "%63[^=]=%191[^\n]", key, value) == 2)
        {
            // Remove whitespace
            char *ptr = key;
            while (*ptr)
            {
                if (*ptr == ' ' || *ptr == '\t')
                {
                    memmove(ptr, ptr + 1, strlen(ptr));
                }
                else
                {
                    ptr++;
                }
            }

            // Trim leading whitespace from value
            ptr = value;
            while (*ptr == ' ' || *ptr == '\t')
            {
                ptr++;
            }

            // Parse configurations
            if (strcmp(key, "num_bread_categories") == 0)
            {
                config->num_bread_categories = atoi(ptr);
            }
            else if (strcmp(key, "num_sandwich_types") == 0)
            {
                config->num_sandwich_types = atoi(ptr);
            }
            else if (strcmp(key, "num_cake_flavors") == 0)
            {
                config->num_cake_flavors = atoi(ptr);
            }
            else if (strcmp(key, "num_sweets_flavors") == 0)
            {
                config->num_sweets_flavors = atoi(ptr);
            }
            else if (strcmp(key, "num_sweet_patisseries") == 0)
            {
                config->num_sweet_patisseries = atoi(ptr);
            }
            else if (strcmp(key, "num_savory_patisseries") == 0)
            {
                config->num_savory_patisseries = atoi(ptr);
            }
            else if (strcmp(key, "num_chefs") == 0)
            {
                config->num_chefs = atoi(ptr);
            }
            else if (strcmp(key, "num_bakers") == 0)
            {
                config->num_bakers = atoi(ptr);
            }
            else if (strcmp(key, "num_sellers") == 0)
            {
                config->num_sellers = atoi(ptr);
            }
            else if (strcmp(key, "num_supply_chain") == 0)
            {
                config->num_supply_chain = atoi(ptr);
            }
            else if (strcmp(key, "max_complaints") == 0)
            {
                config->max_complaints = atoi(ptr);
            }
            else if (strcmp(key, "max_frustrated_customers") == 0)
            {
                config->max_frustrated_customers = atoi(ptr);
            }
            else if (strcmp(key, "max_missing_items_requests") == 0)
            {
                config->max_missing_items_requests = atoi(ptr);
            }
            else if (strcmp(key, "profit_threshold") == 0)
            {
                config->profit_threshold = atof(ptr);
            }
            else if (strcmp(key, "simulation_time_minutes") == 0)
            {
                config->simulation_time_minutes = atoi(ptr);
            }
            else if (strcmp(key, "chef_production_time_min") == 0)
            {
                config->chef_production_time_min = atoi(ptr);
            }
            else if (strcmp(key, "chef_production_time_max") == 0)
            {
                config->chef_production_time_max = atoi(ptr);
            }
            else if (strcmp(key, "baker_time_min") == 0)
            {
                config->baker_time_min = atoi(ptr);
            }
            else if (strcmp(key, "baker_time_max") == 0)
            {
                config->baker_time_max = atoi(ptr);
            }
            else if (strcmp(key, "customer_arrival_min") == 0)
            {
                config->customer_arrival_min = atoi(ptr);
            }
            else if (strcmp(key, "customer_arrival_max") == 0)
            {
                config->customer_arrival_max = atoi(ptr);
            }
            else if (strcmp(key, "customer_batch_min") == 0)
            {
                config->customer_batch_min = atoi(ptr);
            }
            else if (strcmp(key, "customer_batch_max") == 0)
            {
                config->customer_batch_max = atoi(ptr);
            }
            else if (strcmp(key, "purchase_quantity_min") == 0)
            {
                config->purchase_quantity_min = atoi(ptr);
            }
            else if (strcmp(key, "purchase_quantity_max") == 0)
            {
                config->purchase_quantity_max = atoi(ptr);
            }
            
            else if (strcmp(key, "customer_patience") == 0)
            {
                config->customer_patience = atoi(ptr);
            }
            else if (strcmp(key, "quality_threshold") == 0)
            {
                config->quality_threshold = atoi(ptr);
            }
            else if (strcmp(key, "complaint_probability") == 0)
            {
                config->complaint_probability = atof(ptr);
            }
            else if (strcmp(key, "leave_on_complaint_probability") == 0)
            {
                config->leave_on_complaint_probability = atof(ptr);
            }
            else if (strcmp(key, "accept_partial_probability") == 0) {
                config->accept_partial_probability = atof(value);
            }
            else if (strncmp(key, "supply_min_", 11) == 0)
            {
                int index = atoi(key + 11);
                if (index >= 0 && index < SUPPLY_COUNT)
                {
                    config->supply_min[index] = atoi(ptr);
                }
            }
            else if (strncmp(key, "supply_max_", 11) == 0)
            {
                int index = atoi(key + 11);
                if (index >= 0 && index < SUPPLY_COUNT)
                {
                    config->supply_max[index] = atoi(ptr);
                }
            }
            else if (strncmp(key, "price_", 6) == 0)
            {
                char item_type_str[32];
                int flavor;
                if (sscanf(key + 6, "%31[^_]_%d", item_type_str, &flavor) == 2)
                {
                    int item_type = -1;
                    if (strcmp(item_type_str, "bread") == 0)
                    {
                        item_type = ITEM_BREAD;
                    }
                    else if (strcmp(item_type_str, "cake") == 0)
                    {
                        item_type = ITEM_CAKE;
                    }
                    else if (strcmp(item_type_str, "sandwich") == 0)
                    {
                        item_type = ITEM_SANDWICH;
                    }
                    else if (strcmp(item_type_str, "sweets") == 0)
                    {
                        item_type = ITEM_SWEETS;
                    }
                    else if (strcmp(item_type_str, "sweet_patisserie") == 0)
                    {
                        item_type = ITEM_SWEET_PATISSERIE;
                    }
                    else if (strcmp(item_type_str, "savory_patisserie") == 0)
                    {
                        item_type = ITEM_SAVORY_PATISSERIE;
                    }

                    if (item_type >= 0 && flavor >= 0 && flavor < 100)
                    {
                        config->prices[item_type][flavor] = atof(ptr);
                    }
                }
            }
        }
    }

    fclose(file);
    return 0;
}

// Initialize bakery state from config
void init_bakery_state(const BakeryConfig *config)
{
    memset(bakery_state, 0, sizeof(BakeryState));

    bakery_state->is_running = 1;
    bakery_state->start_time = time(NULL);
    bakery_state->daily_profit = 0.0;
    bakery_state->customer_complaints = 0;
    bakery_state->frustrated_customers = 0;
    bakery_state->missing_items_requests = 0;

    // Initialize inventory to 0
    for (int i = 0; i < ITEM_COUNT; i++)
    {
        for (int j = 0; j < 100; j++)
        {
            bakery_state->inventory[i][j] = 0;
        }
    }

    // Initialize supplies to 0
    for (int i = 0; i < SUPPLY_COUNT; i++)
    {
        bakery_state->supplies[i] = 0;
    }

    // Distribute chefs among teams
    int total_teams = 7;
    int chefs_per_team = config->num_chefs / total_teams;
    int extra_chefs = config->num_chefs % total_teams;

    for (int i = TEAM_PASTE; i <= TEAM_BREAD; i++)
    {
        bakery_state->chefs_per_team[i] = chefs_per_team;
        if (extra_chefs > 0)
        {
            bakery_state->chefs_per_team[i]++;
            extra_chefs--;
        }
    }

    // Distribute bakers among teams
    total_teams = 3; // Number of baker teams
    int bakers_per_team = config->num_bakers / total_teams;
    int extra_bakers = config->num_bakers % total_teams;

    for (int i = TEAM_BAKE_CAKES_SWEETS; i <= TEAM_BAKE_BREAD; i++)
    {
        bakery_state->bakers_per_team[i] = bakers_per_team;
        if (extra_bakers > 0)
        {
            bakery_state->bakers_per_team[i]++;
            extra_bakers--;
        }
    }

    bakery_state->supply_employees = config->num_supply_chain;
    bakery_state->sellers = config->num_sellers;

    // Set thresholds
    bakery_state->max_complaints = config->max_complaints;
    bakery_state->max_frustrated_customers = config->max_frustrated_customers;
    bakery_state->max_missing_items_requests = config->max_missing_items_requests;
    bakery_state->profit_threshold = config->profit_threshold;
    bakery_state->simulation_time_minutes = config->simulation_time_minutes;
}