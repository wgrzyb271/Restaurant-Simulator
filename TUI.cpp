// created by glitch on 5/28/25.

#include <ncurses.h>
#include <pthread.h>
#include <cstring>
#include <cstdarg>
#include <unistd.h>

#include "TUI.h"

#define YELLOW 1
#define RED 2
#define GREEN 3
#define CYAN 4
#define WHITE 6
#define MAGENTA 5
#define PEACH 8
#define PINK 10
#define BROWN 11


bool done = false;
bool paused = false;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// print formatted string centered on the given window and row
void print_centered(WINDOW* win, int row, const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    int max_y, max_x;
    getmaxyx(win, max_y, max_x);
    int col = (max_x - std::strlen(buffer)) / 2;
    mvwprintw(win, row, col, "%s", buffer);
}

// print legend explaining client states and resource colors
void print_legend(WINDOW* win) {
    int start_y = 35;
    int start_x = 2;

    wattron(win, A_BOLD);
    mvwprintw(win, start_y, start_x, "Legend:");
    wattroff(win, A_BOLD);

    mvwprintw(win, start_y + 1, start_x, "Client states with forks:");
    mvwprintw(win, start_y + 2, start_x + 2, "..  - No forks");
    mvwprintw(win, start_y + 3, start_x + 2, "L.  - Has left fork");
    mvwprintw(win, start_y + 4, start_x + 2, ".R  - Has right fork");
    mvwprintw(win, start_y + 5, start_x + 2, "LR  - Has both forks");

    mvwprintw(win, start_y + 7, start_x, "Resources:");
    wattron(win, COLOR_PAIR(GREEN));
    mvwprintw(win, start_y + 8, start_x + 2, "Available");
    wattroff(win, COLOR_PAIR(GREEN));
    wattron(win, COLOR_PAIR(RED));
    mvwprintw(win, start_y + 9, start_x + 2, "Occupied");
    wattroff(win, COLOR_PAIR(RED));
}

// draw resource availability with color coding at specified horizontal position
void print_resources(WINDOW* win, int start_x) {
    int y = 25;

    // print header for available resources
    wattron(win, A_BOLD);
    mvwprintw(win, y, start_x, "Available Resources");
    wattroff(win, A_BOLD);

    y += 2;
    mvwprintw(win, y, start_x, "MENU: ");
    int x = start_x + strlen("MENU: ");

    // print menu item availability with colors
    for (int i = 0; i < MENU; ++i) {
        int color_pair = menu[i].is_occupied ? RED : GREEN;
        wattron(win, COLOR_PAIR(color_pair));

        char buf[32];
        // print YES if not occupied (available), NO if occupied
        sprintf(buf, "[%d] %-3s   ", i, !menu[i].is_occupied ? "YES" : "NO");

        mvwprintw(win, y, x, buf);
        x += strlen(buf);

        wattroff(win, COLOR_PAIR(color_pair));
    }

    y += 1;
    int forks_y = y;
    int forks_x = start_x;
    mvwprintw(win, forks_y, forks_x, "Available Forks: ");
    forks_x += strlen("Available Forks: ");

    for (int i = 0; i < FORK; ++i) {
        int color_pair = forks[i].is_occupied ? RED : GREEN;
        wattron(win, COLOR_PAIR(color_pair));

        char buf[16];
        sprintf(buf, "[%d] %-3s   ", i, !forks[i].is_occupied ? "YES" : "NO");

        mvwprintw(win, forks_y, forks_x, buf);
        forks_x += strlen(buf);

        wattroff(win, COLOR_PAIR(color_pair));
    }

    // print fork states (clean or dirty) below availability
    y += 2;
    wattron(win, A_BOLD);
    mvwprintw(win, y, start_x, "Forks states");
    wattroff(win, A_BOLD);

    y += 1;
    x = start_x;

    for (int i = 0; i < FORK; ++i) {
        int color_pair = forks[i].state == CLEAN ? WHITE : BROWN;
        wattron(win, COLOR_PAIR(color_pair));

        char buf[32];
        // print CLEAN if clean, DIRTY if dirty
        sprintf(buf, "[%d] %-6s   ", i, forks[i].state == CLEAN ? "CLEAN" : "DIRTY");

        mvwprintw(win, y, x, buf);
        x += strlen(buf);

        wattroff(win, COLOR_PAIR(color_pair));
    }
}


// main interface loop runs until user quits
void* interface(void*) {
    initscr();
    noecho();
    curs_set(FALSE);
    nodelay(stdscr, TRUE);
    start_color();
    use_default_colors();

    // initialize color pairs for various UI elements
    init_pair(YELLOW, COLOR_YELLOW, -1);
    init_pair(RED, COLOR_RED, -1);
    init_pair(GREEN, COLOR_GREEN, -1);
    init_pair(CYAN, COLOR_CYAN, -1);
    init_pair(WHITE, COLOR_WHITE, -1);
    init_pair(MAGENTA, COLOR_MAGENTA, -1);
    init_pair(PEACH, 217, -1);
    init_pair(PINK, 205, -1);

    init_color(BROWN, 500, 250, 0);
    init_pair(BROWN, BROWN, -1);


    // create off-screen window buffer for smooth drawing
    WINDOW* buffer = newwin(0, 0, 0, 0);

    while (true) {
        int key = getch();
        pthread_mutex_lock(&mutex);
        if (key == 'q' || key == 'Q') {
            done = true;
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&mutex);
            break;
        } else if (key == ' ') {
            paused = !paused;
            if (!paused) pthread_cond_broadcast(&cond);
        }
        pthread_mutex_unlock(&mutex);

        pthread_mutex_lock(&mutex);
        bool local_done = done;
        bool is_paused = paused;
        pthread_mutex_unlock(&mutex);
        if (local_done) break;

        // clear buffer window before drawing
        werase(buffer);

        // draw title centered at top
        print_centered(buffer, 0, "Multithreaded Restaurant Simulation");
        print_legend(buffer);

        int cols = 3;
        int col_width = 30;
        int clients_width = cols * col_width;
        int right_block_width = 25;
        int gap = 0;
        int total_width = clients_width + gap + right_block_width;
        int start_x = (COLS - total_width) / 2;

        // draw client information in a grid
        for (int i = 0; i < STARTING_CLIENT_NO; i++) {
            int col = i % cols;
            int row = i / cols;
            int x = start_x + col * col_width;
            int y = 3 + row * 5;

            wattron(buffer, A_BOLD);
            mvwprintw(buffer, y, x, "Client %d", client[i].id);
            wattroff(buffer, A_BOLD);

            wattron(buffer, COLOR_PAIR(client_state_color(client[i].state)));
            mvwprintw(buffer, y + 1, x, "%s", to_string(client[i].state));
            wattroff(buffer, COLOR_PAIR(client_state_color(client[i].state)));

            mvwprintw(buffer, y + 2, x, "Satiety: %d / %d", client[i].hunger_points, FULL_LIMIT);
            mvwprintw(buffer, y + 3, x, "Meals: %d", client[i].meals);
        }

        int right_x = start_x + clients_width + gap;
        int kitchen_y = 3;

        // draw kitchen status
        wattron(buffer, A_BOLD);
        mvwprintw(buffer, kitchen_y, right_x, "Kitchen:");
        wattroff(buffer, A_BOLD);
        wattron(buffer, COLOR_PAIR(kitchen_state_color(kitchen.state)));
        mvwprintw(buffer, kitchen_y + 1, right_x, "State: %s", to_string(kitchen.state));
        wattroff(buffer, COLOR_PAIR(kitchen_state_color(kitchen.state)));

        // draw dishwasher status
        wattron(buffer, A_BOLD);
        mvwprintw(buffer, kitchen_y + 3, right_x, "Dishwasher:");
        wattroff(buffer, A_BOLD);
        wattron(buffer, COLOR_PAIR(dishwasher_state_color(dishwasher.state)));
        mvwprintw(buffer, kitchen_y + 4, right_x, "State: %s", to_string(dishwasher.state));
        wattroff(buffer, COLOR_PAIR(dishwasher_state_color(dishwasher.state)));


        // draw waiter status
        wattron(buffer, A_BOLD);
        mvwprintw(buffer, kitchen_y + 6, right_x, "Waiter %d:", waiter[0].id);
        wattroff(buffer, A_BOLD);
        int waiter_color = waiter[0].busy ? RED : GREEN;
        wattron(buffer, COLOR_PAIR(waiter_color));
        mvwprintw(buffer, kitchen_y + 7, right_x, "State: %s", waiter[0].busy ? "BUSY" : "AVAILABLE");
        wattroff(buffer, COLOR_PAIR(waiter_color));


        // draw resources on left side below clients
        print_resources(buffer, start_x);

        // show simulation status and user controls at bottom center
        print_centered(buffer, 35, "Simulation status: %s", is_paused ? "PAUSED" : "RUNNING");
        print_centered(buffer, 36, "Press 'q' or 'Q' to quit, space to pause/resume");

        // copy buffer contents to stdscr and refresh screen
        wnoutrefresh(buffer);
        doupdate();

        // small sleep to reduce CPU usage
        usleep(10000);
    }

    // cleanup ncurses resources before exit
    delwin(buffer);
    endwin();
    return nullptr;
}

// convert client state enum to string representation
const char* to_string(ClientState state) {
    switch (state) {
        case ClientState::THINKING: return "THINKING";
        case ClientState::HUNGRY: return "HUNGRY";
        case ClientState::NONE_FORK: return "..";
        case ClientState::LEFT_FORK: return "L.";
        case ClientState::RIGHT_FORK: return ".R";
        case ClientState::LEFT_RIGHT_FORK: return "LR";
        case ClientState::EATING: return "EATING";
        case ClientState::FULL: return "FULL";
        case ClientState::WAITING: return "WAITING";
        case ClientState::WAIT_MENU: return "WAIT_MENU";
        case ClientState::STARVING: return "STARVING";
        default: return "UNKNOWN";
    }
}

// convert kitchen state enum to string
const char* to_string(KitchenState state) {
    switch (state) {
        case KitchenState::READY: return "READY";
        case KitchenState::COOKING: return "COOKING";
        case KitchenState::FINISHED: return "FINISHED";
        default: return "UNKNOWN";
    }
}



// convert item state enum to string
const char* to_string(ItemState state) {
    switch (state) {
        case ItemState::CLEAN: return "CLEAN";
        case ItemState::DIRTY: return "DIRTY";
        default: return "UNKNOWN";
    }
}

const char* to_string(DishwasherState state) {
    switch (state) {
        case DishwasherState::CLEANING: return "CLEANING";
        case DishwasherState::AVAILABLE: return "AVAILABLE";
        default: return "UNKNOWN";
    }
}

// map client states to color pairs
int client_state_color(ClientState state) {
    switch (state) {
        case ClientState::THINKING: return YELLOW;
        case ClientState::HUNGRY: return MAGENTA;
        case ClientState::EATING: return GREEN;
        case ClientState::FULL: return PINK;
        case ClientState::WAITING: return CYAN;
        case ClientState::WAIT_MENU: return PEACH;
        case ClientState::STARVING: return RED;
        default: return WHITE;
    }
}

// map kitchen states to color pairs
int kitchen_state_color(KitchenState state) {
    switch (state) {
        case KitchenState::READY: return GREEN;
        case KitchenState::COOKING: return RED;
        case KitchenState::FINISHED: return CYAN;
        default: return WHITE;
    }
}

int dishwasher_state_color(DishwasherState state) {
    switch (state) {
        case DishwasherState::AVAILABLE: return GREEN;
        case DishwasherState::CLEANING: return RED;
        default: return WHITE;
    }
}
