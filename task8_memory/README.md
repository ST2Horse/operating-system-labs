# Task 8: 内存管理模拟实验

本实验使用 Python Tkinter 实现桌面图形界面，不是网页形式。

## 功能

1. 静态等长分区分配方法：
   - 字位映象图
   - 空闲页面表
   - 空闲页面链
2. 动态异长分区分配方法：
   - 最先适应算法
   - 下次适应算法
   - 最佳适应算法
   - 最坏适应算法
3. 反置页表方法页式内存管理：
   - 可选择内存物理空间大小：256 MB 或 512 MB
   - 可选择页框大小：1 KB、2 KB 或 4 KB
   - 计算反置页表项数和表占用空间
   - 随机生成多个进程二元组
   - 使用 `Hash(pid, p) = pid * 页框大小 + p`
   - 采用顺序探测处理冲突
   - 随机生成 16 位逻辑地址并转换为物理地址

## 运行

```bash
python3 gui_memory_lab.py
```

也可以使用：

```bash
make
```

或：

```bash
make gui
```

## 环境说明

图形界面使用 Python 标准库 Tkinter。若在 WSL 中运行，需要 WSLg 或可用的 X Server。

如果提示缺少 Tkinter，可安装：

```bash
sudo apt install python3-tk
```

## 清理

```bash
make clean
```
