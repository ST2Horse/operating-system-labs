#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define KB 1024U
#define MB (1024ULL * 1024ULL)

#define MAX_FIXED_FRAMES 64
#define MAX_JOBS 10
#define MAX_PARTITIONS 128
#define MAX_PROCESSES 16
#define MAX_DISPLAY_ENTRIES 200
#define INVERTED_ENTRY_BYTES (sizeof(int) * 4)

typedef struct {
    int pid;
    uint32_t size_kb;
    int need_units;
    int allocated_units[MAX_FIXED_FRAMES];
    int allocated_count;
    int success;
} MemoryJob;

typedef struct {
    int start_kb;
    int size_kb;
    int pid;
    int free;
} Partition;

typedef struct {
    int pid;
    uint32_t logical_size;
    uint32_t page_count;
} Process;

typedef struct {
    int pid;
    uint32_t logical_page;
    uint32_t conflict_count;
    int occupied;
} InvertedPageEntry;

typedef enum {
    FIRST_FIT,
    NEXT_FIT,
    BEST_FIT,
    WORST_FIT
} DynamicAlgorithm;

static int read_int(const char *prompt, int min_value, int max_value) {
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

static uint32_t random_u32(uint32_t min_value, uint32_t max_value) {
    if (max_value <= min_value) {
        return min_value;
    }

    return (uint32_t)(rand() % (int)(max_value - min_value + 1)) + min_value;
}

static int ceil_div_int(int value, int divisor) {
    return (value + divisor - 1) / divisor;
}

static uint32_t ceil_div_u32(uint32_t value, uint32_t divisor) {
    return (value + divisor - 1) / divisor;
}

static void print_line(void) {
    printf("------------------------------------------------------------\n");
}

static void wait_enter(void) {
    printf("\n按 Enter 返回菜单...");
    while (getchar() != '\n') {
    }
    getchar();
}

static void reset_jobs(MemoryJob jobs[], int job_count) {
    for (int i = 0; i < job_count; i++) {
        jobs[i].allocated_count = 0;
        jobs[i].success = 0;
        for (int j = 0; j < MAX_FIXED_FRAMES; j++) {
            jobs[i].allocated_units[j] = -1;
        }
    }
}

static void generate_jobs(MemoryJob jobs[], int job_count, int min_kb, int max_kb) {
    for (int i = 0; i < job_count; i++) {
        jobs[i].pid = 100 + i + 1;
        jobs[i].size_kb = random_u32((uint32_t)min_kb, (uint32_t)max_kb);
        jobs[i].need_units = 0;
        jobs[i].allocated_count = 0;
        jobs[i].success = 0;
    }
}

static void print_jobs(const MemoryJob jobs[], int job_count, const char *unit_name) {
    printf("%-10s %-14s %-12s\n", "进程号", "大小(KB)", unit_name);
    for (int i = 0; i < job_count; i++) {
        printf("P%-9d %-14u %-12d\n", jobs[i].pid, jobs[i].size_kb, jobs[i].need_units);
    }
}

static void print_dynamic_jobs(const MemoryJob jobs[], int job_count) {
    printf("%-10s %-14s\n", "进程号", "请求大小(KB)");
    for (int i = 0; i < job_count; i++) {
        printf("P%-9d %-14u\n", jobs[i].pid, jobs[i].size_kb);
    }
}

static void show_allocated_units(const MemoryJob *job) {
    if (!job->success) {
        printf("分配失败");
        return;
    }

    for (int i = 0; i < job->allocated_count; i++) {
        printf("%d", job->allocated_units[i]);
        if (i + 1 < job->allocated_count) {
            printf(",");
        }
    }
}

static void show_bitmap(const int bitmap[], int frame_count) {
    printf("字位映象图（0=空闲，1=占用）：\n");
    for (int i = 0; i < frame_count; i++) {
        printf("%d", bitmap[i]);
        if ((i + 1) % 8 == 0 || i + 1 == frame_count) {
            printf("\n");
        } else {
            printf(" ");
        }
    }
}

static int allocate_by_bitmap(int bitmap[], int frame_count, MemoryJob *job) {
    int found = 0;

    for (int i = 0; i < frame_count && found < job->need_units; i++) {
        if (bitmap[i] == 0) {
            job->allocated_units[found++] = i;
        }
    }

    if (found < job->need_units) {
        return 0;
    }

    for (int i = 0; i < found; i++) {
        bitmap[job->allocated_units[i]] = 1;
    }
    job->allocated_count = found;
    job->success = 1;
    return 1;
}

static void release_by_bitmap(int bitmap[], MemoryJob *job) {
    if (!job->success) {
        return;
    }

    for (int i = 0; i < job->allocated_count; i++) {
        bitmap[job->allocated_units[i]] = 0;
    }
    job->success = 0;
    job->allocated_count = 0;
}

static void run_bitmap_method(MemoryJob source_jobs[], int job_count, int frame_count) {
    int bitmap[MAX_FIXED_FRAMES] = {0};
    MemoryJob jobs[MAX_JOBS] = {0};

    for (int i = 0; i < job_count; i++) {
        jobs[i] = source_jobs[i];
    }
    reset_jobs(jobs, job_count);

    print_line();
    printf("静态等长分区：字位映象图法\n");
    for (int i = 0; i < job_count; i++) {
        printf("P%d 申请 %d 个页框：", jobs[i].pid, jobs[i].need_units);
        if (allocate_by_bitmap(bitmap, frame_count, &jobs[i])) {
            printf("成功，页框号：");
            show_allocated_units(&jobs[i]);
            printf("\n");
        } else {
            printf("失败，空闲页框不足。\n");
        }
    }
    show_bitmap(bitmap, frame_count);

    for (int i = 0; i < job_count; i++) {
        if (jobs[i].success) {
            printf("释放 P%d 后：\n", jobs[i].pid);
            release_by_bitmap(bitmap, &jobs[i]);
            show_bitmap(bitmap, frame_count);
            break;
        }
    }
}

static void show_free_page_table(const int free_table[], int free_count) {
    printf("空闲页面表（共 %d 项）：", free_count);
    if (free_count == 0) {
        printf("空\n");
        return;
    }

    for (int i = 0; i < free_count; i++) {
        printf("%d", free_table[i]);
        if (i + 1 < free_count) {
            printf(",");
        }
    }
    printf("\n");
}

static int allocate_by_free_table(int free_table[], int *free_count, MemoryJob *job) {
    if (*free_count < job->need_units) {
        return 0;
    }

    for (int i = 0; i < job->need_units; i++) {
        job->allocated_units[i] = free_table[i];
    }

    for (int i = job->need_units; i < *free_count; i++) {
        free_table[i - job->need_units] = free_table[i];
    }

    *free_count -= job->need_units;
    job->allocated_count = job->need_units;
    job->success = 1;
    return 1;
}

static void release_by_free_table(int free_table[], int *free_count, MemoryJob *job) {
    if (!job->success) {
        return;
    }

    for (int i = 0; i < job->allocated_count; i++) {
        free_table[*free_count] = job->allocated_units[i];
        (*free_count)++;
    }
    job->success = 0;
    job->allocated_count = 0;
}

static void run_free_page_table_method(MemoryJob source_jobs[], int job_count, int frame_count) {
    int free_table[MAX_FIXED_FRAMES];
    int free_count = frame_count;
    MemoryJob jobs[MAX_JOBS] = {0};

    for (int i = 0; i < frame_count; i++) {
        free_table[i] = i;
    }
    for (int i = 0; i < job_count; i++) {
        jobs[i] = source_jobs[i];
    }
    reset_jobs(jobs, job_count);

    print_line();
    printf("静态等长分区：空闲页面表法\n");
    for (int i = 0; i < job_count; i++) {
        printf("P%d 申请 %d 个页框：", jobs[i].pid, jobs[i].need_units);
        if (allocate_by_free_table(free_table, &free_count, &jobs[i])) {
            printf("成功，页框号：");
            show_allocated_units(&jobs[i]);
            printf("\n");
        } else {
            printf("失败，空闲页面表项不足。\n");
        }
    }
    show_free_page_table(free_table, free_count);

    for (int i = 0; i < job_count; i++) {
        if (jobs[i].success) {
            printf("释放 P%d 后：\n", jobs[i].pid);
            release_by_free_table(free_table, &free_count, &jobs[i]);
            show_free_page_table(free_table, free_count);
            break;
        }
    }
}

static void show_free_chain(const int next[], int head) {
    int current = head;
    int printed = 0;

    printf("空闲页面链：");
    if (current == -1) {
        printf("空\n");
        return;
    }

    while (current != -1 && printed < MAX_FIXED_FRAMES) {
        printf("%d", current);
        current = next[current];
        printed++;
        if (current != -1) {
            printf(" -> ");
        }
    }
    printf("\n");
}

static int allocate_by_free_chain(int next[], int *head, MemoryJob *job) {
    int current = *head;
    int available = 0;

    while (current != -1) {
        available++;
        current = next[current];
    }
    if (available < job->need_units) {
        return 0;
    }

    for (int i = 0; i < job->need_units; i++) {
        int frame = *head;
        job->allocated_units[i] = frame;
        *head = next[frame];
        next[frame] = -1;
    }

    job->allocated_count = job->need_units;
    job->success = 1;
    return 1;
}

static void release_by_free_chain(int next[], int *head, MemoryJob *job) {
    if (!job->success) {
        return;
    }

    for (int i = 0; i < job->allocated_count; i++) {
        int frame = job->allocated_units[i];
        next[frame] = *head;
        *head = frame;
    }
    job->success = 0;
    job->allocated_count = 0;
}

static void run_free_page_chain_method(MemoryJob source_jobs[], int job_count, int frame_count) {
    int next[MAX_FIXED_FRAMES];
    int head = 0;
    MemoryJob jobs[MAX_JOBS] = {0};

    for (int i = 0; i < frame_count; i++) {
        next[i] = (i + 1 < frame_count) ? i + 1 : -1;
    }
    for (int i = 0; i < job_count; i++) {
        jobs[i] = source_jobs[i];
    }
    reset_jobs(jobs, job_count);

    print_line();
    printf("静态等长分区：空闲页面链法\n");
    for (int i = 0; i < job_count; i++) {
        printf("P%d 申请 %d 个页框：", jobs[i].pid, jobs[i].need_units);
        if (allocate_by_free_chain(next, &head, &jobs[i])) {
            printf("成功，页框号：");
            show_allocated_units(&jobs[i]);
            printf("\n");
        } else {
            printf("失败，空闲页面链长度不足。\n");
        }
    }
    show_free_chain(next, head);

    for (int i = 0; i < job_count; i++) {
        if (jobs[i].success) {
            printf("释放 P%d 后：\n", jobs[i].pid);
            release_by_free_chain(next, &head, &jobs[i]);
            show_free_chain(next, head);
            break;
        }
    }
}

static void run_static_fixed_partition(void) {
    MemoryJob jobs[MAX_JOBS] = {0};

    print_line();
    printf("静态等长分区分配模拟\n");
    print_line();
    int frame_count = read_int("请输入等长分区/页框个数（8-64）：", 8, MAX_FIXED_FRAMES);
    int frame_size_kb = read_int("请输入每个分区/页框大小 KB（1-64）：", 1, 64);
    int job_count = read_int("请输入随机产生的进程个数（4-10）：", 4, MAX_JOBS);

    generate_jobs(jobs, job_count, frame_size_kb, frame_size_kb * 4);
    for (int i = 0; i < job_count; i++) {
        jobs[i].need_units = ceil_div_int((int)jobs[i].size_kb, frame_size_kb);
    }

    printf("内存总大小：%d KB，页框大小：%d KB，页框数：%d。\n",
           frame_count * frame_size_kb,
           frame_size_kb,
           frame_count);
    print_jobs(jobs, job_count, "所需页框数");

    run_bitmap_method(jobs, job_count, frame_count);
    run_free_page_table_method(jobs, job_count, frame_count);
    run_free_page_chain_method(jobs, job_count, frame_count);
}

static const char *algorithm_name(DynamicAlgorithm algorithm) {
    switch (algorithm) {
        case FIRST_FIT:
            return "最先适应算法";
        case NEXT_FIT:
            return "下次适应算法";
        case BEST_FIT:
            return "最佳适应算法";
        case WORST_FIT:
            return "最坏适应算法";
        default:
            return "未知算法";
    }
}

static void print_partitions(const Partition partitions[], int count) {
    printf("%-8s %-12s %-12s %-10s\n", "序号", "起址(KB)", "大小(KB)", "状态");
    for (int i = 0; i < count; i++) {
        if (partitions[i].free) {
            printf("%-8d %-12d %-12d %-10s\n",
                   i,
                   partitions[i].start_kb,
                   partitions[i].size_kb,
                   "空闲");
        } else {
            printf("%-8d %-12d %-12d P%-9d\n",
                   i,
                   partitions[i].start_kb,
                   partitions[i].size_kb,
                   partitions[i].pid);
        }
    }
}

static int find_partition(const Partition partitions[], int count, int request_kb, DynamicAlgorithm algorithm, int rover) {
    int selected = -1;

    if (algorithm == FIRST_FIT) {
        for (int i = 0; i < count; i++) {
            if (partitions[i].free && partitions[i].size_kb >= request_kb) {
                return i;
            }
        }
    } else if (algorithm == NEXT_FIT) {
        for (int step = 0; step < count; step++) {
            int i = (rover + step) % count;
            if (partitions[i].free && partitions[i].size_kb >= request_kb) {
                return i;
            }
        }
    } else if (algorithm == BEST_FIT) {
        int best_size = INT32_MAX;
        for (int i = 0; i < count; i++) {
            if (partitions[i].free && partitions[i].size_kb >= request_kb && partitions[i].size_kb < best_size) {
                selected = i;
                best_size = partitions[i].size_kb;
            }
        }
    } else if (algorithm == WORST_FIT) {
        int worst_size = -1;
        for (int i = 0; i < count; i++) {
            if (partitions[i].free && partitions[i].size_kb >= request_kb && partitions[i].size_kb > worst_size) {
                selected = i;
                worst_size = partitions[i].size_kb;
            }
        }
    }

    return selected;
}

static int allocate_dynamic_partition(Partition partitions[],
                                      int *count,
                                      int *rover,
                                      int pid,
                                      int request_kb,
                                      DynamicAlgorithm algorithm) {
    int index = find_partition(partitions, *count, request_kb, algorithm, *rover);

    if (index == -1) {
        return 0;
    }

    if (partitions[index].size_kb == request_kb) {
        partitions[index].pid = pid;
        partitions[index].free = 0;
    } else {
        if (*count >= MAX_PARTITIONS) {
            return 0;
        }

        for (int i = *count; i > index + 1; i--) {
            partitions[i] = partitions[i - 1];
        }

        partitions[index + 1].start_kb = partitions[index].start_kb + request_kb;
        partitions[index + 1].size_kb = partitions[index].size_kb - request_kb;
        partitions[index + 1].pid = -1;
        partitions[index + 1].free = 1;

        partitions[index].size_kb = request_kb;
        partitions[index].pid = pid;
        partitions[index].free = 0;
        (*count)++;
    }

    *rover = (*count == 0) ? 0 : (index + 1) % *count;
    return 1;
}

static void merge_free_partitions(Partition partitions[], int *count) {
    for (int i = 0; i + 1 < *count;) {
        if (partitions[i].free && partitions[i + 1].free) {
            partitions[i].size_kb += partitions[i + 1].size_kb;
            for (int j = i + 1; j + 1 < *count; j++) {
                partitions[j] = partitions[j + 1];
            }
            (*count)--;
        } else {
            i++;
        }
    }
}

static int release_dynamic_partition(Partition partitions[], int *count, int pid) {
    for (int i = 0; i < *count; i++) {
        if (!partitions[i].free && partitions[i].pid == pid) {
            partitions[i].pid = -1;
            partitions[i].free = 1;
            merge_free_partitions(partitions, count);
            return 1;
        }
    }

    return 0;
}

static void run_one_dynamic_algorithm(const MemoryJob source_jobs[],
                                      int job_count,
                                      int memory_size_kb,
                                      DynamicAlgorithm algorithm) {
    Partition partitions[MAX_PARTITIONS] = {{0}};
    int partition_count = 1;
    int rover = 0;
    int first_success_pid = -1;

    partitions[0].start_kb = 0;
    partitions[0].size_kb = memory_size_kb;
    partitions[0].pid = -1;
    partitions[0].free = 1;

    print_line();
    printf("动态异长分区：%s\n", algorithm_name(algorithm));
    print_partitions(partitions, partition_count);

    for (int i = 0; i < job_count; i++) {
        printf("P%d 申请 %u KB：", source_jobs[i].pid, source_jobs[i].size_kb);
        if (allocate_dynamic_partition(partitions,
                                       &partition_count,
                                       &rover,
                                       source_jobs[i].pid,
                                       (int)source_jobs[i].size_kb,
                                       algorithm)) {
            printf("成功。\n");
            if (first_success_pid == -1) {
                first_success_pid = source_jobs[i].pid;
            }
        } else {
            printf("失败，没有合适的空闲分区。\n");
        }
        print_partitions(partitions, partition_count);
    }

    if (first_success_pid != -1) {
        printf("释放第一个分配成功的进程 P%d，并合并相邻空闲区：\n", first_success_pid);
        release_dynamic_partition(partitions, &partition_count, first_success_pid);
        print_partitions(partitions, partition_count);
    }
}

static void run_dynamic_variable_partition(void) {
    MemoryJob jobs[MAX_JOBS] = {0};

    print_line();
    printf("动态异长分区分配模拟\n");
    print_line();

    int memory_size_kb = read_int("请输入内存总大小 KB（128-4096）：", 128, 4096);
    int job_count = read_int("请输入随机产生的进程个数（4-10）：", 4, MAX_JOBS);
    generate_jobs(jobs, job_count, 16, memory_size_kb / 2);

    printf("随机产生的进程如下：\n");
    print_dynamic_jobs(jobs, job_count);

    run_one_dynamic_algorithm(jobs, job_count, memory_size_kb, FIRST_FIT);
    run_one_dynamic_algorithm(jobs, job_count, memory_size_kb, NEXT_FIT);
    run_one_dynamic_algorithm(jobs, job_count, memory_size_kb, BEST_FIT);
    run_one_dynamic_algorithm(jobs, job_count, memory_size_kb, WORST_FIT);
}

static uint64_t choose_memory_size(void) {
    printf("请选择内存物理空间大小：\n");
    printf("1. 256 MB\n");
    printf("2. 512 MB\n");

    int choice = read_int("请输入选项：", 1, 2);
    return choice == 1 ? 256ULL * MB : 512ULL * MB;
}

static uint32_t choose_frame_size(void) {
    printf("请选择页框大小：\n");
    printf("1. 1 KB\n");
    printf("2. 2 KB\n");
    printf("3. 4 KB\n");

    int choice = read_int("请输入选项：", 1, 3);
    if (choice == 1) {
        return 1U * KB;
    }
    if (choice == 2) {
        return 2U * KB;
    }
    return 4U * KB;
}

static uint64_t hash_raw(int pid, uint32_t logical_page, uint32_t frame_size) {
    return (uint64_t)pid * frame_size + logical_page;
}

static uint64_t hash_index(int pid, uint32_t logical_page, uint32_t frame_size, uint64_t frame_count) {
    return hash_raw(pid, logical_page, frame_size) % frame_count;
}

static void generate_processes(Process processes[], int count, uint32_t frame_size, uint64_t frame_count) {
    uint64_t used_pages = 0;
    uint32_t max_pages_by_address = 65536U / frame_size;

    for (int i = 0; i < count; i++) {
        uint64_t remaining_processes = (uint64_t)(count - i - 1);
        uint64_t remaining_pages = frame_count - used_pages;
        uint64_t max_pages_for_this = remaining_pages - remaining_processes * 4ULL;

        if (max_pages_for_this > max_pages_by_address) {
            max_pages_for_this = max_pages_by_address;
        }

        uint32_t page_count = random_u32(4U, (uint32_t)max_pages_for_this);
        uint32_t min_size = (page_count - 1U) * frame_size + 1U;
        uint32_t max_size = page_count * frame_size;
        uint32_t logical_size = random_u32(min_size, max_size);

        processes[i].pid = 200 + i + 1;
        processes[i].logical_size = logical_size;
        processes[i].page_count = ceil_div_u32(logical_size, frame_size);
        used_pages += processes[i].page_count;
    }

    printf("进程二元组如下，总逻辑页面数 = %" PRIu64 "，物理页框数 = %" PRIu64 "。\n",
           used_pages, frame_count);
    printf("%-10s %-18s %-12s\n", "进程号", "逻辑空间大小", "逻辑页数");
    for (int i = 0; i < count; i++) {
        printf("P%-9d %-18u %-12u\n",
               processes[i].pid,
               processes[i].logical_size,
               processes[i].page_count);
    }
}

static int insert_page(InvertedPageEntry table[],
                       uint64_t frame_count,
                       uint32_t frame_size,
                       int pid,
                       uint32_t logical_page) {
    uint64_t index = hash_index(pid, logical_page, frame_size, frame_count);
    uint32_t conflicts = 0;

    for (uint64_t checked = 0; checked < frame_count; checked++) {
        InvertedPageEntry *entry = &table[index];
        if (!entry->occupied) {
            entry->pid = pid;
            entry->logical_page = logical_page;
            entry->conflict_count = conflicts;
            entry->occupied = 1;
            return 1;
        }

        conflicts++;
        index = (index + 1) % frame_count;
    }

    return 0;
}

static uint64_t build_inverted_table(InvertedPageEntry table[],
                                     uint64_t frame_count,
                                     uint32_t frame_size,
                                     const Process processes[],
                                     int process_count) {
    uint64_t occupied_count = 0;

    for (int i = 0; i < process_count; i++) {
        for (uint32_t page = 0; page < processes[i].page_count; page++) {
            if (!insert_page(table, frame_count, frame_size, processes[i].pid, page)) {
                printf("反置页表已满，无法插入 P%d 的逻辑页 %u。\n", processes[i].pid, page);
                return occupied_count;
            }
            occupied_count++;
        }
    }

    return occupied_count;
}

static void show_table_entries(const InvertedPageEntry table[], uint64_t frame_count, uint64_t occupied_count) {
    uint64_t shown = 0;

    printf("非空反置页表项如下：\n");
    printf("%-12s %-10s %-12s %-12s %-10s\n",
           "表项序号", "进程号", "逻辑页号", "冲突次数", "状态");

    for (uint64_t i = 0; i < frame_count; i++) {
        if (!table[i].occupied) {
            continue;
        }

        if (shown < MAX_DISPLAY_ENTRIES) {
            printf("%-12" PRIu64 " P%-9d %-12u %-12u %-10s\n",
                   i,
                   table[i].pid,
                   table[i].logical_page,
                   table[i].conflict_count,
                   "占用");
        }
        shown++;
    }

    if (occupied_count > MAX_DISPLAY_ENTRIES) {
        printf("表项较多，仅显示前 %d 个非空表项；实际非空表项数为 %" PRIu64 "。\n",
               MAX_DISPLAY_ENTRIES,
               occupied_count);
    }
}

static int find_page(const InvertedPageEntry table[],
                     uint64_t frame_count,
                     uint32_t frame_size,
                     int pid,
                     uint32_t logical_page,
                     uint64_t *frame_number) {
    uint64_t index = hash_index(pid, logical_page, frame_size, frame_count);

    for (uint64_t checked = 0; checked < frame_count; checked++) {
        const InvertedPageEntry *entry = &table[index];
        if (!entry->occupied) {
            return 0;
        }
        if (entry->pid == pid && entry->logical_page == logical_page) {
            *frame_number = index;
            return 1;
        }

        index = (index + 1) % frame_count;
    }

    return 0;
}

static void simulate_address_translation(const InvertedPageEntry table[],
                                         uint64_t frame_count,
                                         uint32_t frame_size,
                                         const Process processes[],
                                         int process_count) {
    int selected = rand() % process_count;
    const Process *process = &processes[selected];
    uint32_t logical_address = random_u32(0, process->logical_size - 1U);
    uint32_t logical_page = logical_address / frame_size;
    uint32_t offset = logical_address % frame_size;
    uint64_t frame_number = 0;

    print_line();
    printf("随机地址转换演示：\n");
    printf("选中进程：P%d，逻辑空间大小：%u 字节。\n", process->pid, process->logical_size);
    printf("16 位逻辑地址 L = 0x%04X。\n", logical_address);

    if (logical_address >= process->logical_size) {
        printf("地址检查失败：L 超出该进程逻辑空间。\n");
        return;
    }

    printf("地址检查通过：L 属于该进程逻辑空间。\n");
    printf("逻辑页号 = 0x%X，页内偏移 = 0x%X。\n", logical_page, offset);

    if (!find_page(table, frame_count, frame_size, process->pid, logical_page, &frame_number)) {
        printf("查表失败：未找到对应页框。\n");
        return;
    }

    uint64_t physical_address = frame_number * frame_size + offset;
    printf("对应页框号 = 0x%" PRIX64 "。\n", frame_number);
    printf("实际物理地址 = 0x%" PRIX64 "。\n", physical_address);
}

static void run_inverted_page_table(void) {
    print_line();
    printf("基于杂凑技术的反置页表页式内存管理模拟\n");
    print_line();

    uint64_t memory_size = choose_memory_size();
    uint32_t frame_size = choose_frame_size();
    uint64_t frame_count = memory_size / frame_size;
    uint64_t table_size = frame_count * (uint64_t)INVERTED_ENTRY_BYTES;

    print_line();
    printf("内存物理空间大小：%" PRIu64 " MB\n", memory_size / MB);
    printf("页框大小：%u KB\n", frame_size / KB);
    printf("反置页表表项个数 = ceil(内存大小 / 页框大小) = %" PRIu64 "\n", frame_count);
    printf("每个表项按 4 个 int 字段计算，占 %zu 字节。\n", (size_t)INVERTED_ENTRY_BYTES);
    printf("反置页表所占空间 = %" PRIu64 " 字节，约 %.2f MB。\n",
           table_size,
           (double)table_size / (double)MB);

    int process_count = read_int("请输入随机产生的进程个数（不少于 4）：", 4, MAX_PROCESSES);

    Process processes[MAX_PROCESSES];
    generate_processes(processes, process_count, frame_size, frame_count);

    InvertedPageEntry *table = calloc((size_t)frame_count, sizeof(InvertedPageEntry));
    if (table == NULL) {
        printf("内存分配失败，无法创建反置页表。\n");
        return;
    }

    print_line();
    printf("Hash(pid, p) = pid * 页框大小 + p，表项下标 = Hash(pid, p) %% 表项个数。\n");
    printf("冲突策略：顺序探测法。\n");

    uint64_t occupied_count = build_inverted_table(table, frame_count, frame_size, processes, process_count);
    show_table_entries(table, frame_count, occupied_count);
    simulate_address_translation(table, frame_count, frame_size, processes, process_count);

    free(table);
}

static void print_main_menu(void) {
    print_line();
    printf("第 8 题：内存管理模拟实现\n");
    print_line();
    printf("1. 静态等长分区：字位映象图、空闲页面表、空闲页面链\n");
    printf("2. 动态异长分区：最先适应、下次适应、最佳适应、最坏适应\n");
    printf("3. 反置页表方法页式内存管理（杂凑 + 顺序探测）\n");
    printf("4. 依次运行以上全部模块\n");
    printf("0. 退出\n");
}

int main(void) {
    srand((unsigned int)time(NULL));

    while (1) {
        print_main_menu();
        int choice = read_int("请选择实验模块：", 0, 4);

        switch (choice) {
            case 1:
                run_static_fixed_partition();
                wait_enter();
                break;
            case 2:
                run_dynamic_variable_partition();
                wait_enter();
                break;
            case 3:
                run_inverted_page_table();
                wait_enter();
                break;
            case 4:
                run_static_fixed_partition();
                run_dynamic_variable_partition();
                run_inverted_page_table();
                wait_enter();
                break;
            case 0:
                printf("程序结束。\n");
                return 0;
            default:
                break;
        }
    }
}
