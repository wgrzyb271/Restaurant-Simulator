//simulation.h
#include <pthread.h>

#define STARTING_CLIENT_NO 10
#define WAITER_NO 1
#define KITCHEN_NO 1
// number of total tables
#define TABLE 5
// number of seats for each table
#define SEAT 5
#define PLATE 8
#define FORK 8
#define KNIFE 8
#define GLASS 8
#define MENU 8
#define FULL 100

extern pthread_mutex_t interface_lock;

extern bool running;
extern bool paused;

// General states of threads
enum ClientState{
    WAITING,
    THINKING,
    HUNGRY,
    EATING,
    STARVING
};

enum KitchenState{
    READY,
    BUSY,
    FINISHED
};

enum ItemState{
    CLEAN,
    DIRTY
};

typedef struct{
    int id;
    ItemState state;
    bool is_occupied;
    pthread_mutex_t mutex;
} ItemType;


// representation of client 
typedef struct Client{
    int id;
    int hunger_points;
    int meals;
    ClientState state;
    int table_id;
    int seat_id;
} Client;


typedef struct{
    int id;
    bool seats[SEAT];
    pthread_mutex_t lock[SEAT];
} Table;

typedef struct{
    int id;
    bool busy;
    pthread_mutex_t mutex;
} Waiter;

typedef struct{
    KitchenState state;
    pthread_mutex_t mutex;
} Kitchen;



void* client_job(void* arg);
void* waiter_job(void* arg);
void* kitchen_job(void* arg);

void begin_simulation();

void init_interface();
void interface();
void terminate_interface();

extern int random_between(int a=0, int b=70);
void init();
void cleanup();
