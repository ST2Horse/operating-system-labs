# Task 5: 信号量与 P/V 操作实验

本项目使用 C 语言、pthread 线程和 POSIX 信号量实现四个经典同步问题：

1. 生产者 / 消费者问题
2. 写者优先读者 / 写者问题
3. 哲学家就餐问题
4. 吸烟者问题

## 运行环境

推荐使用 Linux、Ubuntu、WSL Ubuntu。

## 编译

```bash
make
```

## 运行

```bash
./pv_lab
```

## 清理

```bash
make clean
```

## 说明

程序通过菜单选择不同实验模块。每个模块都使用 `sem_wait()` 表示 P 操作，使用 `sem_post()` 表示 V 操作，并通过随机延时模拟进程运行过程。
