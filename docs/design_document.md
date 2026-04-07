# MiniKernel v0.1 — ESP32 迷你操作系统

## 项目设计说明书 & 方案设计文档

---

## 1. 项目概述

### 1.1 项目名称
MiniKernel — ESP32 迷你实时内核

### 1.2 项目目标
为 ESP-WROOM-32 模块从零构建一个教学级别的裸机迷你操作系统内核，不依赖 ESP-IDF 框架，涵盖任务调度、内存管理、中断处理、同步原语和串口调试等核心操作系统功能。

### 1.3 目标硬件
| 特性 | 规格 |
|------|------|
| 芯片 | ESP-WROOM-32 |
| 处理器 | Xtensa LX6 双核 (仅使用 Core 0) |
| 主频 | 240 MHz |
| IRAM | ~128 KB (代码段) |
| DRAM | ~176 KB (数据段 + 堆) |
| Flash | 4 MB |
| 调试接口 | UART0 (GPIO1 TX, GPIO3 RX, 115200 baud) |

### 1.4 技术选型
- **编程语言**: C + Xtensa 汇编
- **编译器**: xtensa-esp32-elf-gcc
- **ABI**: call0 (非窗口化，简化上下文切换)
- **构建工具**: GNU Make
- **烧录工具**: esptool.py

### 1.5 选择 C 语言的原因
1. ESP32 官方工具链 (xtensa-esp32-elf-gcc) 对 C 提供第一级支持
2. 所有寄存器定义、启动代码、链接脚本均为 C 头文件格式
3. 零运行时开销，精确控制内存布局和硬件操作
4. FreeRTOS、ESP-IDF 均为 C 实现，参考资料丰富
5. 无隐藏行为 (无 GC、构造/析构)，适合内核级开发

---

## 2. 系统架构设计

### 2.1 整体架构

```
┌─────────────────────────────────────────────┐
│                Application Layer            │
│            (apps/main.c - 用户应用)          │
├─────────────────────────────────────────────┤
│              System Services                │
│   ┌──────────┐  ┌──────────┐  ┌──────────┐ │
│   │  Mutex   │  │Semaphore │  │  Sleep   │ │
│   └──────────┘  └──────────┘  └──────────┘ │
├─────────────────────────────────────────────┤
│                Kernel Core                  │
│   ┌──────────┐  ┌──────────┐  ┌──────────┐ │
│   │Scheduler │  │  Task    │  │ Memory   │ │
│   │(优先级+  │  │ Manager  │  │ Manager  │ │
│   │ 轮转)    │  │  (TCB)   │  │(first-fit│ │
│   └──────────┘  └──────────┘  └──────────┘ │
├─────────────────────────────────────────────┤
│              Hardware Abstract              │
│   ┌──────────┐  ┌──────────┐  ┌──────────┐ │
│   │  Timer   │  │   IRQ    │  │Context   │ │
│   │ Driver   │  │Framework │  │ Switch   │ │
│   └──────────┘  └──────────┘  └──────────┘ │
├─────────────────────────────────────────────┤
│              Hardware Drivers               │
│   ┌──────────┐  ┌──────────┐               │
│   │  UART0   │  │   GPIO   │               │
│   │ (console)│  │          │               │
│   └──────────┘  └──────────┘               │
├─────────────────────────────────────────────┤
│              Boot Layer                     │
│   ┌──────────────────────────────────────┐  │
│   │ start.S → VECBASE → kernel_main()   │  │
│   └──────────────────────────────────────┘  │
└─────────────────────────────────────────────┘
          ▼ Hardware: ESP-WROOM-32 ▼
```

### 2.2 内存布局

```
地址范围              用途              大小
─────────────────────────────────────────────
0x40080000 - 0x400A0000  IRAM (代码)    128 KB
  ├─ .text.entry          启动入口
  ├─ .text                内核代码 + 驱动
  ├─ .literal             常量
  └─ .rodata              只读数据

0x3FFB0000 - 0x3FFDC000  DRAM (数据)    ~176 KB
  ├─ .data                已初始化数据
  ├─ .bss                 未初始化数据
  ├─ Stack                主栈 (8 KB)
  └─ Heap                 堆 (64 KB)
```

### 2.3 启动流程

```
ESP32 上电
    │
    ▼
ROM Bootloader (芯片内置, 0x40000000)
    │  加载 Flash 0x1000 处的内核镜像到 IRAM
    ▼
_start (boot/start.S)
    │  1. 禁用中断 (PS.INTLEVEL = 15)
    │  2. 设置栈指针 (SP = _stack_top)
    │  3. 安装异常向量表 (VECBASE = _vector_table)
    │  4. 清零 BSS 段
    ▼
kernel_main() (kernel/kernel.c)
    │  1. 初始化 UART0
    │  2. 初始化堆内存
    │  3. 初始化中断控制器
    │  4. 初始化任务子系统
    │  5. 启动系统定时器 (1ms tick)
    │  6. 创建 idle 任务
    │  7. 调用 app_main() 创建用户任务
    │  8. 启用中断
    │  9. 切换到第一个就绪任务
    ▼
任务调度循环运行
```

---

## 3. 模块详细设计

### 3.1 中断处理框架 (kernel/irq.c)

#### 3.1.1 设计原理
ESP32 的中断系统分为两层：
- **外设中断**: 通过 Interrupt Matrix 将外设中断源映射到 CPU 中断号
- **CPU 中断**: Xtensa LX6 支持 Level 1-7 中断级别

#### 3.1.2 异常向量表
```
偏移     类型                 处理方式
0x000    WindowOverflow4     → _start_hang (未使用窗口化 ABI)
0x040    WindowUnderflow4    → _start_hang
0x080    WindowOverflow8     → _start_hang
0x0C0    WindowUnderflow8    → _start_hang
0x100    WindowOverflow12    → _start_hang
0x140    WindowUnderflow12   → _start_hang
0x180    Level 1 中断        → _level1_interrupt_handler (定时器 tick)
0x1C0    Level 2-5           → _default_exception_handler
0x1E0    Level 6 (Debug)     → _default_exception_handler
0x1F0    Level 7 (NMI)       → _default_exception_handler
```

#### 3.1.3 中断分发流程
```
硬件中断触发
    │
    ▼
Xtensa 跳转到 VECBASE + 0x180 (Level 1)
    │
    ▼
_level1_interrupt_handler (vectors.S)
    │  保存 a0-a3, a12-a15, SAR, EPC1, PS 到栈
    │  保存 SP 到 _current_task_sp
    ▼
irq_dispatch() (kernel/irq.c)
    │  读取 INTERRUPT 和 INTENABLE 寄存器
    │  找到最高优先级挂起中断
    │  调用注册的 ISR
    │  清除中断
    ▼
返回 _level1_interrupt_handler
    │  恢复寄存器
    │  rfi 1 (从中断返回)
```

### 3.2 系统定时器 (kernel/timer.c)

#### 3.2.1 硬件配置
- 使用 Timer Group 0 Timer 0 (ESP32 外设定时器)
- 时钟源: APB_CLK = 80 MHz
- 报警值: 80,000 (80MHz / 1000Hz = 每 1ms 触发一次)
- 中断映射: 外设源 46 → CPU 中断 6, 优先级 1

#### 3.2.2 Tick 处理
```c
timer_isr() {
    清除中断标志
    system_ticks++
    更新报警值 += 80000
    调用 sched_tick()  // 检查睡眠任务
}
```

### 3.3 任务管理 (kernel/task.c)

#### 3.3.1 任务控制块 (TCB)
```c
typedef struct tcb {
    uint32_t    *sp;           // 保存的栈指针
    uint32_t    *stack_base;   // 栈基地址
    uint32_t     stack_size;   // 栈大小
    int          pid;          // 任务 ID (0-15)
    int          priority;     // 优先级 (0=最高, 31=最低)
    task_state_t state;        // READY / RUNNING / BLOCKED / DELETED
    uint32_t     wake_tick;    // 唤醒时间 (用于 sleep)
    struct tcb  *next;         // 链表指针
} tcb_t;
```

#### 3.3.2 任务状态转换
```
                    task_create()
                        │
                        ▼
                 ┌─────────────┐
        ┌───────│    READY    │◄───────┐
        │       └─────────────┘        │
        │            │                 │
        │   sched_pick_next()          │ task_unblock()
        │            │                 │
        │            ▼                 │
        │       ┌─────────────┐        │
        │       │   RUNNING   │────────┘
        │       └─────────────┘  task_yield()
        │            │
        │   task_block()
        │            │
        │            ▼
        │       ┌─────────────┐
        │       │   BLOCKED   │─── task_exit() ───→ DELETED
        │       └─────────────┘
        │        (等待 mutex/
        │       semaphore/sleep)
        └────────────────────────────────────
```

#### 3.3.3 优先级定义
| 优先级值 | 含义 | 用途 |
|---------|------|------|
| 0 | 最高 | 实时任务 |
| 4 | 高 | 重要中断处理 |
| 8 | 普通 | 默认任务 |
| 16 | 低 | 后台任务 |
| 31 | 最低 | Idle 任务 |

### 3.4 上下文切换 (kernel/vectors.S)

#### 3.4.1 call0 ABI 寄存器保存
上下文切换时保存的寄存器 (call0 ABI):
```
偏移    寄存器    用途
0       a0       返回地址
4       a12      调用者保存
8       a13      调用者保存
12      a14      调用者保存
16      a15      调用者保存
20      SAR      移位量寄存器
─────────────────────
总计: 24 字节
```

#### 3.4.2 新任务栈初始化
```
新任务的栈布局 (从高地址到低地址):

  ┌────────────────┐  ← stack_base + stack_size
  │    arg         │  任务参数 (传递给 task_func)
  │    func        │  任务函数指针
  │    0           │  填充
  │  _task_entry   │  入口点 (context_switch "返回" 到这里)
  ├────────────────┤
  │  _task_entry   │  a0 (上下文保存区)
  │  0             │  a12
  │  0             │  a13
  │  0             │  a14
  │  0             │  a15
  │  0             │  SAR
  └────────────────┘  ← sp (栈指针指向这里)
```

### 3.5 调度器 (kernel/sched.c)

#### 3.5.1 调度策略
采用 **优先级抢占 + 同优先级轮转** 策略:
1. 每个优先级维护一个 FIFO 就绪队列
2. 调度器从最高优先级 (0) 开始扫描
3. 同优先级任务按 FIFO 顺序执行 (轮转)
4. 每次 tick 中断触发调度检查

#### 3.5.2 调度触发时机
- `task_yield()`: 任务主动让出 CPU
- `task_block()`: 任务等待资源 (mutex/semaphore/sleep)
- Timer tick ISR: 检查睡眠任务是否到时，可能触发调度

### 3.6 内存管理 (kernel/memory.c)

#### 3.6.1 堆分配算法
采用 **First-Fit** 算法:
```
堆内存结构:
┌──────────┬──────────┬──────────┬──────────┐
│ Block Hdr│   Data   │ Block Hdr│   Data   │ ...
└──────────┴──────────┴──────────┴──────────┘

Block Header:
┌─────────────────────┐
│ size: 块大小(不含头) │
│ used: 0=空闲, 1=占用│
│ next: 下一个块指针   │
└─────────────────────┘
```

#### 3.6.2 分配流程
1. 对齐请求大小到 4 字节边界
2. 遍历块链表，找到第一个足够大的空闲块
3. 如果剩余空间 ≥ header + 16 字节，分裂块
4. 标记为已使用，返回数据区指针

#### 3.6.3 释放流程
1. 标记块为空闲
2. 与下一个空闲块合并
3. 从头扫描，与前一个空闲块合并

### 3.7 同步原语 (kernel/sync.c)

#### 3.7.1 互斥锁 (Mutex)
- 支持基本加锁/解锁
- 加锁时若已被占用，当前任务阻塞并加入等待队列
- 解锁时唤醒等待队列中第一个任务
- 临界区内禁用中断保证原子性

#### 3.7.2 计数信号量 (Semaphore)
- 可配置初始值和最大值
- wait: count > 0 时递减并返回；count = 0 时阻塞
- signal: 有等待者时唤醒第一个；无等待者时递增 count
- 用于生产者-消费者模式

---

## 4. 目录结构

```
esp32-os/
├── Makefile                    # 构建系统
├── ld/
│   └── esp32.ld               # 链接脚本 (内存布局)
├── boot/
│   └── start.S                # 汇编启动代码
├── kernel/
│   ├── kernel.c               # 内核主入口 & 初始化
│   ├── kernel.h               # 内核公共接口
│   ├── task.c / task.h        # 任务管理
│   ├── sched.c / sched.h      # 调度器
│   ├── memory.c / memory.h    # 堆内存管理
│   ├── irq.c / irq.h          # 中断框架
│   ├── timer.c / timer.h      # 系统定时器
│   ├── sync.c / sync.h        # Mutex & Semaphore
│   └── vectors.S              # 异常向量表 + 上下文切换
├── drivers/
│   ├── uart.c / uart.h        # UART0 驱动
│   └── gpio.c / gpio.h        # GPIO 驱动
├── include/
│   ├── esp32_regs.h           # ESP32 寄存器定义
│   ├── types.h                # 基本类型
│   ├── xtensa_ops.h           # Xtensa 内联汇编操作
│   └── miniprintf.c / .h      # 最小化 printf
├── apps/
│   └── main.c                 # 示例应用
└── build/                     # 构建输出目录
```

---

## 5. 构建与烧录

### 5.1 前置要求
- xtensa-esp32-elf-gcc 工具链
- esptool.py
- Python (用于串口监视)
- USB 串口驱动 (CP210x / CH340)

### 5.2 编译
```bash
make all
```

### 5.3 生成烧录镜像
```bash
make image
```

### 5.4 烧录
```bash
make flash PORT=COM3
```

### 5.5 串口监视
```bash
make monitor PORT=COM3
```

---

## 6. Demo 应用说明

Demo 应用 (`apps/main.c`) 实现了一个经典的生产者-消费者模式:

| 任务 | 优先级 | 功能 |
|------|--------|------|
| task_status | 16 (低) | 每 2 秒打印系统状态 |
| task_producer | 8 (普通) | 每 500ms 递增计数器，通过 semaphore 通知消费者 |
| task_consumer | 8 (普通) | 等待 semaphore，读取并打印计数器 |
| idle_task | 31 (最低) | 空闲时让出 CPU |

### 预期串口输出
```
========================================
  MiniKernel v0.1 for ESP-WROOM-32
  Xtensa LX6 - Bare Metal
========================================
[kernel] IRAM code section loaded
[kernel] Stack top: 0x3ffb0000
[kernel] Heap: 0x3ffb2000 - 0x3ffb2000 + 0x10000
[kernel] Heap initialized: 65536 bytes
[kernel] Interrupt controller initialized
[kernel] Task subsystem initialized
[kernel] System timer started (1000 Hz tick)
[kernel] Idle task created (pid=0)
[kernel] Starting application...

[app] Initializing synchronization primitives...
[app] Creating demo tasks...
[app] Tasks created: status=1, producer=2, consumer=3
[producer] produced: 1
[consumer] consumed: 1
[producer] produced: 2
[consumer] consumed: 2
[status] tick=2000  counter=4  loop=0
...
```

---

## 7. 关键技术决策

| 决策 | 选择 | 原因 |
|------|------|------|
| ABI | call0 (非窗口化) | 简化上下文切换，避免 WindowOverflow 异常处理 |
| 调度算法 | 优先级 + 轮转 | 简单有效，支持不同优先级任务 |
| 堆算法 | First-Fit | 实现简单，适合小内存系统 |
| 定时器 | Timer Group 0 | ESP32 标准 HP 定时器，文档完善 |
| 代码位置 | IRAM | 避免 Flash Cache 复杂性，确定性执行时间 |
| 最大任务数 | 16 | 平衡内存使用和实际需求 |
| 栈大小 | 可配置 | 不同任务可分配不同栈大小 |

---

## 8. 已知限制与未来扩展

### 当前限制
- 仅支持单核 (Core 0)
- 无虚拟内存/内存保护
- 无文件系统
- 无网络栈
- 调度器为协作式 + tick 检查 (非严格抢占)
- 堆分配器无碎片整理

### 可能的扩展方向
- 双核支持 (SMP 调度)
- 抢占式调度 (在 tick ISR 中强制切换)
- 虚拟文件系统
- SPI Flash 驱动
- Wi-Fi 网络栈
- 动态加载应用
