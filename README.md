# MiniKernel v0.1

为 ESP-WROOM-32 从零构建的裸机迷你操作系统内核。

## 特性

- **纯裸机** — 不依赖 ESP-IDF，自写启动代码、链接脚本、异常向量表
- **任务调度** — 优先级抢占 + 同优先级轮转，最多 16 个任务
- **内存管理** — First-Fit 堆分配器 (kmalloc / kfree)
- **中断处理** — INTC 中断框架，ISR 注册与分发
- **系统定时器** — 1ms 精度 tick (Timer Group 0, 1000Hz)
- **同步原语** — Mutex + 计数信号量
- **UART 控制台** — 115200 baud 串口输出，支持 printf 格式化
- **上下文切换** — Xtensa call0 ABI，手工汇编实现

## 目标硬件

| 项目 | 规格 |
|------|------|
| 芯片 | ESP-WROOM-32 |
| 处理器 | Xtensa LX6 (单核, Core 0) |
| 主频 | 240 MHz |
| IRAM | ~128 KB (代码) |
| DRAM | ~176 KB (数据 + 堆) |
| Flash | 4 MB |
| 串口 | UART0: GPIO1 (TX), GPIO3 (RX) |

## 目录结构

```
esp32-os/
├── Makefile                 # 构建系统
├── ld/esp32.ld              # 链接脚本 (内存布局)
├── boot/start.S             # 汇编启动入口
├── kernel/
│   ├── kernel.c             # 内核主入口 & 初始化
│   ├── task.c / task.h      # 任务管理 (TCB)
│   ├── sched.c / sched.h    # 优先级调度器
│   ├── memory.c / memory.h  # 堆内存管理
│   ├── irq.c / irq.h        # 中断框架
│   ├── timer.c / timer.h    # 系统定时器
│   ├── sync.c / sync.h      # Mutex & Semaphore
│   └── vectors.S            # 异常向量表 & 上下文切换
├── drivers/
│   ├── uart.c / uart.h      # UART0 驱动
│   └── gpio.c / gpio.h      # GPIO 驱动
├── include/
│   ├── esp32_regs.h         # ESP32 寄存器定义
│   ├── types.h              # 基本类型
│   ├── xtensa_ops.h         # Xtensa 内联汇编操作
│   └── miniprintf.c / .h    # 最小化 printf
├── apps/main.c              # 示例应用
└── docs/design_document.md  # 详细设计文档
```

## 快速开始

### 1. 安装工具链

```bash
# 安装 xtensa-esp32-elf-gcc
# https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-tools.html

# 安装 esptool.py
pip install esptool
```

### 2. 编译

```bash
make all
```

### 3. 生成烧录镜像

```bash
make image
```

### 4. 烧录到 ESP32

```bash
make flash PORT=COM3    # Windows
make flash PORT=/dev/ttyUSB0    # Linux
```

### 5. 查看串口输出

```bash
make monitor PORT=COM3    # 115200 baud
```

## 预期输出

```
========================================
  MiniKernel v0.1 for ESP-WROOM-32
  Xtensa LX6 - Bare Metal
========================================
[kernel] IRAM code section loaded
[kernel] Stack top: 0x3ffb0000
[kernel] Heap: 0x3ffb2000 - 0x3ffb3000
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

## API 参考

### 任务管理

```c
// 创建任务 (返回 pid, 失败返回 -1)
int task_create(task_func_t func, void *arg, uint32_t stack_size, int priority);

// 让出 CPU
void task_yield(void);

// 休眠 (毫秒)
void task_sleep_ms(uint32_t ms);

// 获取当前任务 TCB
tcb_t *task_current(void);
```

### 同步原语

```c
// Mutex
mutex_t m;
mutex_init(&m);
mutex_lock(&m);
mutex_unlock(&m);

// Semaphore
semaphore_t s;
sem_init(&s, 0, 10);   // 初始值 0, 最大值 10
sem_wait(&s);
sem_signal(&s);
```

### 内存管理

```c
void *kmalloc(uint32_t size);
void  kfree(void *ptr);
```

### 串口输出

```c
uart_puts("Hello");
miniprintf("value = %d, addr = 0x%x\n", val, ptr);
```

### 优先级定义

| 宏 | 值 | 用途 |
|----|---|------|
| `TASK_PRIORITY_MAX` | 0 | 最高优先级 |
| `TASK_PRIORITY_HIGH` | 4 | 高优先级 |
| `TASK_PRIORITY_NORMAL` | 8 | 默认 |
| `TASK_PRIORITY_LOW` | 16 | 低优先级 |
| `TASK_PRIORITY_IDLE` | 31 | 空闲任务 |

## 启动流程

```
ESP32 上电
  → ROM Bootloader 加载 Flash 0x1000 处内核镜像到 IRAM
    → _start (start.S): 禁用中断, 设置 SP, 安装 VECBASE, 清零 BSS
      → kernel_main(): 初始化 UART / 堆 / INTC / 任务 / 定时器
        → app_main(): 创建用户任务
          → 启用中断, 切换到第一个就绪任务
```

## 技术选型

| 决策 | 选择 | 原因 |
|------|------|------|
| 语言 | C + Xtensa 汇编 | 工具链原生支持, 零开销, 可预测 |
| ABI | call0 (非窗口化) | 简化上下文切换, 无需处理 WindowOverflow |
| 调度 | 优先级 + 轮转 | 简单有效, 适合小系统 |
| 堆算法 | First-Fit | 实现简单, 碎片可接受 |
| 代码位置 | IRAM | 确定性执行, 避免 Flash Cache |

## 详细文档

完整的系统架构设计、模块详细设计、数据结构和流程图见：

[docs/design_document.md](docs/design_document.md)

## 许可证

MIT License
