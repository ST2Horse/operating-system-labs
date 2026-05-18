#include "pv_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void random_sleep(int min_seconds, int max_seconds) {
    if (max_seconds < min_seconds) {
        max_seconds = min_seconds;
    }
    int seconds = rand() % (max_seconds - min_seconds + 1) + min_seconds;
    sleep(seconds);
}

int read_int(const char *prompt, int min_value, int max_value) {
    int value;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &value) == 1 && value >= min_value && value <= max_value) {
            return value;
        }

        printf("输入无效，请输入 %d 到 %d 之间的整数。\n", min_value, max_value);
        while (getchar() != '\n') {
        }
    }
}

void print_line(void) {
    printf("------------------------------------------------------------\n");
}
