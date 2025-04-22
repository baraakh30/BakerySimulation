#ifndef BAKER_H
#define BAKER_H

#include "shared.h"
#include "config.h"

// Baker structure
typedef struct {
    pid_t pid;
    TeamType team;
    int id;
} Baker;

// Baker function prototypes
void start_baker_process(int id, TeamType team, const BakeryConfig *config);
void simulate_baker(int id, TeamType team, const BakeryConfig *config);
int check_items_to_bake(TeamType team, ItemType *item_type, int *flavor);
int bake_item(TeamType team, ItemType item_type, int flavor, int baker_id, const BakeryConfig *config);

#endif