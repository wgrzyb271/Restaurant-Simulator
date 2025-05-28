//main.cpp
#include <ncurses.h>
#include <pthread.h>
//#include "simulation.h"
#include "TUI.h"


int main() {

    pthread_t interface_t;
    pthread_mutex_init(&interface_lock, nullptr);

    pthread_create(&interface_t, nullptr, interface, nullptr);

    init();
    begin_simulation();
    pthread_join(interface_t, nullptr);

    cleanup();
    pthread_mutex_destroy(&interface_lock);

     return 0;
}
