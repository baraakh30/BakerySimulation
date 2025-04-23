#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>

#include "../include/shared.h"
#include "../include/config.h"
#include "../include/bakery.h"
#include "../include/chef.h"
#include "../include/baker.h"
#include "../include/supply.h"
#include "../include/customer.h"
#include "../include/display.h"
#include "../include/seller.h"

BakeryConfig config;

// Process ID arrays
pid_t *chef_pids = NULL;
pid_t *baker_pids = NULL;
pid_t *supply_pids = NULL;
pid_t *seller_pids = NULL;
pid_t customer_generator_pid = -1;
pid_t display_pid = -1;
static pid_t main_process_pid = 0;

// Signal handler for SIGINT (Ctrl+C)
void sigint_handler(int signum)
{
    pid_t current_pid = getpid();

    if (current_pid == main_process_pid)
    {
        printf("\n[Main Process %d] Caught signal %d. Cleaning up and shutting down...\n", current_pid, signum);

        // Stop the simulation
        if (bakery_state)
        {
            stop_simulation();
        }

        // Wait a moment for processes to notice the stop signal
        sleep(1);

        // Terminate all child processes
        if (chef_pids)
        {
            for (int i = 0; i < config.num_chefs; i++)
            {
                if (chef_pids[i] > 0)
                {
                    kill(chef_pids[i], SIGKILL);
                }
            }
            free(chef_pids);
            chef_pids = NULL;
        }

        if (baker_pids)
        {
            for (int i = 0; i < config.num_bakers; i++)
            {
                if (baker_pids[i] > 0)
                {
                    kill(baker_pids[i], SIGKILL);
                }
            }
            free(baker_pids);
            baker_pids = NULL;
        }

        if (supply_pids)
        {
            for (int i = 0; i < config.num_supply_chain; i++)
            {
                if (supply_pids[i] > 0)
                {
                    kill(supply_pids[i], SIGKILL);
                }
            }
            free(supply_pids);
            supply_pids = NULL;
        }

        if (seller_pids)
        {
            for (int i = 0; i < config.num_sellers; i++)
            {
                if (seller_pids[i] > 0)
                {
                    kill(seller_pids[i], SIGKILL);
                }
            }
            free(seller_pids);
            seller_pids = NULL;
        }
        sem_lock(SEM_CUSTOMER_PIDS);
        for (int i = 0; i < bakery_state->num_customers; i++)
        {
            if (bakery_state->customer_pids[i] > 0)
            {
                kill(bakery_state->customer_pids[i], SIGINT);
            }
        }
        sem_unlock(SEM_CUSTOMER_PIDS);

        if (customer_generator_pid > 0)
        {
            kill(customer_generator_pid, SIGKILL);
            customer_generator_pid = -1;
        }

        if (display_pid > 0)
        {
            kill(display_pid, SIGKILL);
            display_pid = -1;
        }

        // Cleanup IPC resources
        cleanup_ipc();

        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char *argv[])
{
    main_process_pid = getpid();
    // Check command line arguments
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Seed random number generator
    srand(time(NULL));

    // Register signal handler for graceful termination
    signal(SIGINT, sigint_handler);

    // Initialize IPC resources
    if (init_ipc() != 0)
    {
        fprintf(stderr, "Failed to initialize IPC resources\n");
        return EXIT_FAILURE;
    }

    // Load configuration
    if (load_config(argv[1], &config) != 0)
    {
        fprintf(stderr, "Failed to load configuration from %s\n", argv[1]);
        cleanup_ipc();
        return EXIT_FAILURE;
    }

    // Initialize bakery state
    init_bakery_state(&config);

    printf("Starting bakery simulation with:\n");
    printf("- %d chef(s)\n", config.num_chefs);
    printf("- %d baker(s)\n", config.num_bakers);
    printf("- %d seller(s)\n", config.num_sellers);
    printf("- %d supply chain employee(s)\n", config.num_supply_chain);

    // Allocate memory for process IDs
    int total_chefs = config.num_chefs;
    int total_bakers = config.num_bakers;
    int total_supply = config.num_supply_chain;
    int total_sellers = config.num_sellers;

    chef_pids = (pid_t *)malloc(total_chefs * sizeof(pid_t));
    baker_pids = (pid_t *)malloc(total_bakers * sizeof(pid_t));
    supply_pids = (pid_t *)malloc(total_supply * sizeof(pid_t));
    seller_pids = (pid_t *)malloc(total_sellers * sizeof(pid_t));

    if (!chef_pids || !baker_pids || !supply_pids || !seller_pids)
    {
        fprintf(stderr, "Memory allocation failed\n");
        cleanup_ipc();
        return EXIT_FAILURE;
    }

    // Create chef processes
    int chef_id = 0;
    for (int team = TEAM_PASTE; team <= TEAM_BREAD; team++)
    {
        for (int i = 0; i < bakery_state->chefs_per_team[team]; i++)
        {
            pid_t pid = fork();
            if (pid == 0)
            {
                // Child process
                start_chef_process(chef_id, team, &config);
                exit(EXIT_SUCCESS); // Should never reach here
            }
            else if (pid > 0)
            {
                // Parent process
                chef_pids[chef_id] = pid;
                chef_id++;
            }
            else
            {
                perror("fork() failed for chef process");
            }
        }
    }

    // Create baker processes
    int baker_id = 0;
    for (int team = TEAM_BAKE_CAKES_SWEETS; team <= TEAM_BAKE_BREAD; team++)
    {
        for (int i = 0; i < bakery_state->bakers_per_team[team]; i++)
        {
            pid_t pid = fork();
            if (pid == 0)
            {
                // Child process
                start_baker_process(baker_id, team, &config);
                exit(EXIT_SUCCESS); // Should never reach here
            }
            else if (pid > 0)
            {
                // Parent process
                baker_pids[baker_id] = pid;
                baker_id++;
            }
            else
            {
                perror("fork() failed for baker process");
            }
        }
    }

    // Create supply chain processes
    for (int i = 0; i < config.num_supply_chain; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            // Child process
            start_supply_process(i, &config);
            exit(EXIT_SUCCESS); // Should never reach here
        }
        else if (pid > 0)
        {
            // Parent process
            supply_pids[i] = pid;
        }
        else
        {
            perror("fork() failed for supply process");
        }
    }
    // Create seller processes
    for (int i = 0; i < config.num_sellers; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            // Child process
            start_seller_process(i, &config);
            exit(EXIT_SUCCESS); // Should never reach here
        }
        else if (pid > 0)
        {
            // Parent process
            seller_pids[i] = pid;
        }
        else
        {
            perror("fork() failed for seller process");
        }
    }

    // Create customer generator process
    customer_generator_pid = fork();
    if (customer_generator_pid == 0)
    {
        // Child process
        start_customer_generator(&config);
        exit(EXIT_SUCCESS); // Should never reach here
    }
    else if (customer_generator_pid < 0)
    {
        perror("fork() failed for customer generator");
    }

    // Create display process (OpenGL visualization)
    display_pid = fork();
    if (display_pid == 0)
    {
        // Child process
        init_display(argc, argv);
        exit(EXIT_SUCCESS); // Should never reach here
    }
    else if (display_pid < 0)
    {
        perror("fork() failed for display process");
    }

    // Main process loop
    signal(SIGINT, sigint_handler);

    log_message("Main process started, monitoring simulation");
    while (1)
    {
        // Check if simulation should end
        check_simulation_end_conditions(&config);
        // Adjust production priorities periodically
        adjust_production_priorities();

        // Print bakery status every 5 seconds
        print_bakery_status();

        // Check if simulation is still running
        sem_lock(0);
        int running = bakery_state->is_running;
        sem_unlock(0);

        if (!running)
        {
            log_message("Simulation ended");
            break;
        }

        sleep(5); // Check status every 5 seconds
    }

    return EXIT_SUCCESS;
}