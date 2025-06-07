//simulation.h
#include <pthread.h>
#include <queue>

#define STARTING_CLIENT_NO 10
#define WAITER_NO 2
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
#define FULL_LIMIT 100


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
    NONE_FORK,
    LEFT_FORK,
    RIGHT_FORK,
    LEFT_RIGHT_FORK,
    EATING,
    FULL,
    STARVING
};

enum KitchenState{
    READY,
    COOKING,
    FINISHED
};



enum DishwasherState{
    CLEANING,
    AVAILABLE
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
    pthread_cond_t cond;
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

    int menu_id;

    pthread_mutex_t points_lock;
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
    pthread_mutex_t mutex_meal;
    pthread_mutex_t mutex_ready;
} Kitchen;

typedef struct {
    DishwasherState state;
    std::queue<int> dirty_fork;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} Dishwasher;


void* client_job(void* arg);
void client_think(Client* current_client);
void client_eat(Client* current_client);
void pick_up_forks(Client* current_client);
void release_forks(Client* current_client);
void fork_wait(int id);


void* waiter_job(void* arg);
void* kitchen_job(void* arg);
void* dishwasher_job(void* arg);



void begin_simulation();
void finish_simulation();

void init_interface();
void interface();
void terminate_interface();



extern int random_between(int a=0, int b=70);
void give_client_meal(Waiter* current_waiter);
void give_client_menu(Waiter* waiter, Client* client_served);
void seat_client(Waiter* waiter, Client* client_served);
void init();
void cleanup();