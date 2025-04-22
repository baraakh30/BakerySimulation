#ifndef SHARED_H
#define SHARED_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <time.h>

// Team types
typedef enum {
    TEAM_PASTE,
    TEAM_CAKE,
    TEAM_SANDWICH,
    TEAM_SWEETS,
    TEAM_SWEET_PATISSERIE,
    TEAM_SAVORY_PATISSERIE,
    TEAM_BREAD,
    TEAM_BAKE_CAKES_SWEETS,
    TEAM_BAKE_PATISSERIES,
    TEAM_BAKE_BREAD,
    TEAM_SUPPLY_CHAIN,
    TEAM_SELLER,
    TEAM_COUNT
} TeamType;

// Item types
typedef enum {
    ITEM_BREAD,
    ITEM_CAKE,
    ITEM_SANDWICH,
    ITEM_SWEETS,
    ITEM_SWEET_PATISSERIE,
    ITEM_SAVORY_PATISSERIE,
    ITEM_PASTE,
    ITEM_COUNT
} ItemType;

// Supply types
typedef enum {
    SUPPLY_WHEAT,
    SUPPLY_YEAST,
    SUPPLY_BUTTER,
    SUPPLY_MILK,
    SUPPLY_SUGAR_SALT,
    SUPPLY_SWEET_ITEMS,
    SUPPLY_CHEESE_SALAMI,
    SUPPLY_COUNT
} SupplyType;

// Message types
typedef enum {
    MSG_ITEM_PRODUCED,
    MSG_ITEM_BAKED,
    MSG_ITEM_SOLD,
    MSG_SUPPLY_PURCHASED,
    MSG_CUSTOMER_COMPLAINT,
    MSG_CHEF_REASSIGNMENT
} MessageType;

// Structure to represent an item
typedef struct {
    ItemType type;
    int flavor;  // Index to identify specific item in category
    double price;
    int quality; // 1-100, determines probability of complaint
} Item;

// Structure to represent a message
typedef struct {
    long mtype;
    MessageType msg_type;
    pid_t sender_pid;
    union {
        struct {
            ItemType item_type;
            int flavor;
            int quantity;
        } item;
        struct {
            SupplyType supply_type;
            int quantity;
        } supply;
        struct {
            TeamType from_team;
            TeamType to_team;
            int num_chefs;
        } reassignment;
    } data;
} Message;

// Shared memory structure
typedef struct {
    // Simulation status
    int is_running;
    time_t start_time;
    double daily_profit;
    int customer_complaints;
    int frustrated_customers;
    int missing_items_requests;
    
    // Inventory
    int inventory[ITEM_COUNT][100];  // [item_type][flavor]
    int supplies[SUPPLY_COUNT];
    
    // Staff assignment
    int chefs_per_team[TEAM_COUNT];
    int bakers_per_team[TEAM_COUNT];
    int supply_employees;
    int sellers;
    
    // Configuration
    int max_complaints;
    int max_frustrated_customers;
    int max_missing_items_requests;
    double profit_threshold;
    int simulation_time_minutes;
    
    // Statistics for display
    int items_produced[ITEM_COUNT];
    int items_sold[ITEM_COUNT];
    int customers_served;
    int waiting_customers;
} BakeryState;

// Semaphore operations
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

// IPC identifiers
extern int shm_id;
extern int sem_id;
extern int msg_id;

// Shared memory pointer
extern BakeryState *bakery_state;

// Initialize shared memory, semaphores, and message queues
int init_ipc(void);

// Clean up IPC resources
void cleanup_ipc(void);

// Semaphore operations
void sem_lock(int sem_index);
void sem_unlock(int sem_index);

// Message queue operations
int send_message(Message *message);
int receive_message(Message *message, long type);

// Random number utilities
int random_range(int min, int max);
double random_float(void);

// Logging function
void log_message(const char *format, ...);

#endif