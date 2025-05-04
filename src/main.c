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
pid_t main_process_pid = 0;

// Signal handler for SIGINT (Ctrl+C)
void sigint_handler(int signum)
{
    pid_t current_pid = getpid();

    if (current_pid == main_process_pid)
    {
        printf("\n[Main Process %d] Caught signal %d. Cleaning up and shutting down...\n", current_pid, signum);

        // Wait a moment for processes to notice the stop signal
        printf("[Main Process] Waiting for processes to notice stop signal...\n");
        sleep(1);

        // Terminate all chef processes
        if (chef_pids)
        {
            printf("[Main Process] Terminating %d chef processes...\n", config.num_chefs);
            for (int i = 0; i < config.num_chefs; i++)
            {
                if (chef_pids[i] > 0)
                {
                 
                    if (kill(chef_pids[i], SIGTERM) < 0) {
                        perror("[Main Process] Failed to send SIGTERM to chef");
                    }
                    if (waitpid(chef_pids[i], NULL, 0) < 0) {
                        perror("[Main Process] Failed waiting for chef to terminate");
                    }
                }
            }
            free(chef_pids);
            chef_pids = NULL;
            printf("[Main Process] All chef processes terminated.\n");
        }
        else {
            printf("[Main Process] No chef processes to terminate.\n");
        }

        // Terminate all baker processes
        if (baker_pids)
        {
            printf("[Main Process] Terminating %d baker processes...\n", config.num_bakers);
            for (int i = 0; i < config.num_bakers; i++)
            {
                if (baker_pids[i] > 0)
                {
                    if (kill(baker_pids[i], SIGTERM) < 0) {
                        perror("[Main Process] Failed to send SIGTERM to baker");
                    }
                    if (waitpid(baker_pids[i], NULL, 0) < 0) {
                        perror("[Main Process] Failed waiting for baker to terminate");
                    }
                }
            }
            free(baker_pids);
            baker_pids = NULL;
            printf("[Main Process] All baker processes terminated.\n");
        }
        else {
            printf("[Main Process] No baker processes to terminate.\n");
        }

        // Terminate all supply chain processes
        if (supply_pids)
        {
            printf("[Main Process] Terminating %d supply chain processes...\n", config.num_supply_chain);
            for (int i = 0; i < config.num_supply_chain; i++)
            {
                if (supply_pids[i] > 0)
                {
                    if (kill(supply_pids[i], SIGTERM) < 0) {
                        perror("[Main Process] Failed to send SIGTERM to supply");
                    }
                    if (waitpid(supply_pids[i], NULL, 0) < 0) {
                        perror("[Main Process] Failed waiting for supply to terminate");
                    }
                }
            }
            free(supply_pids);
            supply_pids = NULL;
            printf("[Main Process] All supply chain processes terminated.\n");
        }
        else {
            printf("[Main Process] No supply chain processes to terminate.\n");
        }

        // Terminate all seller processes
        if (seller_pids)
        {
            printf("[Main Process] Terminating %d seller processes...\n", config.num_sellers);
            for (int i = 0; i < config.num_sellers; i++)
            {
                if (seller_pids[i] > 0)
                {
                    if (kill(seller_pids[i], SIGTERM) < 0) {
                        perror("[Main Process] Failed to send SIGTERM to seller");
                    }
                    if (waitpid(seller_pids[i], NULL, 0) < 0) {
                        perror("[Main Process] Failed waiting for seller to terminate");
                    }
                }
            }
            free(seller_pids);
            seller_pids = NULL;
            printf("[Main Process] All seller processes terminated.\n");
        }
        else {
            printf("[Main Process] No seller processes to terminate.\n");
        }

        // Special handling for customer processes
        printf("[Main Process] Handling customer processes...\n");
        if (bakery_state) {
        sem_lock(SEM_CUSTOMER_PIDS) ;
 
            
            printf("[Main Process] Terminating %d customer processes...\n", bakery_state->num_customers);
            for (int i = 0; i < bakery_state->num_customers + 1; i++)
            {
                if (bakery_state->customer_pids[i] > 0)
                {
                    if (kill(bakery_state->customer_pids[i], SIGTERM) < 0) {
                        perror("[Main Process] Failed to send SIGTERM to customer");
                    }
                    if (waitpid(bakery_state->customer_pids[i], NULL, 0) < 0) {
                        perror("[Main Process] Failed waiting for customer to terminate");
                    }
                }
            }
            
           sem_unlock(SEM_CUSTOMER_PIDS);
            printf("[Main Process] All customer processes handled.\n");
        } else {
            printf("[Main Process] Warning: bakery_state is NULL, cannot access customer PIDs\n");
        }

        // Terminate customer generator process
        if (customer_generator_pid > 0)
        {
            printf("[Main Process] Terminating customer generator (PID: %d)...\n", customer_generator_pid);
            if (kill(customer_generator_pid, SIGTERM) < 0) {
                perror("[Main Process] Failed to send SIGTERM to customer generator");
            }
            if (waitpid(customer_generator_pid, NULL, 0) < 0) {
                perror("[Main Process] Failed waiting for customer generator to terminate");
            }
            customer_generator_pid = -1;
        }
        else {
            printf("[Main Process] No customer generator process to terminate.\n");
        }

        // Terminate display process
        if (display_pid > 0)
        {
            printf("[Main Process] Terminating display process (PID: %d)...\n", display_pid);
            if (kill(display_pid, SIGTERM) < 0) {
                perror("[Main Process] Failed to send SIGTERM to display process");
            }
            if (waitpid(display_pid, NULL, 0) < 0) {
                perror("[Main Process] Failed waiting for display process to terminate");
            }
            display_pid = -1;
        }
        else {
            printf("[Main Process] No display process to terminate.\n");
        }

        // Only after all processes have been terminated, clean up IPC resources
        printf("[Main Process] All child processes terminated. Cleaning up IPC resources...\n");
        cleanup_ipc();
        printf("[Main Process] IPC resources cleanup complete.\n");

        exit(EXIT_SUCCESS);
    }
    else {
        // Child process received SIGINT, just exit cleanly
        printf("[Child Process %d] Received SIGINT, exiting cleanly...\n", current_pid);
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
    sigint_handler(SIGINT);
    return EXIT_SUCCESS;
}