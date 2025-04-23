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


// Process ID arrays
pid_t *chef_pids = NULL;
pid_t *baker_pids = NULL;
pid_t *supply_pids = NULL;
pid_t customer_generator_pid = -1;
pid_t display_pid = -1;

// Signal handler for SIGINT (Ctrl+C)
void sigint_handler(int signum) {
    printf("\nCaught signal %d. Cleaning up and shutting down...\n", signum);
    
    // Stop the simulation
    if (bakery_state) {
        stop_simulation();
    }
    
    // Wait a moment for processes to notice the stop signal
    sleep(1);
    
    // Terminate all child processes
    if (chef_pids) {
        for (int i = 0; i < bakery_state->chefs_per_team[TEAM_PASTE] + 
                         bakery_state->chefs_per_team[TEAM_CAKE] +
                         bakery_state->chefs_per_team[TEAM_SANDWICH] +
                         bakery_state->chefs_per_team[TEAM_SWEETS] +
                         bakery_state->chefs_per_team[TEAM_SWEET_PATISSERIE] +
                         bakery_state->chefs_per_team[TEAM_SAVORY_PATISSERIE] +
                         bakery_state->chefs_per_team[TEAM_BREAD] ; i++) {
            if (chef_pids[i] > 0) {
                kill(chef_pids[i], SIGTERM);
            }
        }
        free(chef_pids);
    }
    
    if (baker_pids) {
        for (int i = 0; i < bakery_state->bakers_per_team[TEAM_BAKE_CAKES_SWEETS] +
                         bakery_state->bakers_per_team[TEAM_BAKE_PATISSERIES] +
                         bakery_state->bakers_per_team[TEAM_BAKE_BREAD]; i++) {
            if (baker_pids[i] > 0) {
                kill(baker_pids[i], SIGTERM);
            }
        }
        free(baker_pids);
    }
    
    if (supply_pids) {
        for (int i = 0; i < bakery_state->supply_employees; i++) {
            if (supply_pids[i] > 0) {
                kill(supply_pids[i], SIGTERM);
            }
        }
        free(supply_pids);
    }
    
    if (customer_generator_pid > 0) {
        kill(customer_generator_pid, SIGTERM);
    }
    
    if (display_pid > 0) {
        kill(display_pid, SIGTERM);
    }
    
    // Cleanup IPC resources
    cleanup_ipc();
    
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Seed random number generator
    srand(time(NULL));
    
    // Register signal handler for graceful termination
    signal(SIGINT, sigint_handler);
    
    // Initialize IPC resources
    if (init_ipc() != 0) {
        fprintf(stderr, "Failed to initialize IPC resources\n");
        return EXIT_FAILURE;
    }
    
    // Load configuration
    BakeryConfig config;
    if (load_config(argv[1], &config) != 0) {
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
    
    chef_pids = (pid_t *)malloc(total_chefs * sizeof(pid_t));
    baker_pids = (pid_t *)malloc(total_bakers * sizeof(pid_t));
    supply_pids = (pid_t *)malloc(total_supply * sizeof(pid_t));
    
    if (!chef_pids || !baker_pids || !supply_pids) {
        fprintf(stderr, "Memory allocation failed\n");
        cleanup_ipc();
        return EXIT_FAILURE;
    }
    
    // Create chef processes
    int chef_id = 0;
    for (int team = TEAM_PASTE; team <= TEAM_BREAD; team++) {
        for (int i = 0; i < bakery_state->chefs_per_team[team]; i++) {
            pid_t pid = fork();
            if (pid == 0) {
                // Child process
                start_chef_process(chef_id, team, &config);
                exit(EXIT_SUCCESS); // Should never reach here
            } else if (pid > 0) {
                // Parent process
                chef_pids[chef_id] = pid;
                chef_id++;
            } else {
                perror("fork() failed for chef process");
            }
        }
    }
    
    // Create baker processes
    int baker_id = 0;
    for (int team = TEAM_BAKE_CAKES_SWEETS; team <= TEAM_BAKE_BREAD; team++) {
        for (int i = 0; i < bakery_state->bakers_per_team[team]; i++) {
            pid_t pid = fork();
            if (pid == 0) {
                // Child process
                start_baker_process(baker_id, team, &config);
                exit(EXIT_SUCCESS); // Should never reach here
            } else if (pid > 0) {
                // Parent process
                baker_pids[baker_id] = pid;
                baker_id++;
            } else {
                perror("fork() failed for baker process");
            }
        }
    }
    
    // Create supply chain processes
    for (int i = 0; i < config.num_supply_chain; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            start_supply_process(i, &config);
            exit(EXIT_SUCCESS); // Should never reach here
        } else if (pid > 0) {
            // Parent process
            supply_pids[i] = pid;
        } else {
            perror("fork() failed for supply process");
        }
    }
    
    // Create customer generator process
    customer_generator_pid = fork();
    if (customer_generator_pid == 0) {
        // Child process
        start_customer_generator(&config);
        exit(EXIT_SUCCESS); // Should never reach here
    } else if (customer_generator_pid < 0) {
        perror("fork() failed for customer generator");
    }
    
    // Create display process (OpenGL visualization)
    display_pid = fork();
    if (display_pid == 0) {
        // Child process
        init_display(argc, argv);
        exit(EXIT_SUCCESS); // Should never reach here
    } else if (display_pid < 0) {
        perror("fork() failed for display process");
    }
    
    // Main process loop
    log_message("Main process started, monitoring simulation");
    while (1) {
        // Check if simulation should end
        check_simulation_end_conditions(&config);
        process_messages();
        // Adjust production priorities periodically
        adjust_production_priorities();
        
        // Print bakery status every 5 seconds
        print_bakery_status();
        
        // Check if simulation is still running
        sem_lock(0);
        int running = bakery_state->is_running;
        sem_unlock(0);
        
        if (!running) {
            log_message("Simulation ended");
            break;
        }
        
        sleep(5); // Check status every 5 seconds
    }
    
    // Final cleanup
    sigint_handler(SIGTERM);
    
    return EXIT_SUCCESS;
}