//simulation.h
#include <pthread.h>
#include <queue>

#define STARTING_CLIENT_NO 10
#define WAITER_NO 1
#define KITCHEN_NO 1
// number of total tables
#define TABLE 5
// number of seat_occupied for each table
#define SEAT 5
#define PLATE 8
#define FORK 8
#define KNIFE 8
#define GLASS 8
#define MENU 8
#define FULL 100

extern pthread_mutex_t interface_lock;

extern bool done;
extern bool paused;
extern pthread_mutex_t mutex;
extern pthread_cond_t cond;


// General states of threads
enum ClientState{
    WAITING,
    WAIT_MENU,
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

    bool seated;
    bool has_menu;
    bool has_food;

    pthread_mutex_t mutex;

    pthread_cond_t cond_seated;
    pthread_cond_t cond_menu;
    pthread_cond_t cond_food;
} Client;


typedef struct{
    int id;
    bool seat_occupied[SEAT];
    pthread_mutex_t lock[SEAT];
} Table;

typedef struct{
    int id;
    bool busy;
    pthread_mutex_t mutex;
} Waiter;

typedef struct{
    KitchenState state;
    std::queue<int> meal_queue; // queue stores client ids
    std::queue<int> ready_queue; // queue stores client ids
    pthread_mutex_t mutex;
} Kitchen;



void* client_job(void* arg);
void* waiter_job(void* arg);
void* kitchen_job(void* arg);

void begin_simulation();
void finish_simulation();

void init_interface();
void interface();
void terminate_interface();

extern int random_between(int a=0, int b=70);
void give_client_menu(Client* client_served);
void seat_client(Client* client_served);
void init();
void cleanup();