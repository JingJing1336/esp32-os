#ifndef ESP32_REGS_H
#define ESP32_REGS_H

#include "types.h"

/* ============================================================
 * ESP32 Memory Map
 * ============================================================ */
#define DRAM_BASE       0x3FFB0000
#define DRAM_SIZE       0x00050000  /* ~320KB, shared with cache */
#define IRAM_BASE       0x40080000
#define IRAM_SIZE       0x00020000  /* 128KB */

/* ============================================================
 * UART Registers (UART0 base: 0x3FF40000)
 * ============================================================ */
#define UART0_BASE      0x3FF40000

#define UART_FIFO_REG       (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UART_INT_RAW_REG    (*(volatile uint32_t *)(UART0_BASE + 0x04))
#define UART_INT_ST_REG     (*(volatile uint32_t *)(UART0_BASE + 0x08))
#define UART_INT_ENA_REG    (*(volatile uint32_t *)(UART0_BASE + 0x0C))
#define UART_INT_CLR_REG    (*(volatile uint32_t *)(UART0_BASE + 0x10))
#define UART_CLKDIV_REG     (*(volatile uint32_t *)(UART0_BASE + 0x14))
#define UART_STATUS_REG     (*(volatile uint32_t *)(UART0_BASE + 0x1C))
#define UART_CONF0_REG      (*(volatile uint32_t *)(UART0_BASE + 0x20))
#define UART_CONF1_REG      (*(volatile uint32_t *)(UART0_BASE + 0x24))
#define UART_LOWPULSE_REG   (*(volatile uint32_t *)(UART0_BASE + 0x28))
#define UART_HIGHPULSE_REG  (*(volatile uint32_t *)(UART0_BASE + 0x2C))
#define UART_ID_REG         (*(volatile uint32_t *)(UART0_BASE + 0x30))

/* UART FIFO bits */
#define UART_FIFO_TX_DATA_SHIFT  0
#define UART_FIFO_RX_DATA_SHIFT  0
#define UART_FIFO_TX_DATA_MASK   0xFF

/* UART CLKDIV: baud = APB_CLK / CLKDIV */
/* APB_CLK = 80MHz, 115200 baud → CLKDIV = 80M/115200 ≈ 694.44 */
#define UART_CLKDIV_VAL_115200   694
#define UART_CLKDIV_FRAG_115200  4  /* fractional part */

/* UART CONF0 bits */
#define UART_TXFIFO_RST     (1 << 0)
#define UART_RXFIFO_RST     (1 << 1)
#define UART_TXD_BRK        (1 << 8)

/* UART STATUS bits */
#define UART_TXFIFO_CNT_SHIFT  16
#define UART_TXFIFO_CNT_MASK   0xFF
#define UART_RXFIFO_CNT_SHIFT  0
#define UART_RXFIFO_CNT_MASK   0xFF

/* UART INT bits */
#define UART_TXFIFO_EMPTY_INT  (1 << 0)
#define UART_RXFIFO_FULL_INT   (1 << 0)

/* ============================================================
 * GPIO Registers
 * ============================================================ */
#define GPIO_BASE       0x3FF44000

#define GPIO_OUT_REG        (*(volatile uint32_t *)(GPIO_BASE + 0x04))
#define GPIO_OUT_W1TS_REG   (*(volatile uint32_t *)(GPIO_BASE + 0x08))
#define GPIO_OUT_W1TC_REG   (*(volatile uint32_t *)(GPIO_BASE + 0x0C))
#define GPIO_ENABLE_REG     (*(volatile uint32_t *)(GPIO_BASE + 0x20))
#define GPIO_ENABLE_W1TS    (*(volatile uint32_t *)(GPIO_BASE + 0x24))
#define GPIO_ENABLE_W1TC    (*(volatile uint32_t *)(GPIO_BASE + 0x28))
#define GPIO_IN_REG         (*(volatile uint32_t *)(GPIO_BASE + 0x3C))

/* GPIO pin mux (IO_MUX) */
#define IO_MUX_BASE     0x3FF49000

#define IO_MUX_GPIO0_REG   (*(volatile uint32_t *)(IO_MUX_BASE + 0x44))
#define IO_MUX_GPIO1_REG   (*(volatile uint32_t *)(IO_MUX_BASE + 0x48))
#define IO_MUX_GPIO2_REG   (*(volatile uint32_t *)(IO_MUX_BASE + 0x4C))
#define IO_MUX_GPIO3_REG   (*(volatile uint32_t *)(IO_MUX_BASE + 0x50))

/* IO_MUX function bits */
#define IO_MUX_FUNC_SHIFT   12
#define IO_MUX_FUNC_U0TXD   0  /* function 0 for GPIO1 = U0TXD */
#define IO_MUX_FUNC_U0RXD   0  /* function 0 for GPIO3 = U0RXD */
#define IO_MUX_MCU_SEL(x)   ((x) << 12)
#define IO_MUX_PU            (1 << 7)  /* pull up */
#define IO_MUX_PD            (1 << 6)  /* pull down */

/* ============================================================
 * Timer Group 0 Registers
 * ============================================================ */
#define TIMER_GROUP0_BASE   0x3FF5F000

#define TG0_T0CONFIG_REG    (*(volatile uint32_t *)(TIMER_GROUP0_BASE + 0x00))
#define TG0_T0LO_REG        (*(volatile uint32_t *)(TIMER_GROUP0_BASE + 0x04))
#define TG0_T0HI_REG        (*(volatile uint32_t *)(TIMER_GROUP0_BASE + 0x08))
#define TG0_T0UPDATE_REG    (*(volatile uint32_t *)(TIMER_GROUP0_BASE + 0x0C))
#define TG0_T0ALARMLO_REG   (*(volatile uint32_t *)(TIMER_GROUP0_BASE + 0x10))
#define TG0_T0ALARMHI_REG   (*(volatile uint32_t *)(TIMER_GROUP0_BASE + 0x14))
#define TG0_T0LOADLO_REG    (*(volatile uint32_t *)(TIMER_GROUP0_BASE + 0x18))
#define TG0_T0LOADHI_REG    (*(volatile uint32_t *)(TIMER_GROUP0_BASE + 0x1C))
#define TG0_T0LOAD_REG      (*(volatile uint32_t *)(TIMER_GROUP0_BASE + 0x20))
#define TG0_INT_CLR_REG     (*(volatile uint32_t *)(TIMER_GROUP0_BASE + 0x3C))
#define TG0_INT_RAW_REG     (*(volatile uint32_t *)(TIMER_GROUP0_BASE + 0x40))
#define TG0_INT_ST_REG      (*(volatile uint32_t *)(TIMER_GROUP0_BASE + 0x44))
#define TG0_INT_ENA_REG     (*(volatile uint32_t *)(TIMER_GROUP0_BASE + 0x48))

/* Timer CONFIG bits */
#define TIMER_EN            (1 << 31)
#define TIMER_INCREASE      (1 << 30)
#define TIMER_AUTORELOAD    (1 << 29)
#define TIMER_DIVIDER_SHIFT 13
#define TIMER_DIVIDER_MASK  0xFFFF
#define TIMER_EDGE_INT      (1 << 12)
#define TIMER_LEVEL_INT     (0 << 12)
#define TIMER_ALARM_EN      (1 << 10)

/* Timer interrupt bit */
#define TG0_T0_INT_CLR      (1 << 0)

/* ============================================================
 * Interrupt Controller (INTC)
 * ============================================================ */
#define INTC_BASE       0x3FFC0000

/* Interrupt matrix - map peripheral interrupts to CPU interrupts */
#define INTPRI_BASE     0x3FFC0000

#define INTPRI_CORE0_INT_MAP_REG(n)  (*(volatile uint32_t *)(INTPRI_BASE + 0x000 + (n) * 4))

/* CPU interrupt enable/unenable */
#define INTPRI_CORE0_CPU_INT_ENABLE_REG     (*(volatile uint32_t *)(INTPRI_BASE + 0x028))
#define INTPRI_CORE0_CPU_INT_CLEAR_REG      (*(volatile uint32_t *)(INTPRI_BASE + 0x02C))
/* CPU interrupt priority: 4 registers, 8 priorities each */
#define INTPRI_CORE0_CPU_INT_PRI_REG(n)     (*(volatile uint32_t *)(INTPRI_BASE + 0x030 + (n) * 4))

/* CPU interrupt type (edge=1, level=0): 4 registers */
#define INTPRI_CORE0_CPU_INT_TYPE_REG(n)    (*(volatile uint32_t *)(INTPRI_BASE + 0x040 + (n) * 4))
/* CPU interrupt clear: 4 registers */
#define INTPRI_CORE0_CPU_INT_EOI_REG(n)     (*(volatile uint32_t *)(INTPRI_BASE + 0x050 + (n) * 4))
/* Interrupt status */
#define INTPRI_CORE0_CPU_INT_THRESH_REG     (*(volatile uint32_t *)(INTPRI_BASE + 0x05C))
#define INTPRI_CORE0_INTERRUPT_REG          (*(volatile uint32_t *)(INTPRI_BASE + 0x060))

/* Peripheral interrupt source numbers */
#define ETS_TG0_T0_LEVEL_INTR_SOURCE    46  /* Timer Group 0 Timer 0 */

/* CPU interrupt numbers (we use 1-7 for levels) */
#define CPU_INTC_TIMER   6  /* CPU interrupt 6 for timer */

/* ============================================================
 * System Registers
 * ============================================================ */
#define SYS_BASE        0x3FF46000

#define SYS_CLK_CONF_REG    (*(volatile uint32_t *)(SYS_BASE + 0x000))
#define SYS_WIFI_CLK_EN_REG (*(volatile uint32_t *)(SYS_BASE + 0x014))
#define SYS_CORE_RST_REG    (*(volatile uint32_t *)(0x3FF48034))

/* APB_CLK = 80 MHz (derived from PLL) */
#define APB_CLK_FREQ    80000000UL

/* ============================================================
 * Xtensa Special Registers
 * ============================================================ */
/* CCOMPARE registers for timer interrupts */
#define CCOMPARE_0      0x200E
#define CCOMPARE_1      0x200F
#define CCOMPARE_2      0x2010

/* Exception registers */
#define EXCCAUSE        0x00E8
#define EPC1            0x00B1
#define EPS1            0x00B2
#define EXCSAVE1        0x00D1
#define DEPC            0x00C2
#define EXCVADDR        0x00C8

/* Interrupt enable / status */
#define INTENABLE       0x2004
#define INTERRUPT       0x2003
#define PS              0x2006
#define ICOUNT          0x2008
#define ICOUNTLEVEL     0x2009
#define CCOUNT          0x200A
#define VECBASE         0x2007

/* PS register bits */
#define PS_INTLEVEL_SHIFT   0
#define PS_INTLEVEL_MASK    0x0F
#define PS_EXCM_SHIFT       4
#define PS_UM_SHIFT         5  /* user mode bit */

/* ============================================================
 * DPORT Registers (clock/power management)
 * ============================================================ */
#define DPORT_BASE      0x3FF00000

#define DPORT_PERI_CLK_EN_REG       (*(volatile uint32_t *)(DPORT_BASE + 0x0C0))
#define DPORT_PERI_RST_EN_REG       (*(volatile uint32_t *)(DPORT_BASE + 0x0C4))

/* Timer group clock enable */
#define DPORT_TIMG0_CLK_EN          (1 << 13)
#define DPORT_TIMG0_RST             (1 << 13)

/* UART clock enable (already enabled by ROM) */
#define DPORT_UART0_CLK_EN          (1 << 24)

#endif /* ESP32_REGS_H */
