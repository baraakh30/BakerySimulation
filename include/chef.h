#ifndef CHEF_H
#define CHEF_H

#include "shared.h"
#include "config.h"

// Chef structure
typedef struct {
    pid_t pid;
    TeamType team;
    int id;
} Chef;

// Chef function prototypes
void start_chef_process(int id, TeamType team, const BakeryConfig *config);
void simulate_chef(int id, TeamType team, const BakeryConfig *config);
int check_ingredients(TeamType team);
ItemType get_chef_item_type(TeamType team);
int produce_item(TeamType team, int chef_id, const BakeryConfig *config);

#endif