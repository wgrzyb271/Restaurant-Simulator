//
// Created by glitch on 5/28/25.
//
//TUI.h
#ifndef T_TUI_H
#define T_TUI_H

#include "simulation.h"

extern Client* client;
extern Group* group;
extern Waiter waiter[WAITER_NO];
extern Kitchen kitchen;
extern Dishwasher dishwasher[DISHWASHER_NO];

extern ItemType menu[MENU];
extern ItemType forks[FORK_NO];
extern ItemType knives[KNIFE_NO];

void* interface(void*);

const char*  to_string(ClientState state);
const char*  to_string(KitchenState state);
const char*  to_string(ItemState state);
const char* to_string(DishwasherState state);
void print_resources(int start_x);
void print_legend();


int client_state_color(ClientState state);
int kitchen_state_color(KitchenState state);
int dishwasher_state_color(DishwasherState state);

#endif //T_TUI_H
