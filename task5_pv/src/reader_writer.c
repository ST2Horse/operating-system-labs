#include "pv_common.h"

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct {
    int id;
    int times;
} RWThreadArg;

static sem_t rw_resource;
static sem_t rw_rmutex;
static sem_t rw_wmutex;
static sem_t rw_read_try;
static pthread_mutex_t rw_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t rw_print_mutex = PTHREAD_MUTEX_INITIALIZER;

static int rw_reader_count = 0;
static int rw_writer_count = 0;
static int rw_reader_waiting[MAX_THREADS + 1];
static int rw_reader_reading[MAX_THREADS + 1];
static int rw_writer_waiting[MAX_THREADS + 1];
static int rw_current_writer = 0;
static int rw_total_readers = 0;
static int rw_total_writers = 0;

static void rw_show_state(const char *message) {
    pthread_mutex_lock(&rw_print_mutex);
    printf("\n[读者/写者] %s\n", message);

    printf("正在读的读者：");
    int has_reader = 0;
    for (int i = 1; i <= rw_total_readers; i++) {
        if (rw_reader_reading[i]) {
            printf("R%d ", i);
            has_reader = 1;
        }
    }
    if (!has_reader) {
        printf("无");
    }
    printf("\n");

    printf("正在写的写者：");
    if (rw_current_writer == 0) {
        printf("无");
    } else {
        printf("W%d", rw_current_writer);
    }
    printf("\n");

    printf("等待读者队列：");
    int has_wait_reader = 0;
    for (int i = 1; i <= rw_total_readers; i++) {
        if (rw_reader_waiting[i]) {
            printf("R%d ", i);
            has_wait_reader = 1;
        }
    }
    if (!has_wait_reader) {
        printf("无");
    }
    printf("\n");

    printf("等待写者队列：");
    int has_wait_writer = 0;
    for (int i = 1; i <= rw_total_writers; i++) {
        if (rw_writer_waiting[i]) {
            printf("W%d ", i);
            has_wait_writer = 1;
        }
    }
    if (!has_wait_writer) {
        printf("无");
    }
    printf("\n");
    pthread_mutex_unlock(&rw_print_mutex);
}

static void *reader_thread(void *arg) {
    RWThreadArg *thread_arg = (RWThreadArg *)arg;
    int id = thread_arg->id;

    for (int i = 0; i < thread_arg->times; i++) {
        random_sleep(1, 3);

        pthread_mutex_lock(&rw_state_mutex);
        rw_reader_waiting[id] = 1;
        pthread_mutex_unlock(&rw_state_mutex);
        rw_show_state("读者请求读操作。若已有写者等待，后续读者需要等待。");

        sem_wait(&rw_read_try);
        sem_wait(&rw_rmutex);

        rw_reader_count++;
        if (rw_reader_count == 1) {
            sem_wait(&rw_resource);
        }

        pthread_mutex_lock(&rw_state_mutex);
        rw_reader_waiting[id] = 0;
        rw_reader_reading[id] = 1;
        pthread_mutex_unlock(&rw_state_mutex);

        sem_post(&rw_rmutex);
        sem_post(&rw_read_try);

        rw_show_state("读者开始读。多个读者可以同时读。");
        random_sleep(2, 4);

        sem_wait(&rw_rmutex);
        rw_reader_count--;
        pthread_mutex_lock(&rw_state_mutex);
        rw_reader_reading[id] = 0;
        pthread_mutex_unlock(&rw_state_mutex);

        if (rw_reader_count == 0) {
            sem_post(&rw_resource);
        }
        sem_post(&rw_rmutex);

        rw_show_state("读者结束读操作。");
    }

    return NULL;
}

static void *writer_thread(void *arg) {
    RWThreadArg *thread_arg = (RWThreadArg *)arg;
    int id = thread_arg->id;

    for (int i = 0; i < thread_arg->times; i++) {
        random_sleep(1, 3);

        pthread_mutex_lock(&rw_state_mutex);
        rw_writer_waiting[id] = 1;
        pthread_mutex_unlock(&rw_state_mutex);
        rw_show_state("写者请求写操作。写者到达后将阻止后续读者进入。");

        sem_wait(&rw_wmutex);
        rw_writer_count++;
        if (rw_writer_count == 1) {
            sem_wait(&rw_read_try);
        }
        sem_post(&rw_wmutex);

        sem_wait(&rw_resource);

        pthread_mutex_lock(&rw_state_mutex);
        rw_writer_waiting[id] = 0;
        rw_current_writer = id;
        pthread_mutex_unlock(&rw_state_mutex);
        rw_show_state("写者开始写。写者独占共享资源。");

        random_sleep(2, 4);

        pthread_mutex_lock(&rw_state_mutex);
        rw_current_writer = 0;
        pthread_mutex_unlock(&rw_state_mutex);
        rw_show_state("写者结束写操作。");

        sem_post(&rw_resource);

        sem_wait(&rw_wmutex);
        rw_writer_count--;
        if (rw_writer_count == 0) {
            sem_post(&rw_read_try);
        }
        sem_post(&rw_wmutex);
    }

    return NULL;
}

void run_reader_writer(void) {
    print_line();
    printf("写者优先读者 / 写者问题\n");
    print_line();

    rw_total_readers = read_int("请输入读者数量：", 1, MAX_THREADS);
    rw_total_writers = read_int("请输入写者数量：", 1, MAX_THREADS);
    int read_times = read_int("请输入每个读者读的次数：", 1, 10);
    int write_times = read_int("请输入每个写者写的次数：", 1, 10);

    pthread_t readers[MAX_THREADS];
    pthread_t writers[MAX_THREADS];
    RWThreadArg reader_args[MAX_THREADS];
    RWThreadArg writer_args[MAX_THREADS];

    rw_reader_count = 0;
    rw_writer_count = 0;
    rw_current_writer = 0;
    for (int i = 0; i <= MAX_THREADS; i++) {
        rw_reader_waiting[i] = 0;
        rw_reader_reading[i] = 0;
        rw_writer_waiting[i] = 0;
    }

    sem_init(&rw_resource, 0, 1);
    sem_init(&rw_rmutex, 0, 1);
    sem_init(&rw_wmutex, 0, 1);
    sem_init(&rw_read_try, 0, 1);

    for (int i = 0; i < rw_total_readers; i++) {
        reader_args[i].id = i + 1;
        reader_args[i].times = read_times;
        pthread_create(&readers[i], NULL, reader_thread, &reader_args[i]);
    }

    for (int i = 0; i < rw_total_writers; i++) {
        writer_args[i].id = i + 1;
        writer_args[i].times = write_times;
        pthread_create(&writers[i], NULL, writer_thread, &writer_args[i]);
    }

    for (int i = 0; i < rw_total_readers; i++) {
        pthread_join(readers[i], NULL);
    }
    for (int i = 0; i < rw_total_writers; i++) {
        pthread_join(writers[i], NULL);
    }

    sem_destroy(&rw_resource);
    sem_destroy(&rw_rmutex);
    sem_destroy(&rw_wmutex);
    sem_destroy(&rw_read_try);

    printf("读者 / 写者问题运行结束。\n");
}
