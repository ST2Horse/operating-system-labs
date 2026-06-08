import math
import random
import sys
import tkinter as tk
from tkinter import ttk


KB = 1024
MB = 1024 * 1024
ENTRY_BYTES = 16

BG = "#eef2f7"
PANEL = "#ffffff"
INK = "#172033"
MUTED = "#64748b"
BORDER = "#d7dee8"
BLUE = "#2563eb"
CYAN = "#0891b2"
GREEN = "#16a34a"
AMBER = "#d97706"
RED = "#dc2626"
VIOLET = "#7c3aed"
SLATE = "#334155"


class MemoryLabApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("第 8 题 - 内存管理模拟实现")
        self.geometry("1280x820")
        self.minsize(1120, 720)
        self.configure(bg=BG)

        self.font_ui = ("Microsoft YaHei UI", 10)
        self.font_title = ("Microsoft YaHei UI", 18, "bold")
        self.font_subtitle = ("Microsoft YaHei UI", 11)
        self.font_section = ("Microsoft YaHei UI", 11, "bold")
        self.font_mono = ("Consolas", 10)

        self.configure_style()
        self.build_layout()

    def configure_style(self):
        style = ttk.Style(self)
        style.theme_use("clam")
        style.configure(".", font=self.font_ui, background=BG, foreground=INK)
        style.configure("TFrame", background=BG)
        style.configure("Panel.TFrame", background=PANEL)
        style.configure("Header.TFrame", background=SLATE)
        style.configure("TLabel", background=BG, foreground=INK)
        style.configure("Panel.TLabel", background=PANEL, foreground=INK)
        style.configure("Muted.TLabel", background=PANEL, foreground=MUTED)
        style.configure("Header.TLabel", background=SLATE, foreground="#ffffff")
        style.configure("TButton", padding=(12, 7), font=self.font_ui)
        style.map("TButton", background=[("active", "#dbeafe")])
        style.configure("Accent.TButton", background=BLUE, foreground="#ffffff")
        style.map("Accent.TButton", background=[("active", "#1d4ed8")])
        style.configure("TNotebook", background=BG, borderwidth=0)
        style.configure("TNotebook.Tab", padding=(18, 9), font=("Microsoft YaHei UI", 10, "bold"))
        style.map("TNotebook.Tab", background=[("selected", PANEL)], foreground=[("selected", BLUE)])
        style.configure("Treeview", rowheight=28, font=self.font_ui, background="#fbfdff", fieldbackground="#fbfdff", foreground=INK)
        style.configure("Treeview.Heading", font=("Microsoft YaHei UI", 10, "bold"), background="#e8eef7", foreground=INK)
        style.map("Treeview", background=[("selected", "#dbeafe")], foreground=[("selected", INK)])

    def build_layout(self):
        header = ttk.Frame(self, style="Header.TFrame")
        header.pack(fill="x")

        ttk.Label(header, text="操作系统实验 8", style="Header.TLabel", font=self.font_title).pack(side="left", padx=22, pady=(14, 3))
        ttk.Label(
            header,
            text="内存管理模拟：等长分区、异长分区、反置页表",
            style="Header.TLabel",
            font=self.font_subtitle,
        ).pack(side="left", padx=14, pady=(19, 3))

        self.notebook = ttk.Notebook(self)
        self.notebook.pack(fill="both", expand=True, padx=14, pady=14)

        self.static_tab = ttk.Frame(self.notebook)
        self.dynamic_tab = ttk.Frame(self.notebook)
        self.inverted_tab = ttk.Frame(self.notebook)
        self.notebook.add(self.static_tab, text="静态等长分区")
        self.notebook.add(self.dynamic_tab, text="动态异长分区")
        self.notebook.add(self.inverted_tab, text="反置页表")

        self.build_static_tab()
        self.build_dynamic_tab()
        self.build_inverted_tab()

    def panel(self, parent):
        frame = ttk.Frame(parent, style="Panel.TFrame")
        frame.configure(padding=12)
        return frame

    def section_label(self, parent, text):
        return ttk.Label(parent, text=text, style="Panel.TLabel", font=self.font_section)

    def add_spin(self, parent, label, variable, min_value, max_value, width=8):
        ttk.Label(parent, text=label, style="Panel.TLabel").pack(side="left", padx=(0, 5))
        spin = ttk.Spinbox(parent, from_=min_value, to=max_value, textvariable=variable, width=width)
        spin.pack(side="left", padx=(0, 14))
        return spin

    def make_tree(self, parent, columns, headings, widths):
        frame = ttk.Frame(parent, style="Panel.TFrame")
        frame.pack(fill="both", expand=True, pady=(8, 0))
        tree = ttk.Treeview(frame, columns=columns, show="headings", height=8)
        for column, heading, width in zip(columns, headings, widths):
            tree.heading(column, text=heading)
            tree.column(column, width=width, anchor="center", stretch=True)
        ybar = ttk.Scrollbar(frame, orient="vertical", command=tree.yview)
        tree.configure(yscrollcommand=ybar.set)
        tree.pack(side="left", fill="both", expand=True)
        ybar.pack(side="right", fill="y")
        return tree

    def clear_tree(self, tree):
        for item in tree.get_children():
            tree.delete(item)

    def build_static_tab(self):
        root = ttk.Frame(self.static_tab)
        root.pack(fill="both", expand=True)

        controls = self.panel(root)
        controls.pack(fill="x", pady=(0, 12))
        self.section_label(controls, "静态等长分区分配").pack(side="left", padx=(0, 18))
        self.static_frame_count = tk.IntVar(value=32)
        self.static_frame_size = tk.IntVar(value=8)
        self.static_job_count = tk.IntVar(value=6)
        self.add_spin(controls, "页框个数", self.static_frame_count, 8, 64)
        self.add_spin(controls, "页框大小(KB)", self.static_frame_size, 1, 64)
        self.add_spin(controls, "进程个数", self.static_job_count, 4, 10)
        ttk.Button(controls, text="重新模拟", style="Accent.TButton", command=self.run_static).pack(side="left")

        body = ttk.PanedWindow(root, orient="horizontal")
        body.pack(fill="both", expand=True)
        left = self.panel(body)
        right = self.panel(body)
        body.add(left, weight=1)
        body.add(right, weight=2)

        self.section_label(left, "随机进程").pack(anchor="w")
        self.static_jobs = self.make_tree(left, ("pid", "size", "need"), ("进程号", "大小(KB)", "所需页框"), (80, 100, 100))

        ttk.Separator(left).pack(fill="x", pady=12)
        self.section_label(left, "三种方法的分配结果").pack(anchor="w")
        self.static_result = self.make_tree(
            left,
            ("method", "pid", "need", "result", "frames"),
            ("方法", "进程", "页框", "结果", "页框号"),
            (105, 70, 60, 70, 180),
        )

        self.static_canvas = tk.Canvas(right, bg=PANEL, highlightthickness=0)
        self.static_canvas.pack(fill="both", expand=True)
        self.run_static()

    def build_dynamic_tab(self):
        root = ttk.Frame(self.dynamic_tab)
        root.pack(fill="both", expand=True)

        controls = self.panel(root)
        controls.pack(fill="x", pady=(0, 12))
        self.section_label(controls, "动态异长分区分配").pack(side="left", padx=(0, 18))
        self.dynamic_memory_size = tk.IntVar(value=512)
        self.dynamic_job_count = tk.IntVar(value=6)
        self.add_spin(controls, "内存大小(KB)", self.dynamic_memory_size, 128, 4096)
        self.add_spin(controls, "进程个数", self.dynamic_job_count, 4, 10)
        ttk.Button(controls, text="重新模拟", style="Accent.TButton", command=self.run_dynamic).pack(side="left")

        body = ttk.PanedWindow(root, orient="horizontal")
        body.pack(fill="both", expand=True)
        left = self.panel(body)
        right = self.panel(body)
        body.add(left, weight=1)
        body.add(right, weight=2)

        self.section_label(left, "随机进程").pack(anchor="w")
        self.dynamic_jobs = self.make_tree(left, ("pid", "size"), ("进程号", "请求大小(KB)"), (100, 130))
        ttk.Separator(left).pack(fill="x", pady=12)
        self.section_label(left, "操作日志").pack(anchor="w")
        self.dynamic_log = tk.Text(left, height=16, wrap="word", bg="#fbfdff", fg=INK, font=self.font_ui, relief="flat", padx=10, pady=10)
        self.dynamic_log.pack(fill="both", expand=True, pady=(8, 0))

        self.dynamic_canvas = tk.Canvas(right, bg=PANEL, highlightthickness=0)
        self.dynamic_canvas.pack(fill="both", expand=True)
        self.run_dynamic()

    def build_inverted_tab(self):
        root = ttk.Frame(self.inverted_tab)
        root.pack(fill="both", expand=True)

        controls = self.panel(root)
        controls.pack(fill="x", pady=(0, 12))
        self.section_label(controls, "反置页表页式内存管理").pack(side="left", padx=(0, 18))
        self.memory_choice = tk.StringVar(value="256 MB")
        self.frame_choice = tk.StringVar(value="4 KB")
        self.process_count = tk.IntVar(value=5)
        ttk.Label(controls, text="内存大小", style="Panel.TLabel").pack(side="left", padx=(0, 5))
        ttk.Combobox(controls, textvariable=self.memory_choice, values=("256 MB", "512 MB"), width=9, state="readonly").pack(side="left", padx=(0, 14))
        ttk.Label(controls, text="页框大小", style="Panel.TLabel").pack(side="left", padx=(0, 5))
        ttk.Combobox(controls, textvariable=self.frame_choice, values=("1 KB", "2 KB", "4 KB"), width=9, state="readonly").pack(side="left", padx=(0, 14))
        self.add_spin(controls, "进程个数", self.process_count, 4, 16)
        ttk.Button(controls, text="重新生成", style="Accent.TButton", command=self.run_inverted).pack(side="left")

        body = ttk.PanedWindow(root, orient="horizontal")
        body.pack(fill="both", expand=True)
        left = self.panel(body)
        right = self.panel(body)
        body.add(left, weight=1)
        body.add(right, weight=2)

        self.section_label(left, "内存与页表参数").pack(anchor="w")
        self.inverted_stats = ttk.Label(left, text="", style="Muted.TLabel", justify="left")
        self.inverted_stats.pack(anchor="w", pady=(8, 12))
        self.section_label(left, "进程二元组").pack(anchor="w")
        self.process_tree = self.make_tree(left, ("pid", "size", "pages"), ("进程号", "逻辑空间(B)", "逻辑页数"), (90, 120, 90))
        ttk.Separator(left).pack(fill="x", pady=12)
        self.section_label(left, "随机逻辑地址转换").pack(anchor="w")
        self.translation = ttk.Label(left, text="", style="Panel.TLabel", justify="left", font=self.font_mono)
        self.translation.pack(anchor="w", pady=(8, 0))

        self.section_label(right, "非空反置页表表项").pack(anchor="w")
        self.inverted_tree = self.make_tree(
            right,
            ("index", "pid", "page", "conflict", "state"),
            ("表项序号", "进程号", "逻辑页号", "冲突", "状态"),
            (90, 90, 90, 70, 70),
        )
        ttk.Separator(right).pack(fill="x", pady=12)
        self.inverted_canvas = tk.Canvas(right, bg=PANEL, height=210, highlightthickness=0)
        self.inverted_canvas.pack(fill="both", expand=True)
        self.run_inverted()

    def jobs(self, count, min_size, max_size, base_pid=101):
        return [{"pid": base_pid + i, "size": random.randint(min_size, max_size)} for i in range(count)]

    def run_static(self):
        frame_count = int(self.static_frame_count.get())
        frame_size = int(self.static_frame_size.get())
        job_count = int(self.static_job_count.get())
        jobs = self.jobs(job_count, frame_size, frame_size * 4)
        for job in jobs:
            job["need"] = math.ceil(job["size"] / frame_size)

        self.clear_tree(self.static_jobs)
        for job in jobs:
            self.static_jobs.insert("", "end", values=(f"P{job['pid']}", job["size"], job["need"]))

        rows = []
        bitmap = self.sim_bitmap(jobs, frame_count, rows)
        free_table = self.sim_free_table(jobs, frame_count, rows)
        chain = self.sim_chain(jobs, frame_count, rows)

        self.clear_tree(self.static_result)
        for row in rows:
            self.static_result.insert("", "end", values=row)

        self.draw_static(frame_count, frame_size, bitmap, free_table, chain)

    def sim_bitmap(self, jobs, frame_count, rows):
        bitmap = [0] * frame_count
        for job in jobs:
            free = [i for i, value in enumerate(bitmap) if value == 0]
            if len(free) >= job["need"]:
                frames = free[: job["need"]]
                for frame in frames:
                    bitmap[frame] = 1
                rows.append(("字位映象图", f"P{job['pid']}", job["need"], "成功", ",".join(map(str, frames))))
            else:
                rows.append(("字位映象图", f"P{job['pid']}", job["need"], "失败", "-"))
        return bitmap

    def sim_free_table(self, jobs, frame_count, rows):
        table = list(range(frame_count))
        for job in jobs:
            if len(table) >= job["need"]:
                frames = table[: job["need"]]
                del table[: job["need"]]
                rows.append(("空闲页面表", f"P{job['pid']}", job["need"], "成功", ",".join(map(str, frames))))
            else:
                rows.append(("空闲页面表", f"P{job['pid']}", job["need"], "失败", "-"))
        return table

    def sim_chain(self, jobs, frame_count, rows):
        chain = list(range(frame_count))
        for job in jobs:
            if len(chain) >= job["need"]:
                frames = chain[: job["need"]]
                del chain[: job["need"]]
                rows.append(("空闲页面链", f"P{job['pid']}", job["need"], "成功", ",".join(map(str, frames))))
            else:
                rows.append(("空闲页面链", f"P{job['pid']}", job["need"], "失败", "-"))
        return chain

    def draw_static(self, frame_count, frame_size, bitmap, free_table, chain):
        c = self.static_canvas
        c.delete("all")
        width = max(c.winfo_width(), 760)
        self.draw_title(c, "静态等长分区图形化结果", 24, 24)
        self.draw_metric_row(
            c,
            24,
            58,
            [
                ("页框数", str(frame_count), BLUE),
                ("页框大小", f"{frame_size} KB", CYAN),
                ("已占用", str(sum(bitmap)), RED),
                ("空闲", str(frame_count - sum(bitmap)), GREEN),
            ],
        )

        c.create_text(24, 136, text="字位映象图", anchor="w", fill=INK, font=self.font_section)
        cols = 16
        cell = min(30, max(22, (width - 80) // cols - 4))
        x0, y0 = 24, 164
        for i, value in enumerate(bitmap):
            row, col = divmod(i, cols)
            x = x0 + col * (cell + 5)
            y = y0 + row * (cell + 5)
            color = RED if value else GREEN
            self.round_rect(c, x, y, x + cell, y + cell, 6, fill=color, outline="")
            c.create_text(x + cell / 2, y + cell / 2, text=str(i), fill="#ffffff", font=("Consolas", 8, "bold"))

        y_table = y0 + math.ceil(frame_count / cols) * (cell + 5) + 34
        c.create_text(24, y_table, text="空闲页面表", anchor="w", fill=INK, font=self.font_section)
        self.draw_chip_list(c, 24, y_table + 26, free_table, max_items=32, color="#e0f2fe", border=CYAN)

        y_chain = y_table + 112
        c.create_text(24, y_chain, text="空闲页面链", anchor="w", fill=INK, font=self.font_section)
        self.draw_chain(c, 24, y_chain + 30, chain[:14], more=len(chain) > 14)

    def run_dynamic(self):
        memory_size = int(self.dynamic_memory_size.get())
        job_count = int(self.dynamic_job_count.get())
        jobs = self.jobs(job_count, 16, max(16, memory_size // 2))

        self.clear_tree(self.dynamic_jobs)
        for job in jobs:
            self.dynamic_jobs.insert("", "end", values=(f"P{job['pid']}", job["size"]))

        algorithms = [("最先适应", "first", BLUE), ("下次适应", "next", CYAN), ("最佳适应", "best", GREEN), ("最坏适应", "worst", VIOLET)]
        states = []
        self.dynamic_log.delete("1.0", "end")

        for title, key, color in algorithms:
            partitions, log = self.sim_dynamic(jobs, memory_size, key)
            states.append((title, color, partitions))
            self.dynamic_log.insert("end", f"【{title}】\n")
            for line in log:
                self.dynamic_log.insert("end", f"{line}\n")
            self.dynamic_log.insert("end", "\n")

        self.draw_dynamic(memory_size, states)

    def sim_dynamic(self, jobs, memory_size, algorithm):
        partitions = [{"start": 0, "size": memory_size, "pid": None, "free": True}]
        rover = 0
        first_pid = None
        log = []

        for job in jobs:
            index = self.find_partition(partitions, job["size"], algorithm, rover)
            if index is None:
                log.append(f"P{job['pid']} 申请 {job['size']}KB：失败，无可用空闲区")
                continue

            part = partitions[index]
            old_start = part["start"]
            if part["size"] == job["size"]:
                part["pid"] = job["pid"]
                part["free"] = False
            else:
                partitions.insert(index + 1, {
                    "start": part["start"] + job["size"],
                    "size": part["size"] - job["size"],
                    "pid": None,
                    "free": True,
                })
                part["size"] = job["size"]
                part["pid"] = job["pid"]
                part["free"] = False
            rover = (index + 1) % len(partitions)
            first_pid = first_pid or job["pid"]
            log.append(f"P{job['pid']} 申请 {job['size']}KB：成功，起址 {old_start}KB")

        if first_pid is not None:
            self.release_partition(partitions, first_pid)
            log.append(f"释放 P{first_pid}，合并相邻空闲区")
        return partitions, log

    def find_partition(self, partitions, request, algorithm, rover):
        candidates = [(i, p) for i, p in enumerate(partitions) if p["free"] and p["size"] >= request]
        if not candidates:
            return None
        if algorithm == "first":
            return candidates[0][0]
        if algorithm == "next":
            for step in range(len(partitions)):
                i = (rover + step) % len(partitions)
                p = partitions[i]
                if p["free"] and p["size"] >= request:
                    return i
        if algorithm == "best":
            return min(candidates, key=lambda item: item[1]["size"])[0]
        return max(candidates, key=lambda item: item[1]["size"])[0]

    def release_partition(self, partitions, pid):
        for part in partitions:
            if part["pid"] == pid:
                part["pid"] = None
                part["free"] = True
                break
        i = 0
        while i + 1 < len(partitions):
            if partitions[i]["free"] and partitions[i + 1]["free"]:
                partitions[i]["size"] += partitions[i + 1]["size"]
                del partitions[i + 1]
            else:
                i += 1

    def draw_dynamic(self, memory_size, states):
        c = self.dynamic_canvas
        c.delete("all")
        width = max(c.winfo_width(), 760)
        self.draw_title(c, "动态异长分区最终状态", 24, 24)
        self.draw_metric_row(c, 24, 58, [("内存总量", f"{memory_size} KB", BLUE), ("算法数", "4", CYAN)])

        y = 138
        for title, color, partitions in states:
            c.create_text(24, y + 18, text=title, anchor="w", fill=INK, font=self.font_section)
            x = 132
            bar_w = width - 180
            for part in partitions:
                w = max(36, int(bar_w * part["size"] / memory_size))
                fill = "#dcfce7" if part["free"] else "#dbeafe"
                outline = GREEN if part["free"] else color
                self.round_rect(c, x, y, x + w, y + 44, 8, fill=fill, outline=outline, width=2)
                label = "空闲" if part["free"] else f"P{part['pid']}"
                c.create_text(x + w / 2, y + 16, text=label, fill=INK, font=("Microsoft YaHei UI", 9, "bold"))
                c.create_text(x + w / 2, y + 32, text=f"{part['size']}KB", fill=MUTED, font=("Consolas", 8))
                x += w + 2
            c.create_text(132, y + 62, text="0KB", anchor="w", fill=MUTED, font=("Consolas", 9))
            c.create_text(132 + bar_w, y + 62, text=f"{memory_size}KB", anchor="e", fill=MUTED, font=("Consolas", 9))
            y += 120

    def run_inverted(self):
        memory_size = 256 * MB if self.memory_choice.get().startswith("256") else 512 * MB
        frame_size = int(self.frame_choice.get().split()[0]) * KB
        frame_count = memory_size // frame_size
        process_count = int(self.process_count.get())
        processes = self.inverted_processes(process_count, frame_size, frame_count)

        table = {}
        entries = []
        for process in processes:
            for page in range(process["pages"]):
                index, conflicts = self.insert_page(table, frame_count, frame_size, process["pid"], page)
                entries.append({"index": index, "pid": process["pid"], "page": page, "conflicts": conflicts})

        table_size = frame_count * ENTRY_BYTES
        self.inverted_stats.configure(
            text=(
                f"内存物理空间：{memory_size // MB} MB\n"
                f"页框大小：{frame_size // KB} KB\n"
                f"反置页表项数：{frame_count}\n"
                f"每个表项：{ENTRY_BYTES} 字节\n"
                f"页表空间：{table_size} 字节，约 {table_size / MB:.2f} MB"
            )
        )

        self.clear_tree(self.process_tree)
        for process in processes:
            self.process_tree.insert("", "end", values=(f"P{process['pid']}", process["size"], process["pages"]))

        self.clear_tree(self.inverted_tree)
        for entry in sorted(entries, key=lambda item: item["index"])[:500]:
            self.inverted_tree.insert("", "end", values=(entry["index"], f"P{entry['pid']}", entry["page"], entry["conflicts"], "占用"))

        translation = self.translate(table, frame_count, frame_size, processes)
        self.translation.configure(text=translation)
        self.draw_inverted(frame_count, frame_size, entries, len(entries))

    def inverted_processes(self, count, frame_size, frame_count):
        processes = []
        used_pages = 0
        max_pages_by_address = 65536 // frame_size
        for i in range(count):
            remaining = count - i - 1
            max_pages = min(frame_count - used_pages - remaining * 4, max_pages_by_address)
            pages = random.randint(4, max(4, int(max_pages)))
            size = random.randint((pages - 1) * frame_size + 1, pages * frame_size)
            processes.append({"pid": 201 + i, "size": size, "pages": math.ceil(size / frame_size)})
            used_pages += pages
        return processes

    def hash_index(self, pid, page, frame_size, frame_count):
        return (pid * frame_size + page) % frame_count

    def insert_page(self, table, frame_count, frame_size, pid, page):
        index = self.hash_index(pid, page, frame_size, frame_count)
        conflicts = 0
        while index in table:
            conflicts += 1
            index = (index + 1) % frame_count
        table[index] = {"pid": pid, "page": page, "conflicts": conflicts}
        return index, conflicts

    def translate(self, table, frame_count, frame_size, processes):
        process = random.choice(processes)
        logical_address = random.randint(0, process["size"] - 1)
        page = logical_address // frame_size
        offset = logical_address % frame_size
        index = self.hash_index(process["pid"], page, frame_size, frame_count)

        for _ in range(frame_count):
            entry = table.get(index)
            if entry is None:
                break
            if entry["pid"] == process["pid"] and entry["page"] == page:
                physical = index * frame_size + offset
                return (
                    f"选中进程：P{process['pid']}\n"
                    f"逻辑地址 L：0x{logical_address:04X}\n"
                    f"逻辑页号：0x{page:X}\n"
                    f"页内偏移：0x{offset:X}\n"
                    f"对应页框号：0x{index:X}\n"
                    f"物理地址：0x{physical:X}"
                )
            index = (index + 1) % frame_count
        return "查表失败"

    def draw_inverted(self, frame_count, frame_size, entries, occupied_count):
        c = self.inverted_canvas
        c.delete("all")
        width = max(c.winfo_width(), 760)
        self.draw_title(c, "Hash 表占用抽样图", 24, 24)
        self.draw_metric_row(
            c,
            24,
            58,
            [
                ("页框数", str(frame_count), BLUE),
                ("页框大小", f"{frame_size // KB} KB", CYAN),
                ("非空表项", str(occupied_count), GREEN),
            ],
        )

        sample = 120
        used = {int(entry["index"] * sample / frame_count) for entry in entries}
        x0, y0 = 24, 142
        w = max(5, (width - 70) // sample)
        for i in range(sample):
            fill = BLUE if i in used else "#e5eaf2"
            c.create_rectangle(x0 + i * w, y0, x0 + (i + 1) * w - 1, y0 + 48, fill=fill, outline="")
        c.create_text(x0, y0 + 66, text="0", anchor="w", fill=MUTED, font=self.font_mono)
        c.create_text(x0 + sample * w, y0 + 66, text=str(frame_count - 1), anchor="e", fill=MUTED, font=self.font_mono)
        self.legend(c, 24, y0 + 96, [(BLUE, "占用表项"), ("#e5eaf2", "空闲表项")])

    def draw_title(self, canvas, text, x, y):
        canvas.create_text(x, y, text=text, anchor="w", fill=INK, font=self.font_title)

    def draw_metric_row(self, canvas, x, y, metrics):
        box_w = 150
        for i, (label, value, color) in enumerate(metrics):
            left = x + i * (box_w + 12)
            self.round_rect(canvas, left, y, left + box_w, y + 58, 12, fill="#f8fafc", outline=BORDER)
            canvas.create_text(left + 14, y + 17, text=label, anchor="w", fill=MUTED, font=("Microsoft YaHei UI", 9))
            canvas.create_text(left + 14, y + 40, text=value, anchor="w", fill=color, font=("Microsoft YaHei UI", 14, "bold"))

    def draw_chip_list(self, canvas, x, y, values, max_items, color, border):
        if not values:
            self.round_rect(canvas, x, y, x + 82, y + 30, 14, fill="#f1f5f9", outline=BORDER)
            canvas.create_text(x + 41, y + 15, text="空", fill=MUTED)
            return
        cx, cy = x, y
        for value in values[:max_items]:
            self.round_rect(canvas, cx, cy, cx + 48, cy + 28, 14, fill=color, outline=border)
            canvas.create_text(cx + 24, cy + 14, text=str(value), fill=INK, font=self.font_mono)
            cx += 56
            if cx > 720:
                cx = x
                cy += 36
        if len(values) > max_items:
            canvas.create_text(cx + 8, cy + 14, text="...", anchor="w", fill=MUTED)

    def draw_chain(self, canvas, x, y, values, more=False):
        if not values:
            canvas.create_text(x, y + 16, text="空", anchor="w", fill=MUTED)
            return
        cx = x
        for i, value in enumerate(values):
            self.round_rect(canvas, cx, y, cx + 54, y + 34, 10, fill="#fff7ed", outline=AMBER, width=2)
            canvas.create_text(cx + 27, y + 17, text=str(value), fill=INK, font=self.font_mono)
            if i + 1 < len(values):
                canvas.create_line(cx + 56, y + 17, cx + 82, y + 17, arrow="last", fill=AMBER, width=2)
            cx += 86
        if more:
            canvas.create_text(cx, y + 17, text="...", anchor="w", fill=MUTED, font=self.font_section)

    def legend(self, canvas, x, y, items):
        cx = x
        for color, label in items:
            canvas.create_rectangle(cx, y, cx + 18, y + 18, fill=color, outline=BORDER)
            canvas.create_text(cx + 26, y + 9, text=label, anchor="w", fill=MUTED)
            cx += 116

    def round_rect(self, canvas, x1, y1, x2, y2, radius, **kwargs):
        points = [
            x1 + radius, y1, x2 - radius, y1, x2, y1, x2, y1 + radius,
            x2, y2 - radius, x2, y2, x2 - radius, y2, x1 + radius, y2,
            x1, y2, x1, y2 - radius, x1, y1 + radius, x1, y1,
        ]
        return canvas.create_polygon(points, smooth=True, **kwargs)


if __name__ == "__main__":
    try:
        MemoryLabApp().mainloop()
    except tk.TclError as exc:
        print(f"图形界面启动失败：{exc}", file=sys.stderr)
        print("请在 Windows Python、WSLg 或配置好 X Server 的环境中运行。", file=sys.stderr)
        sys.exit(1)
