//
// Created by glitch on 5/28/25.
//
//TUI.h
#ifndef T_TUI_H
#define T_TUI_H

#include "simulation.h"

extern Client* client;
extern Waiter waiter[WAITER_NO];
extern Kitchen kitchen;

extern ItemType menu[MENU];
extern ItemType forks[FORK];

void* interface(void*);

const char*  to_string(ClientState state);
const char*  to_string(KitchenState state);
const char*  to_string(ItemState state);
void print_resources(int start_x);
void print_legend();


int client_state_color(ClientState state);
int kitchen_state_color(KitchenState state);


#endif //T_TUI_H
