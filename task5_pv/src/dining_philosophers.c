/*
 * 哲学家就餐问题 —— Dijkstra 状态数组方案
 *
 * 算法要点（对应教材第四章 PPT 第103页起）：
 *   - state[i]：每个哲学家的状态，取值 THINKING / HUNGRY / EATING
 *   - self[i] ：每个哲学家的私有信号量，初值 0
 *   - mutex   ：保护 state 数组的互斥信号量，初值 1
 *
 *   test(i)：若 state[i]==HUNGRY 且左右邻居都不是 EATING，
 *            则置 state[i]=EATING 并 V(self[i])。
 *
 *   哲学家循环：
 *     think
 *     P(mutex); state[i]=HUNGRY; test(i); V(mutex)
 *     P(self[i])          // 若 test 未让自己进入 EATING，则在此阻塞
 *     取左右筷子 / 打印
 *     eat
 *     放左右筷子 / 打印
 *     P(mutex); state[i]=THINKING; test(left); test(right); V(mutex)
 *
 * P 操作：sem_wait()   V 操作：sem_post()
 */

#include "pv_common.h"

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct {
    int id;
    int times;
} PhilosopherArg;

/* ---------- 全局变量 ---------- */
static int dp_total = 0;

/* 哲学家状态 */
enum { THINKING = 0, HUNGRY = 1, EATING = 2 };
static int  dp_state[MAX_THREADS];          /* state[0..N-1]  */

/* 信号量 */
static sem_t dp_self[MAX_THREADS];          /* 每人一个私有信号量，初值 0 */
static sem_t dp_mutex;                      /* 保护 state，初值 1          */

/* 打印辅助 */
static pthread_mutex_t dp_print_mutex = PTHREAD_MUTEX_INITIALIZER;
static int  dp_chopstick_owner[MAX_THREADS];/* 0 = 空闲，否则为持有者编号 */

/* ---------- 状态显示 ---------- */
static void dp_show_state(const char *message)
{
    pthread_mutex_lock(&dp_print_mutex);
    printf("\n[哲学家就餐] %s\n", message);

    printf("哲学家状态：");
    for (int i = 0; i < dp_total; i++) {
        const char *s = "思考";
        if (dp_state[i] == HUNGRY) s = "饥饿";
        else if (dp_state[i] == EATING) s = "就餐";
        printf("P%d:%s ", i + 1, s);
    }
    printf("\n");

    printf("筷子占用：");
    for (int i = 0; i < dp_total; i++) {
        if (dp_chopstick_owner[i] == 0)
            printf("C%d:空闲 ", i + 1);
        else
            printf("C%d:P%d ", i + 1, dp_chopstick_owner[i]);
    }
    printf("\n");
    pthread_mutex_unlock(&dp_print_mutex);
}

/* ---------- test(i) ---------- */
/*
 * 在 mutex 保护下调用。
 * 若 i 饥饿且两侧邻居都不在就餐，则令 i 进入就餐并 V(self[i])。
 */
static void test(int i)
{
    int left  = (i + dp_total - 1) % dp_total;
    int right = (i + 1) % dp_total;

    if (dp_state[i] == HUNGRY &&
        dp_state[left]  != EATING &&
        dp_state[right] != EATING)
    {
        dp_state[i] = EATING;
        sem_post(&dp_self[i]);          /* V(self[i]) */
    }
}

/* ---------- 哲学家线程 ---------- */
static void *philosopher_thread(void *arg)
{
    PhilosopherArg *thread_arg = (PhilosopherArg *)arg;
    int id    = thread_arg->id;
    int index = id - 1;
    int left  = (index + dp_total - 1) % dp_total;  /* 左筷子编号 */
    int right = index;                                /* 右筷子编号（同自身序号）*/

    /*
     * 筷子编号约定：哲学家 i（0-based）左侧筷子为 (i+N-1)%N，右侧筷子为 i。
     * 与原项目保持一致：chopstick[left] 和 chopstick[right=index]。
     */

    for (int t = 0; t < thread_arg->times; t++) {

        /* ---- THINKING ---- */
        {
            char msg[64];
            sprintf(msg, "P%d 正在思考", id);
            dp_show_state(msg);
        }
        random_sleep(1, 3);

        /* ---- 申请进入就餐 ---- */
        sem_wait(&dp_mutex);                    /* P(mutex) */
        dp_state[index] = HUNGRY;
        {
            char msg[64];
            sprintf(msg, "P%d 饥饿，请求筷子", id);
            dp_show_state(msg);
        }
        test(index);
        sem_post(&dp_mutex);                    /* V(mutex) */

        sem_wait(&dp_self[index]);              /* P(self[i])：若未能进入 EATING 则阻塞 */

        /* ---- 已获准就餐：记录筷子占用（仅用于显示）---- */
        pthread_mutex_lock(&dp_print_mutex);
        dp_chopstick_owner[left]  = id;
        dp_chopstick_owner[right] = id;
        pthread_mutex_unlock(&dp_print_mutex);

        {
            char msg[64];
            sprintf(msg, "P%d 获得筷子 C%d 和 C%d，开始就餐",
                    id, left + 1, right + 1);
            dp_show_state(msg);
        }
        random_sleep(2, 4);

        /* ---- 放下筷子（仅用于显示）---- */
        pthread_mutex_lock(&dp_print_mutex);
        dp_chopstick_owner[left]  = 0;
        dp_chopstick_owner[right] = 0;
        pthread_mutex_unlock(&dp_print_mutex);

        {
            char msg[64];
            sprintf(msg, "P%d 结束就餐，放下筷子 C%d 和 C%d",
                    id, left + 1, right + 1);
            dp_show_state(msg);
        }

        /* ---- 恢复 THINKING，唤醒可能阻塞的邻居 ---- */
        sem_wait(&dp_mutex);                    /* P(mutex) */
        dp_state[index] = THINKING;
        test((index + dp_total - 1) % dp_total);   /* test(left)  */
        test((index + 1) % dp_total);              /* test(right) */
        sem_post(&dp_mutex);                    /* V(mutex) */
    }

    return NULL;
}

/* ---------- 对外接口 ---------- */
void run_dining_philosophers(void)
{
    print_line();
    printf("哲学家就餐问题（Dijkstra 状态数组方案）\n");
    print_line();

    dp_total   = read_int("请输入哲学家数量，建议为 5：", 2, MAX_THREADS);
    int eat_times = read_int("请输入每个哲学家就餐次数：", 1, 10);

    pthread_t      philosophers[MAX_THREADS];
    PhilosopherArg args[MAX_THREADS];

    /* 初始化信号量和状态 */
    sem_init(&dp_mutex, 0, 1);                  /* mutex 初值 1 */
    for (int i = 0; i < dp_total; i++) {
        dp_state[i]         = THINKING;
        dp_chopstick_owner[i] = 0;
        sem_init(&dp_self[i], 0, 0);            /* self[i] 初值 0 */
    }

    /* 创建线程 */
    for (int i = 0; i < dp_total; i++) {
        args[i].id    = i + 1;
        args[i].times = eat_times;
        pthread_create(&philosophers[i], NULL, philosopher_thread, &args[i]);
    }

    /* 等待所有线程结束 */
    for (int i = 0; i < dp_total; i++) {
        pthread_join(philosophers[i], NULL);
    }

    /* 销毁信号量 */
    sem_destroy(&dp_mutex);
    for (int i = 0; i < dp_total; i++) {
        sem_destroy(&dp_self[i]);
    }

    printf("哲学家就餐问题运行结束。\n");
}
