#ifndef PV_COMMON_H
#define PV_COMMON_H

#include <pthread.h>
#include <semaphore.h>

#define MAX_THREADS 50
#define MAX_BUFFER_SIZE 50

void random_sleep(int min_seconds, int max_seconds);
int read_int(const char *prompt, int min_value, int max_value);
void print_line(void);

void run_producer_consumer(void);
void run_reader_writer(void);
void run_dining_philosophers(void);
void run_smokers(void);

#endif
