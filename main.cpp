//main.cpp
#include <ncurses.h>
#include <pthread.h>
//#include "simulation.h"
#include "TUI.h"


int main() {

    pthread_t interface_t;
    pthread_mutex_init(&interface_lock, nullptr);
    init();

    pthread_create(&interface_t, nullptr, interface, nullptr);
    begin_simulation();



    pthread_join(interface_t, nullptr);
    finish_simulation();


    cleanup();
    pthread_mutex_destroy(&interface_lock);

     return 0;
}
