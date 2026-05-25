#include "pv_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

static sem_t sm_agent;
static sem_t sm_tobacco;
static sem_t sm_paper;
static sem_t sm_match;
static pthread_mutex_t sm_print_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t sm_state_mutex = PTHREAD_MUTEX_INITIALIZER;

static int sm_rounds = 0;
static int sm_finished = 0;
static int sm_smoking = 0;
static const char *sm_table = "空";

static void sm_show_state(const char *message) {
    pthread_mutex_lock(&sm_print_mutex);
    printf("\n[吸烟者问题] %s\n", message);
    printf("桌上材料：%s\n", sm_table);
    printf("正在吸烟者：");
    if (sm_smoking == 0) {
        printf("无\n");
    } else if (sm_smoking == 1) {
        printf("拥有烟草的吸烟者 S1\n");
    } else if (sm_smoking == 2) {
        printf("拥有纸的吸烟者 S2\n");
    } else {
        printf("拥有火柴的吸烟者 S3\n");
    }
    pthread_mutex_unlock(&sm_print_mutex);
}

static void *agent_thread(void *arg) {
    (void)arg;

    for (int i = 0; i < sm_rounds; i++) {
        sem_wait(&sm_agent);
        random_sleep(1, 2);

        int choice = rand() % 3;
        pthread_mutex_lock(&sm_state_mutex);
        sm_smoking = 0;
        if (choice == 0) {
            sm_table = "纸 + 火柴";
        } else if (choice == 1) {
            sm_table = "烟草 + 火柴";
        } else {
            sm_table = "烟草 + 纸";
        }
        pthread_mutex_unlock(&sm_state_mutex);

        sm_show_state("代理者投放两种材料。");

        if (choice == 0) {
            sem_post(&sm_tobacco);
        } else if (choice == 1) {
            sem_post(&sm_paper);
        } else {
            sem_post(&sm_match);
        }
    }

    sem_wait(&sm_agent);
    pthread_mutex_lock(&sm_state_mutex);
    sm_finished = 1;
    pthread_mutex_unlock(&sm_state_mutex);

    sem_post(&sm_tobacco);
    sem_post(&sm_paper);
    sem_post(&sm_match);

    return NULL;
}

static void smoke(int smoker_id, const char *name) {
    pthread_mutex_lock(&sm_state_mutex);
    sm_smoking = smoker_id;
    sm_table = "空";
    pthread_mutex_unlock(&sm_state_mutex);

    char message[128];
    snprintf(message, sizeof(message), "%s 获得所需材料，开始吸烟。", name);
    sm_show_state(message);
    random_sleep(2, 4);

    pthread_mutex_lock(&sm_state_mutex);
    sm_smoking = 0;
    pthread_mutex_unlock(&sm_state_mutex);
    snprintf(message, sizeof(message), "%s 吸烟结束，通知代理者继续投放材料。", name);
    sm_show_state(message);

    sem_post(&sm_agent);
}

static void *smoker_tobacco_thread(void *arg) {
    (void)arg;
    while (1) {
        sem_wait(&sm_tobacco);
        if (sm_finished) {
            break;
        }
        smoke(1, "拥有烟草的吸烟者 S1");
    }
    return NULL;
}

static void *smoker_paper_thread(void *arg) {
    (void)arg;
    while (1) {
        sem_wait(&sm_paper);
        if (sm_finished) {
            break;
        }
        smoke(2, "拥有纸的吸烟者 S2");
    }
    return NULL;
}

static void *smoker_match_thread(void *arg) {
    (void)arg;
    while (1) {
        sem_wait(&sm_match);
        if (sm_finished) {
            break;
        }
        smoke(3, "拥有火柴的吸烟者 S3");
    }
    return NULL;
}

void run_smokers(void) {
    print_line();
    printf("吸烟者问题\n");
    print_line();

    sm_rounds = read_int("请输入代理者投放材料次数：", 1, 30);

    pthread_t agent;
    pthread_t smoker1;
    pthread_t smoker2;
    pthread_t smoker3;

    sm_finished = 0;
    sm_smoking = 0;
    sm_table = "空";

    sem_init(&sm_agent, 0, 1);
    sem_init(&sm_tobacco, 0, 0);
    sem_init(&sm_paper, 0, 0);
    sem_init(&sm_match, 0, 0);

    pthread_create(&agent, NULL, agent_thread, NULL);
    pthread_create(&smoker1, NULL, smoker_tobacco_thread, NULL);
    pthread_create(&smoker2, NULL, smoker_paper_thread, NULL);
    pthread_create(&smoker3, NULL, smoker_match_thread, NULL);

    pthread_join(agent, NULL);
    pthread_join(smoker1, NULL);
    pthread_join(smoker2, NULL);
    pthread_join(smoker3, NULL);

    sem_destroy(&sm_agent);
    sem_destroy(&sm_tobacco);
    sem_destroy(&sm_paper);
    sem_destroy(&sm_match);

    printf("吸烟者问题运行结束。\n");
}
