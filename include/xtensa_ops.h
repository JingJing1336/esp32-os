#ifndef XTENSA_OPS_H
#define XTENSA_OPS_H

#include "types.h"

/* ============================================================
 * Xtensa LX6 Special Register Access
 * ============================================================ */

/* Read special register */
#define WSR(reg, val)   __asm__ volatile("wsr %0, " #reg : : "a"(val))
#define RSR(reg, val)   __asm__ volatile("rsr %0, " #reg : "=a"(val))

/* Read/write specific registers */
static inline uint32_t xtensa_get_ps(void) {
    uint32_t ps;
    RSR(PS, ps);
    return ps;
}

static inline void xtensa_set_ps(uint32_t ps) {
    WSR(PS, ps);
}

static inline uint32_t xtensa_get_intenable(void) {
    uint32_t val;
    RSR(INTENABLE, val);
    return val;
}

static inline void xtensa_set_intenable(uint32_t val) {
    WSR(INTENABLE, val);
}

static inline uint32_t xtensa_get_interrupt(void) {
    uint32_t val;
    RSR(INTERRUPT, val);
    return val;
}

static inline uint32_t xtensa_get_ccompare(int n) {
    uint32_t val;
    switch (n) {
        case 0: RSR(CCOMPARE_0, val); break;
        case 1: RSR(CCOMPARE_1, val); break;
        case 2: RSR(CCOMPARE_2, val); break;
    }
    return val;
}

static inline void xtensa_set_ccompare(int n, uint32_t val) {
    switch (n) {
        case 0: WSR(CCOMPARE_0, val); break;
        case 1: WSR(CCOMPARE_1, val); break;
        case 2: WSR(CCOMPARE_2, val); break;
    }
}

static inline uint32_t xtensa_get_sar(void) {
    uint32_t sar;
    __asm__ volatile("rsr %0, SAR" : "=a"(sar));
    return sar;
}

static inline void xtensa_set_sar(uint32_t sar) {
    __asm__ volatile("wsr %0, SAR" : : "a"(sar));
}

/* ============================================================
 * Interrupt Control
 * ============================================================ */

/* Disable all interrupts, return previous PS */
static inline uint32_t irq_disable_save(void) {
    uint32_t ps;
    __asm__ volatile(
        "rsr.ps %0\n"
        "movi a3, 0\n"
        "wsr.ps a3\n"
        "rsync\n"
        : "=a"(ps) : : "a3"
    );
    return ps;
}

/* Restore PS (including interrupt level) */
static inline void irq_restore(uint32_t ps) {
    xtensa_set_ps(ps);
    __asm__ volatile("rsync");
}

/* Enable interrupts globally */
static inline void irq_global_enable(void) {
    uint32_t ps = xtensa_get_ps();
    ps |= (1 << 0);  /* INTLEVEL = 1 enables level-1 interrupts */
    xtensa_set_ps(ps);
    __asm__ volatile("rsync");
}

/* Disable interrupts globally */
static inline void irq_global_disable(void) {
    uint32_t ps = xtensa_get_ps();
    ps &= ~0xF;  /* INTLEVEL = 0 means all disabled... actually we want max */
    ps |= 0xF;   /* INTLEVEL = 15 disables all */
    xtensa_set_ps(ps);
    __asm__ volatile("rsync");
}

/* NOP and sync */
static inline void xtensa_nop(void) {
    __asm__ volatile("nop");
}

static inline void xtensa_rsync(void) {
    __asm__ volatile("rsync");
}

/* Memory barrier */
static inline void xtensa_memw(void) {
    __asm__ volatile("memw");
}

#endif /* XTENSA_OPS_H */
