#include "../include/shared.h"
#include <stdarg.h>

// Global variables for IPC
int shm_id = -1;
int sem_id = -1;
int msg_id = -1;
BakeryState *bakery_state = NULL;

// Initialize IPC resources
int init_ipc(void)
{
    key_t key;

    // Generate keys for IPC
    key = ftok(".", 'S');
    if (key == -1)
    {
        perror("ftok failed");
        return -1;
    }

    // Create shared memory
    shm_id = shmget(key, sizeof(BakeryState), IPC_CREAT | 0666);
    if (shm_id == -1)
    {
        perror("shmget failed");
        return -1;
    }

    // Attach shared memory
    bakery_state = (BakeryState *)shmat(shm_id, NULL, 0);
    if (bakery_state == (void *)-1)
    {
        perror("shmat failed");
        shmctl(shm_id, IPC_RMID, NULL);
        return -1;
    }

    // Initialize semaphores (one for each resource type)
    key = ftok(".", 'T');
    sem_id = semget(key, SUPPLY_COUNT + ITEM_COUNT + 7, IPC_CREAT | 0666);
    if (sem_id == -1)
    {
        perror("semget failed");
        shmdt(bakery_state);
        shmctl(shm_id, IPC_RMID, NULL);
        return -1;
    }

    // Initialize all semaphores to 1 (available)
    union semun arg;
    unsigned short values[SUPPLY_COUNT + ITEM_COUNT + 7];
    for (int i = 0; i < SUPPLY_COUNT + ITEM_COUNT + 7; i++)
    {
        values[i] = 1;
    }
    arg.array = values;

    if (semctl(sem_id, 0, SETALL, arg) == -1)
    {
        perror("semctl failed");
        shmdt(bakery_state);
        shmctl(shm_id, IPC_RMID, NULL);
        semctl(sem_id, 0, IPC_RMID, NULL);
        return -1;
    }

    // Create message queue
    key = ftok(".", 'U');
    msg_id = msgget(key, IPC_CREAT | 0666);
    if (msg_id == -1)
    {
        perror("msgget failed");
        shmdt(bakery_state);
        shmctl(shm_id, IPC_RMID, NULL);
        semctl(sem_id, 0, IPC_RMID, NULL);
        return -1;
    }

    return 0;
}

// Clean up IPC resources
void cleanup_ipc(void)
{
    if (bakery_state)
    {
        shmdt(bakery_state);
    }

    if (shm_id != -1)
    {
        shmctl(shm_id, IPC_RMID, NULL);
    }

    if (sem_id != -1)
    {
        semctl(sem_id, 0, IPC_RMID, NULL);
    }

    if (msg_id != -1)
    {
        msgctl(msg_id, IPC_RMID, NULL);
    }
}

// Semaphore lock operation
void sem_lock(int sem_index)
{
    struct sembuf sb;
    sb.sem_num = sem_index;
    sb.sem_op = -1;
    sb.sem_flg = 0;

    if (semop(sem_id, &sb, 1) == -1)
    {
        perror("semop lock failed");
    }
}

// Semaphore unlock operation
void sem_unlock(int sem_index)
{
    struct sembuf sb;
    sb.sem_num = sem_index;
    sb.sem_op = 1;
    sb.sem_flg = 0;

    if (semop(sem_id, &sb, 1) == -1)
    {
        perror("semop unlock failed");
    }
}

// Send a message to the queue
int send_message(Message *message)
{
    return msgsnd(msg_id, message, sizeof(Message) - sizeof(long), 0);
}

// Receive a message from the queue
int receive_message(Message *message, long type)
{
    return msgrcv(msg_id, message, sizeof(Message) - sizeof(long), type, 0);
}

// Generate random integer in range [min, max]
int random_range(int min, int max)
{
    return min + rand() % (max - min + 1);
}

// Generate random float in range [0, 1]
double random_float(void)
{
    return (double)rand() / RAND_MAX;
}

// Log a message with timestamp to both stdout and a log file
void log_message(const char *format, ...)
{
    time_t now;
    char timestamp[26];
    FILE *logfile;

    time(&now);
    ctime_r(&now, timestamp);
    timestamp[24] = '\0'; // Remove newline

    // Format the message
    va_list args;
    va_start(args, format);

    // Print to stdout
    printf("[%s] ", timestamp);
    vprintf(format, args);
    printf("\n");
    fflush(stdout);

    // Print to file
    logfile = fopen("bakery_log.txt", "a");
    if (logfile != NULL)
    {
        fprintf(logfile, "[%s] ", timestamp);
        va_end(args); // End the first va_list before starting another

        va_start(args, format); // Restart va_list for the file output
        vfprintf(logfile, format, args);
        fprintf(logfile, "\n");
        fclose(logfile);
    }

    va_end(args);
}
