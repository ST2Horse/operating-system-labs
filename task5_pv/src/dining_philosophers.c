#include "pv_common.h"

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct {
    int id;
    int times;
} PhilosopherArg;

static sem_t dp_chopsticks[MAX_THREADS];
static sem_t dp_room;
static pthread_mutex_t dp_print_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t dp_state_mutex = PTHREAD_MUTEX_INITIALIZER;

static int dp_total = 0;
static int dp_state[MAX_THREADS];
static int dp_chopstick_owner[MAX_THREADS];

enum {
    THINKING = 0,
    WAITING = 1,
    EATING = 2
};

static void dp_show_state(const char *message) {
    pthread_mutex_lock(&dp_print_mutex);
    printf("\n[哲学家就餐] %s\n", message);

    printf("哲学家状态：");
    for (int i = 0; i < dp_total; i++) {
        const char *state_text = "思考";
        if (dp_state[i] == WAITING) {
            state_text = "等待";
        } else if (dp_state[i] == EATING) {
            state_text = "就餐";
        }
        printf("P%d:%s ", i + 1, state_text);
    }
    printf("\n");

    printf("筷子占用：");
    for (int i = 0; i < dp_total; i++) {
        if (dp_chopstick_owner[i] == 0) {
            printf("C%d:空闲 ", i + 1);
        } else {
            printf("C%d:P%d ", i + 1, dp_chopstick_owner[i]);
        }
    }
    printf("\n");
    pthread_mutex_unlock(&dp_print_mutex);
}

static void *philosopher_thread(void *arg) {
    PhilosopherArg *thread_arg = (PhilosopherArg *)arg;
    int id = thread_arg->id;
    int index = id - 1;
    int left = index;
    int right = (index + 1) % dp_total;

    for (int i = 0; i < thread_arg->times; i++) {
        pthread_mutex_lock(&dp_state_mutex);
        dp_state[index] = THINKING;
        pthread_mutex_unlock(&dp_state_mutex);
        char msg1[64];
	sprintf(msg1, "P%d 正在思考", id);
	dp_show_state(msg1);
	random_sleep(1, 3);

        pthread_mutex_lock(&dp_state_mutex);
        dp_state[index] = WAITING;
        pthread_mutex_unlock(&dp_state_mutex);
	char msg2[64];
	sprintf(msg2, "P%d 请求左右两根筷子", id);
	dp_show_state(msg2);
        sem_wait(&dp_room);
        sem_wait(&dp_chopsticks[left]);
        pthread_mutex_lock(&dp_state_mutex);
        dp_chopstick_owner[left] = id;
        pthread_mutex_unlock(&dp_state_mutex);

        sem_wait(&dp_chopsticks[right]);
        pthread_mutex_lock(&dp_state_mutex);
        dp_chopstick_owner[right] = id;
        dp_state[index] = EATING;
        pthread_mutex_unlock(&dp_state_mutex);

        char msg3[64];
	sprintf(msg3, "P%d 获得两根筷子，开始就餐", id);
	dp_show_state(msg3);
	random_sleep(2, 4);

        pthread_mutex_lock(&dp_state_mutex);
        dp_chopstick_owner[right] = 0;
        dp_chopstick_owner[left] = 0;
        dp_state[index] = THINKING;
        pthread_mutex_unlock(&dp_state_mutex);

        sem_post(&dp_chopsticks[right]);
        sem_post(&dp_chopsticks[left]);
        sem_post(&dp_room);

	char msg4[64];
	sprintf(msg4, "P%d 结束就餐并释放筷子", id);
	dp_show_state(msg4);
    }

    return NULL;
}

void run_dining_philosophers(void) {
    print_line();
    printf("哲学家就餐问题\n");
    print_line();

    dp_total = read_int("请输入哲学家数量，建议为 5：", 2, MAX_THREADS);
    int eat_times = read_int("请输入每个哲学家就餐次数：", 1, 10);

    pthread_t philosophers[MAX_THREADS];
    PhilosopherArg args[MAX_THREADS];

    for (int i = 0; i < dp_total; i++) {
        dp_state[i] = THINKING;
        dp_chopstick_owner[i] = 0;
        sem_init(&dp_chopsticks[i], 0, 1);
    }
    sem_init(&dp_room, 0, (unsigned int)(dp_total - 1));

    for (int i = 0; i < dp_total; i++) {
        args[i].id = i + 1;
        args[i].times = eat_times;
        pthread_create(&philosophers[i], NULL, philosopher_thread, &args[i]);
    }

    for (int i = 0; i < dp_total; i++) {
        pthread_join(philosophers[i], NULL);
    }

    for (int i = 0; i < dp_total; i++) {
        sem_destroy(&dp_chopsticks[i]);
    }
    sem_destroy(&dp_room);

    printf("哲学家就餐问题运行结束。\n");
}
