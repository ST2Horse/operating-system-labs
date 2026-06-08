#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define KB UINT32_C(1024)
#define MB UINT64_C(1048576)
#define MAX_PROCESSES 16
#define MAX_DISPLAY_ENTRIES 200
#define INVERTED_ENTRY_BYTES (sizeof(int) * 4)

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

static uint32_t ceil_div_u32(uint32_t value, uint32_t divisor) {
    return (value + divisor - 1) / divisor;
}

static uint64_t hash_raw(int pid, uint32_t logical_page, uint32_t frame_size) {
    return (uint64_t)pid * frame_size + logical_page;
}

static uint64_t hash_index(int pid, uint32_t logical_page, uint32_t frame_size, uint64_t frame_count) {
    return hash_raw(pid, logical_page, frame_size) % frame_count;
}

static void print_line(void) {
    printf("------------------------------------------------------------\n");
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

        processes[i].pid = 100 + i + 1;
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

int main(void) {
    srand((unsigned int)time(NULL));

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
    printf("反置页表表项个数 = 内存大小 / 页框大小 = %" PRIu64 "\n", frame_count);
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
        return 1;
    }

    print_line();
    printf("Hash(pid, p) = pid * 页框大小 + p，表项下标 = Hash(pid, p) %% 表项个数。\n");
    printf("冲突策略：顺序探测法。\n");

    uint64_t occupied_count = build_inverted_table(table, frame_count, frame_size, processes, process_count);
    show_table_entries(table, frame_count, occupied_count);
    simulate_address_translation(table, frame_count, frame_size, processes, process_count);

    free(table);
    print_line();
    printf("模拟结束。\n");

    return 0;
}
