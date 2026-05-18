#include "pv_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct {
    int id;
    int count;
} PCThreadArg;

static sem_t pc_empty;
static sem_t pc_full;
static sem_t pc_mutex;
static pthread_mutex_t pc_print_mutex = PTHREAD_MUTEX_INITIALIZER;

static int *pc_buffer = NULL;
static int pc_buffer_size = 0;
static int pc_in = 0;
static int pc_out = 0;
static int pc_item_id = 1;
static int pc_current_count = 0;

static void pc_show_buffer(const char *message) {
    pthread_mutex_lock(&pc_print_mutex);
    printf("\n[生产者/消费者] %s\n", message);
    printf("缓冲区当前产品数：%d / %d\n", pc_current_count, pc_buffer_size);
    printf("缓冲区内容：");
    for (int i = 0; i < pc_buffer_size; i++) {
        if (pc_buffer[i] == 0) {
            printf("[空] ");
        } else {
            printf("[%d] ", pc_buffer[i]);
        }
    }
    printf("\n");
    pthread_mutex_unlock(&pc_print_mutex);
}

static void *producer_thread(void *arg) {
    PCThreadArg *thread_arg = (PCThreadArg *)arg;
    int id = thread_arg->id;
    int count = thread_arg->count;

    for (int i = 0; i < count; i++) {
        random_sleep(1, 3);

        pthread_mutex_lock(&pc_print_mutex);
        printf("生产者 P%d 请求生产产品。\n", id);
        pthread_mutex_unlock(&pc_print_mutex);

        sem_wait(&pc_empty);
        sem_wait(&pc_mutex);

        int item = pc_item_id++;
        pc_buffer[pc_in] = item;
        pc_in = (pc_in + 1) % pc_buffer_size;
        pc_current_count++;

        char message[128];
        snprintf(message, sizeof(message), "生产者 P%d 生产产品 %d。", id, item);
        pc_show_buffer(message);

        sem_post(&pc_mutex);
        sem_post(&pc_full);
    }

    pthread_mutex_lock(&pc_print_mutex);
    printf("生产者 P%d 完成所有生产任务。\n", id);
    pthread_mutex_unlock(&pc_print_mutex);
    return NULL;
}

static void *consumer_thread(void *arg) {
    PCThreadArg *thread_arg = (PCThreadArg *)arg;
    int id = thread_arg->id;
    int count = thread_arg->count;

    for (int i = 0; i < count; i++) {
        random_sleep(1, 3);

        pthread_mutex_lock(&pc_print_mutex);
        printf("消费者 C%d 请求消费产品。\n", id);
        pthread_mutex_unlock(&pc_print_mutex);

        sem_wait(&pc_full);
        sem_wait(&pc_mutex);

        int item = pc_buffer[pc_out];
        pc_buffer[pc_out] = 0;
        pc_out = (pc_out + 1) % pc_buffer_size;
        pc_current_count--;

        char message[128];
        snprintf(message, sizeof(message), "消费者 C%d 消费产品 %d。", id, item);
        pc_show_buffer(message);

        sem_post(&pc_mutex);
        sem_post(&pc_empty);
    }

    pthread_mutex_lock(&pc_print_mutex);
    printf("消费者 C%d 完成所有消费任务。\n", id);
    pthread_mutex_unlock(&pc_print_mutex);
    return NULL;
}

void run_producer_consumer(void) {
    print_line();
    printf("生产者 / 消费者问题\n");
    print_line();

    int producer_count = read_int("请输入生产者数量：", 1, MAX_THREADS);
    int consumer_count = read_int("请输入消费者数量：", 1, MAX_THREADS);
    pc_buffer_size = read_int("请输入缓冲区大小：", 1, MAX_BUFFER_SIZE);
    int produce_times = read_int("请输入每个生产者生产次数：", 1, 20);

    int total_items = producer_count * produce_times;
    int base_consume = total_items / consumer_count;
    int extra_consume = total_items % consumer_count;

    pthread_t producers[MAX_THREADS];
    pthread_t consumers[MAX_THREADS];
    PCThreadArg producer_args[MAX_THREADS];
    PCThreadArg consumer_args[MAX_THREADS];

    pc_buffer = (int *)calloc((size_t)pc_buffer_size, sizeof(int));
    pc_in = 0;
    pc_out = 0;
    pc_item_id = 1;
    pc_current_count = 0;

    sem_init(&pc_empty, 0, (unsigned int)pc_buffer_size);
    sem_init(&pc_full, 0, 0);
    sem_init(&pc_mutex, 0, 1);

    for (int i = 0; i < producer_count; i++) {
        producer_args[i].id = i + 1;
        producer_args[i].count = produce_times;
        pthread_create(&producers[i], NULL, producer_thread, &producer_args[i]);
    }

    for (int i = 0; i < consumer_count; i++) {
        consumer_args[i].id = i + 1;
        consumer_args[i].count = base_consume + (i < extra_consume ? 1 : 0);
        pthread_create(&consumers[i], NULL, consumer_thread, &consumer_args[i]);
    }

    for (int i = 0; i < producer_count; i++) {
        pthread_join(producers[i], NULL);
    }
    for (int i = 0; i < consumer_count; i++) {
        pthread_join(consumers[i], NULL);
    }

    sem_destroy(&pc_empty);
    sem_destroy(&pc_full);
    sem_destroy(&pc_mutex);
    free(pc_buffer);
    pc_buffer = NULL;

    printf("生产者 / 消费者问题运行结束。\n");
}
