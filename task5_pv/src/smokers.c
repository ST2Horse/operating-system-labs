/*
 * 吸烟者问题 -- AND 型信号量方案（SP / SV）
 *
 * 算法要点（对应教材第四章 PPT 第 117-123 页）：
 *
 *  问题描述：
 *   - 3 个供应者 X / Y / Z，3 个吸烟者 A / B / C。
 *   - X 供应 tobacco + match  （t, m）。
 *   - Y 供应 match  + wrapper （m, w）。
 *   - Z 供应 wrapper + tobacco（w, t）。
 *   - A 自有 tobacco，需要 match + wrapper（m, w）。
 *   - B 自有 match，  需要 wrapper + tobacco（w, t）。
 *   - C 自有 wrapper，需要 tobacco + match  （t, m）。
 *   - 一次只能一个供应者供应；供应者必须等待上次材料被消费后才能继续。
 *
 *  共享信号量（计数器）：
 *   - t, w, m 初值 0（桌上三种材料的数量）
 *   - s        初值 1（控制同一时刻只有一个供应者放材料）
 *
 *  供应者伪代码：
 *   X: loop  P(s); SV(t,1; m,1);  endloop
 *   Y: loop  P(s); SV(m,1; w,1);  endloop
 *   Z: loop  P(s); SV(w,1; t,1);  endloop
 *
 *  吸烟者伪代码：
 *   A: loop  SP(m,1,1; w,1,1);  smoke;  V(s);  endloop
 *   B: loop  SP(w,1,1; t,1,1);  smoke;  V(s);  endloop
 *   C: loop  SP(t,1,1; m,1,1);  smoke;  V(s);  endloop
 *
 *  SP(S1,t1,d1; S2,t2,d2) 语义：
 *   仅当 S1 >= t1 且 S2 >= t2 时，原子地执行 S1 -= d1; S2 -= d2；
 *   否则阻塞，被唤醒后重新检查全部条件（无虚假扣减）。
 *   返回值：0 = 成功扣减；-1 = 因 sm_finished 而提前返回。
 *
 *  SV(S1,d1; S2,d2) 语义：
 *   原子地执行 S1 += d1; S2 += d2，并广播唤醒所有等待者重新检查条件。
 *
 *  SP / SV 实现：用一把全局 mutex 保护所有计数器，用 condition variable
 *  实现"等待并重新检查"，保证多资源的检查与扣减不可分割。
 */

#include "pv_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/* ======================================================================
 * AND 型信号量核心结构
 * ====================================================================== */

/*
 * 用一把 mutex 保护 4 个整型计数器（s, t, m, w），
 * 一个 condition variable 用于"等待直到条件满足"。
 * 所有 SP / SV 操作都在 mutex 保护下进行，保证原子性。
 */
static pthread_mutex_t and_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  and_cond  = PTHREAD_COND_INITIALIZER;

/* 信号量计数器（初值由 run_smokers 设置） */
static int cnt_s;   /* 供应者互斥，初值 1 */
static int cnt_t;   /* tobacco，初值 0    */
static int cnt_m;   /* match，初值 0      */
static int cnt_w;   /* wrapper，初值 0    */

/* 终止标志：所有供应者结束后置 1 */
static int sm_finished = 0;

/* 轮次计数 */
static int sm_rounds = 0;

static pthread_mutex_t sm_print_mutex = PTHREAD_MUTEX_INITIALIZER;

static void sm_print(const char *msg)
{
    pthread_mutex_lock(&sm_print_mutex);
    printf("[吸烟者问题] %s\n", msg);
    pthread_mutex_unlock(&sm_print_mutex);
}

/* ======================================================================
 * 基础 P / V（单资源）
 * ====================================================================== */

/*
 * P(counter)：等待 counter >= 1，然后原子地 --
 * 在 sm_finished 时也会退出（用于供应者的 P(s)）。
 */
static void and_P(int *counter)
{
    pthread_mutex_lock(&and_mutex);
    while (*counter < 1 && !sm_finished) {
        pthread_cond_wait(&and_cond, &and_mutex);
    }
    if (*counter >= 1) {
        *counter -= 1;
    }
    pthread_mutex_unlock(&and_mutex);
}

/*
 * V(counter)：原子地 ++，广播唤醒
 */
static void and_V(int *counter)
{
    pthread_mutex_lock(&and_mutex);
    *counter += 1;
    pthread_cond_broadcast(&and_cond);
    pthread_mutex_unlock(&and_mutex);
}

/* ======================================================================
 * SP / SV：AND 型信号量核心操作
 * ====================================================================== */

/*
 * SP(s1,t1,d1; s2,t2,d2) -- 同时 P 操作（AND 型）
 *
 *   仅当 s1 >= t1 且 s2 >= t2 时才同时原子扣减；
 *   否则调用 cond_wait 阻塞，被唤醒后重新检查所有条件（无虚假扣减）。
 *   若 sm_finished 为真则提前退出（返回 -1），否则返回 0。
 *
 *   关键点：整个"检查 + 扣减"在同一把 mutex 内完成，不可分割。
 */
static int SP(int *s1, int t1, int d1,
              int *s2, int t2, int d2)
{
    pthread_mutex_lock(&and_mutex);
    while (!(*s1 >= t1 && *s2 >= t2)) {
        if (sm_finished) {
            pthread_mutex_unlock(&and_mutex);
            return -1;          /* 终止信号，调用者应退出循环 */
        }
        pthread_cond_wait(&and_cond, &and_mutex);
    }
    /* 条件满足，原子扣减 */
    *s1 -= d1;
    *s2 -= d2;
    pthread_mutex_unlock(&and_mutex);
    return 0;
}

/*
 * SV(s1,d1; s2,d2) -- 同时 V 操作
 *
 *   原子地将 s1 += d1, s2 += d2，并广播唤醒所有等待者重新检查条件。
 *   广播保证：任何一个等待者的两个条件都能在被唤醒后重新评估。
 */
static void SV(int *s1, int d1,
               int *s2, int d2)
{
    pthread_mutex_lock(&and_mutex);
    *s1 += d1;
    *s2 += d2;
    pthread_cond_broadcast(&and_cond);
    pthread_mutex_unlock(&and_mutex);
}

/* ======================================================================
 * 供应者线程
 * ====================================================================== */

/*
 * 供应者 X：提供 tobacco(t) + match(m)
 *   X: loop  P(s); SV(t,1; m,1);  endloop
 */
static void *supplier_X(void *arg)
{
    (void)arg;
    char msg[128];
    for (int i = 0; i < sm_rounds; i++) {
        and_P(&cnt_s);                      /* P(s) */
        if (sm_finished) break;
        random_sleep(1, 2);
        snprintf(msg, sizeof(msg),
                 "供应者 X [第%d轮] 投放：tobacco + match", i + 1);
        sm_print(msg);
        SV(&cnt_t, 1, &cnt_m, 1);          /* SV(t,1; m,1) */
    }
    return NULL;
}

/*
 * 供应者 Y：提供 match(m) + wrapper(w)
 *   Y: loop  P(s); SV(m,1; w,1);  endloop
 */
static void *supplier_Y(void *arg)
{
    (void)arg;
    char msg[128];
    for (int i = 0; i < sm_rounds; i++) {
        and_P(&cnt_s);                      /* P(s) */
        if (sm_finished) break;
        random_sleep(1, 2);
        snprintf(msg, sizeof(msg),
                 "供应者 Y [第%d轮] 投放：match + wrapper", i + 1);
        sm_print(msg);
        SV(&cnt_m, 1, &cnt_w, 1);          /* SV(m,1; w,1) */
    }
    return NULL;
}

/*
 * 供应者 Z：提供 wrapper(w) + tobacco(t)
 *   Z: loop  P(s); SV(w,1; t,1);  endloop
 */
static void *supplier_Z(void *arg)
{
    (void)arg;
    char msg[128];
    for (int i = 0; i < sm_rounds; i++) {
        and_P(&cnt_s);                      /* P(s) */
        if (sm_finished) break;
        random_sleep(1, 2);
        snprintf(msg, sizeof(msg),
                 "供应者 Z [第%d轮] 投放：wrapper + tobacco", i + 1);
        sm_print(msg);
        SV(&cnt_w, 1, &cnt_t, 1);          /* SV(w,1; t,1) */
    }
    return NULL;
}

/* ======================================================================
 * 吸烟者线程
 * ====================================================================== */

/*
 * 吸烟者 A：自有 tobacco，需要 match + wrapper
 *   A: loop  SP(m,1,1; w,1,1);  smoke;  V(s);  endloop
 */
static void *smoker_A(void *arg)
{
    (void)arg;
    while (1) {
        int rc = SP(&cnt_m, 1, 1, &cnt_w, 1, 1);   /* SP(m,1,1; w,1,1) */
        if (rc < 0) break;                           /* sm_finished */
        sm_print("吸烟者 A（自有 tobacco）取得 match + wrapper，开始吸烟");
        random_sleep(2, 4);
        sm_print("吸烟者 A 吸烟结束，V(s) 通知供应者继续");
        and_V(&cnt_s);                               /* V(s) */
    }
    return NULL;
}

/*
 * 吸烟者 B：自有 match，需要 wrapper + tobacco
 *   B: loop  SP(w,1,1; t,1,1);  smoke;  V(s);  endloop
 */
static void *smoker_B(void *arg)
{
    (void)arg;
    while (1) {
        int rc = SP(&cnt_w, 1, 1, &cnt_t, 1, 1);   /* SP(w,1,1; t,1,1) */
        if (rc < 0) break;
        sm_print("吸烟者 B（自有 match）取得 wrapper + tobacco，开始吸烟");
        random_sleep(2, 4);
        sm_print("吸烟者 B 吸烟结束，V(s) 通知供应者继续");
        and_V(&cnt_s);
    }
    return NULL;
}

/*
 * 吸烟者 C：自有 wrapper，需要 tobacco + match
 *   C: loop  SP(t,1,1; m,1,1);  smoke;  V(s);  endloop
 */
static void *smoker_C(void *arg)
{
    (void)arg;
    while (1) {
        int rc = SP(&cnt_t, 1, 1, &cnt_m, 1, 1);   /* SP(t,1,1; m,1,1) */
        if (rc < 0) break;
        sm_print("吸烟者 C（自有 wrapper）取得 tobacco + match，开始吸烟");
        random_sleep(2, 4);
        sm_print("吸烟者 C 吸烟结束，V(s) 通知供应者继续");
        and_V(&cnt_s);
    }
    return NULL;
}

/* ======================================================================
 * 对外接口
 * ====================================================================== */

void run_smokers(void)
{
    print_line();
    printf("吸烟者问题（AND 型信号量 / SP+SV 方案）\n");
    print_line();
    printf("算法说明（对应 PPT 第 117-123 页）：\n");
    printf("  供应者 X/Y/Z 每次 P(s) 占用桌子，再用 SV 原子地放两种材料。\n");
    printf("  吸烟者 A/B/C 用 SP 同时等待两种材料，原子扣减后吸烟，最后 V(s)。\n");
    printf("  SP/SV 用 mutex+condvar 实现，多资源检查与扣减不可分割，无死锁。\n");
    printf("  对应关系：\n");
    printf("    X 投 t+m  -> C(自有w)消费；\n");
    printf("    Y 投 m+w  -> A(自有t)消费；\n");
    printf("    Z 投 w+t  -> B(自有m)消费。\n");
    print_line();

    sm_rounds = read_int("请输入每个供应者供应轮次（建议 3-10）：", 1, 30);

    /* 初始化计数器 */
    cnt_s = 1;
    cnt_t = 0;
    cnt_m = 0;
    cnt_w = 0;
    sm_finished = 0;

    pthread_t tid_X, tid_Y, tid_Z;
    pthread_t tid_A, tid_B, tid_C;

    /* 创建供应者线程 */
    pthread_create(&tid_X, NULL, supplier_X, NULL);
    pthread_create(&tid_Y, NULL, supplier_Y, NULL);
    pthread_create(&tid_Z, NULL, supplier_Z, NULL);

    /* 创建吸烟者线程 */
    pthread_create(&tid_A, NULL, smoker_A, NULL);
    pthread_create(&tid_B, NULL, smoker_B, NULL);
    pthread_create(&tid_C, NULL, smoker_C, NULL);

    /* 等待三个供应者全部完成 */
    pthread_join(tid_X, NULL);
    pthread_join(tid_Y, NULL);
    pthread_join(tid_Z, NULL);

    /*
     * 所有供应者已结束，置终止标志并广播，
     * 让仍在 SP 中阻塞的吸烟者线程退出循环。
     */
    pthread_mutex_lock(&and_mutex);
    sm_finished = 1;
    pthread_cond_broadcast(&and_cond);
    pthread_mutex_unlock(&and_mutex);

    pthread_join(tid_A, NULL);
    pthread_join(tid_B, NULL);
    pthread_join(tid_C, NULL);

    printf("吸烟者问题运行结束。\n");
}
