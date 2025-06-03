//main.cpp
#include <ncurses.h>
#include <pthread.h>
//#include "simulation.h"
#include "TUI.h"


int main() {

    pthread_t interface_t;
    init();

    pthread_create(&interface_t, nullptr, interface, nullptr);
    begin_simulation();



    pthread_join(interface_t, nullptr);
    finish_simulation();


    cleanup();

     return 0;
}
