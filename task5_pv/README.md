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

---

### 哲学家就餐问题算法（Dijkstra 状态数组方案）

本实现采用教材第四章 PPT 第 103 页起介绍的 Dijkstra 状态数组方案，**不使用**服务员/房间最多 4 人/`count=4` 的限制方案。

核心数据结构：
- `state[0..N-1]`：每个哲学家状态，取值 `THINKING` / `HUNGRY` / `EATING`
- `self[0..N-1]` ：每个哲学家的私有信号量，初值 0
- `mutex`         ：保护 `state` 数组的互斥信号量，初值 1

`test(i)` 函数：在 `mutex` 保护下，若 `state[i]==HUNGRY` 且左右邻居均不是 `EATING`，则令 `state[i]=EATING` 并 `V(self[i])`。

哲学家循环伪代码：
```
think
P(mutex); state[i]=HUNGRY; test(i); V(mutex)
P(self[i])          // 未能立即就餐则阻塞
取左右筷子 / 打印
eat
放左右筷子 / 打印
P(mutex); state[i]=THINKING; test(left); test(right); V(mutex)
```

---

### 吸烟者问题算法（AND 型信号量 / SP+SV 方案）

本实现采用教材第四章 PPT 第 117-123 页介绍的 AND 型信号量方案，
**不使用**传统 `P(one); P(other)` 连续取两种材料的方案（PPT 明确指出该写法存在死锁风险）。

#### 问题描述

| 角色 | 自有材料 | 需要材料 |
|------|---------|---------|
| 供应者 X | - | 提供 tobacco + match |
| 供应者 Y | - | 提供 match + wrapper |
| 供应者 Z | - | 提供 wrapper + tobacco |
| 吸烟者 A | tobacco | match + wrapper |
| 吸烟者 B | match | wrapper + tobacco |
| 吸烟者 C | wrapper | tobacco + match |

一次只能一个供应者供应；供应者必须等待上次材料被完全消费后才能继续投放。

#### 共享信号量

| 信号量 | 初值 | 含义 |
|-------|------|------|
| `s`   | 1    | 控制同一时刻只有一个供应者放材料 |
| `t`   | 0    | 桌上 tobacco 数量 |
| `m`   | 0    | 桌上 match 数量 |
| `w`   | 0    | 桌上 wrapper 数量 |

#### PPT 伪代码

供应者：
```
X: loop  P(s); SV(t,1; m,1);  endloop
Y: loop  P(s); SV(m,1; w,1);  endloop
Z: loop  P(s); SV(w,1; t,1);  endloop
```

吸烟者：
```
A: loop  SP(m,1,1; w,1,1);  smoke;  V(s);  endloop
B: loop  SP(w,1,1; t,1,1);  smoke;  V(s);  endloop
C: loop  SP(t,1,1; m,1,1);  smoke;  V(s);  endloop
```

#### SP / SV 语义

- **`SP(S1,t1,d1; S2,t2,d2)`**：仅当 `S1 >= t1` 且 `S2 >= t2` 时，
  原子地执行 `S1 -= d1; S2 -= d2`；否则阻塞，被唤醒后重新检查所有条件（无虚假扣减）。
- **`SV(S1,d1; S2,d2)`**：原子地执行 `S1 += d1; S2 += d2`，
  并广播唤醒所有等待者重新检查条件。

#### 实现方式

用一把全局 `pthread_mutex_t` 保护所有整型计数器（`s, t, m, w`），
用一个 `pthread_cond_t` 条件变量实现"等待并重新检查"。
整个"检查全部条件 + 同时扣减"在同一把 mutex 内完成，不可分割，
满足 AND 型信号量的原子性语义，杜绝死锁。
