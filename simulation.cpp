//simulation.cpp
#include <deque>
#include <pthread.h>
#include <random>
#include <csignal>
#include "simulation.h"

//interface
//pthread_t interface_t;

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
                client_think(current_client);

                break;
            case HUNGRY:{
//                usleep(100000);
                // wait for the order
                pthread_mutex_lock(&current_client->mutex);
                while (!current_client->has_food)
                    pthread_cond_wait(&current_client->cond_food, &current_client->mutex);
                // pick up forks
                pick_up_forks(current_client);

//                pthread_mutex_lock()
                current_client->state = EATING;
                pthread_mutex_unlock(&current_client->mutex);
                break;
                }
            case EATING:
                client_eat(current_client);
                break;
            case FULL:
                usleep(10000);
                break;
            case STARVING:
                break;
            default:
                usleep(5000);
                break;
        }

        usleep(10000);
    }
    return nullptr;
}

void client_think(Client* current_client){
    // choose a meal
    sleep(random_between(2, 3));

    // release menu

    pthread_mutex_lock(&current_client->mutex);
    int menu_id = current_client->menu_id;
    current_client->menu_id = -1;
    current_client->has_menu = false;

    pthread_mutex_lock(&menu[menu_id].mutex);
    menu[menu_id].is_occupied = false;
    pthread_mutex_unlock(&menu[menu_id].mutex);

    // change state
    current_client->state = HUNGRY;
    pthread_mutex_unlock(&current_client->mutex);

    // make an order
    pthread_mutex_lock(&kitchen.mutex_meal);
    kitchen.meal_queue.push(current_client->id);
    pthread_mutex_unlock(&kitchen.mutex_meal);

}

void pick_up_forks(Client* current_client){
    // TODO make sure client can only access forks at his table
    //  on his left/right else he must wait for it
    //  TODO add state of  forks clean/dirty and kitchen must clean it
    bool is_even = current_client->id % 2 == 0;
    int left_fork = (current_client->id + 1) % FORK;
    int right_fork = current_client->id % FORK;

    current_client->state = NONE_FORK;

    if(is_even){
        pthread_mutex_lock(&forks[right_fork].mutex);
//            pthread_mutex_lock(&current_client->mutex);
                current_client->state = RIGHT_FORK;
//            pthread_mutex_unlock(&current_client->mutex);
        forks[right_fork].is_occupied = true;

        pthread_mutex_lock(&forks[left_fork].mutex);
//            pthread_mutex_lock(&current_client->mutex);
                current_client->state = LEFT_RIGHT_FORK;
//            pthread_mutex_unlock(&current_client->mutex);
        forks[left_fork].is_occupied = true;

    } else {
        pthread_mutex_lock(&forks[left_fork].mutex);
//            pthread_mutex_lock(&current_client->mutex);
                current_client->state = LEFT_FORK;
//            pthread_mutex_unlock(&current_client->mutex);
        forks[left_fork].is_occupied = true;

        pthread_mutex_lock(&forks[right_fork].mutex);
//            pthread_mutex_lock(&current_client->mutex);
                current_client->state = LEFT_RIGHT_FORK;
//            pthread_mutex_unlock(&current_client->mutex);
        forks[right_fork].is_occupied = true;

    }
}

void release_forks(Client* current_client){
    bool is_even = current_client->id % 2 == 0;
    int left_fork = (current_client->id + 1) % FORK;
    int right_fork = current_client->id % FORK;

    if(is_even){
        forks[right_fork].is_occupied = false;
        pthread_mutex_unlock(&forks[right_fork].mutex);
//            pthread_mutex_lock(&current_client->mutex);
                current_client->state = LEFT_FORK;
//            pthread_mutex_unlock(&current_client->mutex);

        forks[left_fork].is_occupied = false;
        pthread_mutex_unlock(&forks[left_fork].mutex);
//            pthread_mutex_lock(&current_client->mutex);
                current_client->state = NONE_FORK;
//            pthread_mutex_unlock(&current_client->mutex);

    } else {
        forks[left_fork].is_occupied = false;
        pthread_mutex_unlock(&forks[left_fork].mutex);
//            pthread_mutex_lock(&current_client->mutex);
                current_client->state = RIGHT_FORK;
//            pthread_mutex_unlock(&current_client->mutex);

        forks[right_fork].is_occupied = false;
        pthread_mutex_unlock(&forks[right_fork].mutex);
//            pthread_mutex_lock(&current_client->mutex);
                current_client->state = NONE_FORK;
//            pthread_mutex_unlock(&current_client->mutex);

    }
}

void client_eat(Client* current_client){
    sleep(random_between(3, 4));

    pthread_mutex_lock(&current_client->mutex);
    release_forks(current_client);
    pthread_mutex_unlock(&current_client->mutex);

    current_client->meals++;

    pthread_mutex_lock(&current_client->points_lock);
    current_client->hunger_points += random_between(1,15);
    pthread_mutex_unlock(&current_client->points_lock);

    if (current_client->hunger_points < FULL_LIMIT)
        current_client->state = WAIT_MENU;
    else
        current_client->state = FULL;

    // TODO points thread to decrement and add even/not even fork pick up


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
                current_waiter->busy = true;
                seat_client(client_served);
                current_waiter->busy = false;
            }

            if (client_served->state == WAIT_MENU && !client_served->has_menu) {
                current_waiter->busy = true;
                give_client_menu(client_served);
                current_waiter->busy = false;
            }

//            if(client_served->state == HUNGRY && !client_served->has_food)
                give_client_meal(client_served, current_waiter);
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

        switch (kitchen.state) {

            case READY:
                // check if food is needed
                pthread_mutex_lock(&kitchen.mutex_meal);
                if (!kitchen.meal_queue.empty())
                    kitchen.state = BUSY;
                pthread_mutex_unlock(&kitchen.mutex_meal);
                break;
            case BUSY:
                // prepare food

                while (true){
                    pthread_mutex_lock(&kitchen.mutex_meal);

                    if (kitchen.meal_queue.empty()) {
                        kitchen.state = FINISHED;
                        pthread_mutex_unlock(&kitchen.mutex_meal);
                        break;
                    }
                    int meal = kitchen.meal_queue.front();
                    kitchen.meal_queue.pop();
                    pthread_mutex_unlock(&kitchen.mutex_meal);

                    // simulate preparation time
//                    sleep(random_between(5, 10));
                    sleep(random_between(1, 2));

                    // mark as finished
                    pthread_mutex_lock(&kitchen.mutex_ready);
                    kitchen.ready_queue.push(meal);
                    // notify client that his meal is ready
//                    pthread_cond_signal(&client[meal].cond_food);
                    pthread_mutex_unlock(&kitchen.mutex_ready);
                }

                break;
            case FINISHED:
                // show state
                usleep(1000);
                kitchen.state = READY;
                break;
            default:
                usleep(5000);
                break;

        }

        usleep(10000);
    }
    return nullptr;
}

void give_client_meal(Client* client_served, Waiter* current_waiter){
    // TODO waiter - hand meal round till empty queue
    pthread_mutex_lock(&kitchen.mutex_ready);
    if (!kitchen.ready_queue.empty()) {
        current_waiter->busy = true;
        int meal = kitchen.ready_queue.front();
        kitchen.ready_queue.pop();
        pthread_mutex_unlock(&kitchen.mutex_ready);

        pthread_mutex_lock(&client[meal].mutex);
        client[meal].has_food = true;
        pthread_cond_signal(&client[meal].cond_food);
        pthread_mutex_unlock(&client[meal].mutex);
        current_waiter->busy = false;
    } else {
        pthread_mutex_unlock(&kitchen.mutex_ready);
    }
}


void give_client_menu(Client* client_served){
    for(int m = 0; m < MENU; m++){
        pthread_mutex_lock(&menu[m].mutex);
        if(!menu[m].is_occupied){
            pthread_mutex_lock(&client_served->mutex);
            client_served->has_menu = true;
            menu[m].is_occupied = true;
            client_served->menu_id = m;
            pthread_cond_signal(&client_served->cond_menu);
            pthread_mutex_unlock(&client_served->mutex);
            pthread_mutex_unlock(&menu[m].mutex);
            return;
        }
        pthread_mutex_unlock(&menu[m].mutex);
    }
}



void seat_client(Client* client_served){

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

       client[i].menu_id = -1;

       pthread_mutex_init(&client[i].mutex, nullptr);
       pthread_mutex_init(&client[i].points_lock, nullptr);
       pthread_cond_init(&client[i].cond_seated, nullptr);
       pthread_cond_init(&client[i].cond_menu, nullptr);
       pthread_cond_init(&client[i].cond_food, nullptr);

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
    }

    // init kitchen
    kitchen.state = READY;
    pthread_mutex_init(&kitchen.mutex_meal, nullptr);
    pthread_mutex_init(&kitchen.mutex_ready, nullptr);




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

    for(int i = 0; i < clients; i++){
        pthread_mutex_destroy(&client[i].mutex);
        pthread_mutex_destroy(&client[i].points_lock);
        pthread_cond_destroy(&client[i].cond_seated);
        pthread_cond_destroy(&client[i].cond_menu);
        pthread_cond_destroy(&client[i].cond_food);
    }

	// destroy structures
	delete[] client;

	for(int i=0; i<TABLE; i++)
		for(int j=0; j<SEAT; j++)
			pthread_mutex_destroy(&table[i].lock[j]);



    for (int i = 0; i < WAITER_NO; i++)
        pthread_mutex_destroy(&waiter[i].mutex);

    pthread_mutex_destroy(&kitchen.mutex_meal);
    pthread_mutex_destroy(&kitchen.mutex_ready);

    delete[] client_t;
    delete[] waiter_t;

    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);


}

int random_between(int a, int b) {
    static std::random_device rd;
    static std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(a, b);
    return dist(mt);
}

