//simulation.cpp
#include <deque>
#include <pthread.h>
#include <random>
#include <csignal>
#include "simulation.h"

//interface
//pthread_t interface_t;
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
    while(true) {

        pthread_mutex_lock(&mutex);
        while(paused && !done) {
            pthread_cond_wait(&cond, &mutex);
        }

        pthread_mutex_unlock(&mutex);

        if(done){
            break;
        }

        switch (current_client->state) {
            case WAITING:
                pthread_mutex_lock(&current_client->mutex);
                if (!current_client->seated) {
                    pthread_cond_wait(&current_client->cond_seated, &current_client->mutex);
                }
                current_client->state = WAIT_MENU;
                pthread_mutex_unlock(&current_client->mutex);
                break;

            case WAIT_MENU:
                pthread_mutex_lock(&current_client->mutex);
                while (!current_client->has_menu)
                    pthread_cond_wait(&current_client->cond_menu, &current_client->mutex);
                current_client->state = THINKING;
                pthread_mutex_unlock(&current_client->mutex);


                break;
            case THINKING:
                pthread_mutex_lock(&current_client->mutex);
                current_client->state = HUNGRY;
                pthread_mutex_unlock(&current_client->mutex);

                break;
            case HUNGRY:
                break;
            case EATING:
                break;
            case STARVING:
                break;
            default:
                usleep(5000);
                break;
        }
        current_client->meals++;
        usleep(10000);
    }
    return nullptr;
}

void* waiter_job(void* arg){
    Waiter* current_waiter = (Waiter*) arg;
    while(true) {
        // pause / finish
        pthread_mutex_lock(&mutex);
        while(paused && !done) {
            pthread_cond_wait(&cond, &mutex);
        }
        pthread_mutex_unlock(&mutex);


        if(done) break;

        for(int i = 0; i < clients; i++) {
            Client* client_served = &client[i];
            if (client_served->state == WAITING && !client_served->seated) {
                seat_client(client_served);
            }

            if (client_served->state == WAIT_MENU && !client_served->has_menu) {
                give_client_menu(client_served);
            }
        }


        usleep(10000);
    }
    return nullptr;
}

void* kitchen_job(void* arg){
    Kitchen* current_kitchen = (Kitchen*) arg;
    while(true) {

        pthread_mutex_lock(&mutex);
        while(paused && !done) {
            pthread_cond_wait(&cond, &mutex);
        }
        pthread_mutex_unlock(&mutex);

        if(done) break;


        usleep(10000);
    }
    return nullptr;
}

void give_client_menu(Client* client_served){
    for(int m = 0; m < MENU; m++){
        pthread_mutex_lock(&menu[m].mutex);
        if(!menu[m].is_occupied){
            pthread_mutex_lock(&client_served->mutex);
            client_served->has_menu = true;
            menu[m].is_occupied = true;
            pthread_cond_signal(&client_served->cond_menu);
            pthread_mutex_unlock(&client_served->mutex);
            pthread_mutex_unlock(&menu[m].mutex);
            return;
        }
        pthread_mutex_unlock(&menu[m].mutex);
    }
}



void seat_client(Client* client_served){
    if(client_served->state == WAITING){
        for(int t = 0; t < TABLE; t++){
            for(int s = 0; s < SEAT; s++){
                pthread_mutex_lock(&table[t].lock[s]);
                if(!table[t].seat_occupied[s]){
                    table[t].seat_occupied[s] = true;

                    pthread_mutex_lock(&client_served->mutex);

                    client_served->seated = true;
                    client_served->table_id = t;
                    client_served->seat_id = s;

                    pthread_cond_signal(&client_served->cond_seated);

                    pthread_mutex_unlock(&client_served->mutex);
                    pthread_mutex_unlock(&table[t].lock[s]);

                    return;
                }
                pthread_mutex_unlock(&table[t].lock[s]);
            }
        }
    }
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

       client[i].seated = false;
       client[i].has_menu = false;
       client[i].has_food = false;


       pthread_mutex_init(&client[i].mutex, nullptr);
       pthread_cond_init(&client[i].cond_seated, nullptr);
       pthread_cond_init(&client[i].cond_menu, nullptr);
       pthread_cond_init(&client[i].cond_food, nullptr);


       // client threads
		  pthread_create(&client_t[i], nullptr, client_job, &client[i]);
   }

   // init table
   for(int i=0; i<TABLE; i++) {
       table[i].id = i;
       for (int j = 0; j < SEAT; j++) {
           table[i].seat_occupied[j] = false;
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



}

void finish_simulation(){
    // wait for threads termination
    for(int i=0; i< clients; i++)
        pthread_join(client_t[i], nullptr);

    for(int i=0; i< WAITER_NO; i++)
        pthread_join(waiter_t[i], nullptr);

    pthread_join(kitchen_t, nullptr);
}

void begin_simulation(){

    for(int i=0; i<STARTING_CLIENT_NO; i++){
        // client threads
        pthread_create(&client_t[i], nullptr, client_job, &client[i]);
    }

    for(int i=0; i<WAITER_NO; i++){
        pthread_create(&waiter_t[i], nullptr, waiter_job, &waiter[i]);
    }

    pthread_create(&kitchen_t, nullptr, kitchen_job, &kitchen);

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

