#ifndef SHARED_H
#define SHARED_H
#define SEM_WAITING_CUSTOMERS     (SUPPLY_COUNT + ITEM_COUNT + 1)
#define SEM_AVAILABLE_SELLERS     (SUPPLY_COUNT + ITEM_COUNT + 2)
#define SEM_CUSTOMER_STATS        (SUPPLY_COUNT + ITEM_COUNT + 3)  // For frustrated_customers, etc.
#define SEM_ACTIVE_COMPLAINT      (SUPPLY_COUNT + ITEM_COUNT + 4)
#define SEM_CUSTOMER_PIDS         (SUPPLY_COUNT + ITEM_COUNT + 5)
#define SEM_PROFIT_STATS          (SUPPLY_COUNT + ITEM_COUNT + 6)

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>

// Constants
#define MAX_CUSTOMERS 500

// Enums for item types
typedef enum {
    ITEM_PASTE,
    ITEM_BREAD,
    ITEM_CAKE,
    ITEM_SANDWICH,
    ITEM_SWEETS,
    ITEM_SWEET_PATISSERIE,
    ITEM_SAVORY_PATISSERIE,
    ITEM_COUNT
} ItemType;

// Enums for supply types
typedef enum {
    SUPPLY_WHEAT,
    SUPPLY_YEAST,
    SUPPLY_SUGAR_SALT,
    SUPPLY_BUTTER,
    SUPPLY_MILK,
    SUPPLY_SWEET_ITEMS,
    SUPPLY_CHEESE_SALAMI,
    SUPPLY_COUNT
} SupplyType;

// Enums for teams
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
    TEAM_COUNT
} TeamType;


// Enums for message types
typedef enum {
    MSG_ITEM_PRODUCED,
    MSG_ITEM_SOLD,
    MSG_CHEF_REASSIGNMENT,
    MSG_START_SERVING,      
    MSG_SERVICE_COMPLETE,    
    MSG_CUSTOMER_LEFT,       
    MSG_TRANSACTION_COMPLETE,
    MSG_SERVICE_ACKNOWLEDGED, 
    MSG_SERVICE_REJECTED      
} MessageType;



typedef struct {
    // Simulation status
    int is_running;
    time_t start_time;
    double daily_profit;
    int customer_complaints;
    int frustrated_customers;
    int missing_items_requests;
    int active_complaint;

    // Inventory
    int inventory[ITEM_COUNT][100];  // [item_type][flavor]
    int supplies[SUPPLY_COUNT];
    
    // Staff assignment
    int chefs_per_team[TEAM_COUNT];
    int bakers_per_team[TEAM_COUNT];
    int supply_employees;
    int sellers;
    int available_sellers;  

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
    pid_t customer_pids[MAX_CUSTOMERS];
    int num_customers;
} BakeryState;

// Message data union for IPC
typedef union {
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
    
    struct {
        int customer_id;
        int seller_id;
        ItemType item_type;
        int flavor;
        int quantity;
    } service;
} MessageData;

// Message structure for IPC
typedef struct {
    long mtype;
    MessageType msg_type;
    pid_t sender_pid;
    MessageData data;
} Message;

// For semctl
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

// Global variables
extern int shm_id;
extern int sem_id;
extern int msg_id;
extern BakeryState *bakery_state;
extern pid_t main_process_pid;

// Function prototypes
int init_ipc(void);
void cleanup_ipc(void);
void sem_lock(int sem_index);
void sem_unlock(int sem_index);
int send_message(Message *message);
int receive_message(Message *message, long type);
int random_range(int min, int max);
double random_float(void);
void log_message(const char *format, ...);

#endif // SHARED_H