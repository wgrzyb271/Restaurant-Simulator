//simulation.cpp
#include <deque>
#include <pthread.h>
#include <random>
#include <csignal>
#include "simulation.h"

//interface
pthread_t interface_t;
pthread_mutex_t interface_lock;



// resources
ItemType plate[PLATE];
ItemType forks[FORK];
ItemType knife[KNIFE];
ItemType glass[GLASS];
ItemType menu[MENU];

// client thread
pthread_t* client_t;
int clients;

// waiter thread
pthread_t* waiter_t;

// kitchen thread
pthread_t kitchen_t;

// client array
Client* client;

// table array
Table table[TABLE];

// waiter
Waiter waiter[WAITER_NO];

// kitchen
Kitchen kitchen;

void* client_job(void* arg){
    Client* current_client = (Client *) arg;
    while(running) {
        current_client->meals++;
        usleep(10000);
    }
    return nullptr;
}

void* waiter_job(void* arg){
    return nullptr;
}

void* kitchen_job(void* arg){
    return nullptr;
}




void init(){
	// init structures
	   				
   // init client
   client = new Client[STARTING_CLIENT_NO];
   client_t = new pthread_t[STARTING_CLIENT_NO];

   clients = STARTING_CLIENT_NO;
   
   for(int i=0; i<STARTING_CLIENT_NO; i++){
        client[i].id = i;
        // each client has random hunger points
        client[i].hunger_points = random_between();
        client[i].meals = 0;
        client[i].state = WAITING;
        client[i].table_id = -1;
        client[i].seat_id = -1;
		
		  // client threads
		  pthread_create(&client_t[i], nullptr, client_job, &client[i]);
   }

   // init table 
   for(int i=0; i<TABLE; i++) {
       table[i].id = i;
       for (int j = 0; j < SEAT; j++) {
           table[i].seats[j] = false;
           pthread_mutex_init(&table[i].lock[j], nullptr);
       }
   }

    // init waiter
    waiter_t = new pthread_t[WAITER_NO];
    for(int i=0; i<WAITER_NO; i++){
        waiter[i].id = i;
        waiter[i].busy = false;
        pthread_mutex_init(&waiter[i].mutex, nullptr);
		  pthread_create(&waiter_t[i], nullptr, waiter_job, &waiter[i]);
    }

    // init kitchen
    kitchen.state = READY;
    pthread_mutex_init(&kitchen.mutex, nullptr);
    pthread_create(&kitchen_t, nullptr, kitchen_job, &kitchen);
	

}

void begin_simulation(){
    // wait for threads termination
    for(int i=0; i< clients; i++)
        pthread_join(client_t[i], nullptr);

    for(int i=0; i< WAITER_NO; i++)
        pthread_join(waiter_t[i], nullptr);

    pthread_join(kitchen_t, nullptr);
}

void cleanup(){


	// destroy structures
	delete[] client;

	for(int i=0; i<TABLE; i++)
		for(int j=0; j<SEAT; j++)
			pthread_mutex_destroy(&table[i].lock[j]);

    for (int i = 0; i < WAITER_NO; i++)
        pthread_mutex_destroy(&waiter[i].mutex);

    pthread_mutex_destroy(&kitchen.mutex);

    delete[] client_t;
    delete[] waiter_t;


}

int random_between(int a, int b) {
    static std::random_device rd;
    static std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(a, b);
    return dist(mt);
}

