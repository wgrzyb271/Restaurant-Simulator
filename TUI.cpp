//
// Created by glitch on 5/28/25.
//
//TUI.cpp
#include <ncurses.h>
#include <pthread.h>
#include <cstring>
#include <csignal>

#include "TUI.h"


#define YELLOW 1
#define RED 2
#define GREEN 3
#define CYAN 4
#define WHITE 6
#define MAGENTA 5
#define PEACH 8

bool done = false;
bool paused = false;


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;



void print_centered(int row, const char* format, ...) {
    char buffer[256];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);


    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    int col = (max_x - std::strlen(buffer)) / 2;
    mvprintw(row, col, "%s", buffer);
}


void* interface(void*){
    // set TUI options
    initscr();
    noecho();
    curs_set(FALSE);
    nodelay(stdscr, TRUE);
    start_color();
    use_default_colors();

    init_pair(YELLOW, COLOR_YELLOW, -1);
    init_pair(RED, COLOR_RED, -1);
    init_pair(GREEN, COLOR_GREEN, -1);
    init_pair(CYAN, COLOR_CYAN, -1);
    init_pair(WHITE, COLOR_WHITE, -1);
    init_pair(MAGENTA, COLOR_MAGENTA, -1);
    init_pair(PEACH, 217, -1);


    // draw interface
    while(true) {
        pthread_mutex_lock(&mutex);
        bool local_done = done;
        bool is_paused = paused;
        pthread_mutex_unlock(&mutex);

        if (local_done) break;

        clear();
        print_centered(0, "Multithreaded Restaurant Simulation");

        int cols = 3;          // number of columns for clients
        int col_width = 30;    // width of each client column

        int clients_width = cols * col_width;  // clients block width
        int right_block_width = 25;             // kitchen + waiter block width
        int gap = 0;                           // gap between clients and kitchen/waiter

        int total_width = clients_width + gap + right_block_width;
        int start_x = (COLS - total_width) / 2;

        // Print clients
        for (int i = 0; i < STARTING_CLIENT_NO; i++) {
            int col = i % cols;
            int row = i / cols;
            int x = start_x + col * col_width;
            int y = 3 + row * 5;


            attron(A_BOLD);
            mvprintw(y, x, "Client %d", client[i].id);
            attroff(A_BOLD);


            attron(COLOR_PAIR(client_state_color(client[i].state)));
            mvprintw(y + 1, x, "%s", to_string(client[i].state));
            attroff(COLOR_PAIR(client_state_color(client[i].state)));

            mvprintw(y + 2, x, "Satiety: %d / %d", client[i].hunger_points, FULL);
            mvprintw(y + 3, x, "Meals: %d", client[i].meals);
        }

        // Print kitchen and waiter block
        int right_x = start_x + clients_width + gap;
        int kitchen_y = 3;

        attron(A_BOLD);
        mvprintw(kitchen_y, right_x, "Kitchen:");
        attroff(A_BOLD);
        attron(COLOR_PAIR(kitchen_state_color(kitchen.state)));
        mvprintw(kitchen_y + 1, right_x, "State: %s", to_string(kitchen.state));
        attroff(COLOR_PAIR(kitchen_state_color(kitchen.state)));

        attron(A_BOLD);
        mvprintw(kitchen_y + 3, right_x, "Waiter %d:", waiter[0].id);
        attroff(A_BOLD);
        if (waiter[0].busy)
            attron(COLOR_PAIR(RED));
        else
            attron(COLOR_PAIR(GREEN));

        mvprintw(kitchen_y + 4, right_x, "State: %s", waiter[0].busy ? "BUSY" : "AVAILABLE");
        attroff(COLOR_PAIR(waiter[0].busy ? RED : GREEN));

        print_centered(25, "Simulation status: %s", is_paused ? "PAUSED" : "RUNNING");
        print_centered(26, "Press 'q' or 'Q' to quit, space to pause/resume");

        refresh();


        int key = getch();
        if(key == 'q' || key == 'Q') {
            pthread_mutex_lock(&mutex);
            done = true;
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&mutex);
        } else if(key == ' ') {
            pthread_mutex_lock(&mutex);
            paused = !paused;
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&mutex);
        }


        // sleep for 0.01 seconds
        usleep(10000);
    }

    endwin();
    return nullptr;

}

const char* to_string(ClientState state){
    switch(state) {
        case ClientState::THINKING: return "THINKING";
        case ClientState::HUNGRY:   return "HUNGRY";
        case ClientState::EATING:   return "EATING";
        case ClientState::WAITING:  return "WAITING";
        case ClientState::WAIT_MENU:  return "WAIT_MENU";
        case ClientState::STARVING:     return "STARVING";
        default:                   return "UNKNOWN";
    }
}

const char* to_string(KitchenState state){
    switch(state) {
        case KitchenState::READY: return "READY";
        case KitchenState::BUSY:   return "BUSY";
        case KitchenState::FINISHED:   return "FINISHED";
        default:                   return "UNKNOWN";
    }
}

const char* to_string(ItemState state){
    switch(state) {
        case ItemState::CLEAN: return "CLEAN";
        case ItemState::DIRTY:   return "DIRTY";
        default:                   return "UNKNOWN";
    }
}

int client_state_color(ClientState state){
    switch(state) {
        case ClientState::THINKING: return YELLOW;
        case ClientState::HUNGRY:   return MAGENTA;
        case ClientState::EATING:   return GREEN;
        case ClientState::WAITING:  return CYAN;
        case ClientState::WAIT_MENU:  return PEACH;
        case ClientState::STARVING: return RED;
        default:                   return WHITE;
    }
}

int kitchen_state_color(KitchenState state){
    switch(state) {
        case KitchenState::READY:    return GREEN;
        case KitchenState::BUSY:     return RED;
        case KitchenState::FINISHED: return CYAN;
        default:                    return WHITE;
    }
}
