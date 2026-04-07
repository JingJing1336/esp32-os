#include "uart.h"
#include "esp32_regs.h"

void uart_init(void)
{
    /*
     * UART0 is already initialized by the ROM bootloader for serial
     * communication at 115200 baud. We just verify/adjust settings.
     *
     * GPIO1 = U0TXD, GPIO3 = U0RXD (already configured by ROM)
     */

    /* Set baud rate to 115200 (APB_CLK = 80MHz) */
    UART_CLKDIV_REG = (UART_CLKDIV_VAL_115200 & 0xFFFFF)
                    | ((UART_CLKDIV_FRAG_115200 & 0xF) << 20);

    /* Reset TX/RX FIFOs */
    UART_CONF0_REG |= UART_TXFIFO_RST | UART_RXFIFO_RST;
    UART_CONF0_REG &= ~(UART_TXFIFO_RST | UART_RXFIFO_RST);

    /* 8 data bits, no parity, 1 stop bit (default in CONF0) */
}

void uart_putc(char c)
{
    /* Wait until TX FIFO has space (TXFIFO_CNT < 128) */
    while ((UART_STATUS_REG >> UART_TXFIFO_CNT_SHIFT & UART_TXFIFO_CNT_MASK) >= 128)
        ;
    UART_FIFO_REG = (uint32_t)c;
}

void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n')
            uart_putc('\r');
        uart_putc(*s++);
    }
}

char uart_getc(void)
{
    /* Wait until RX FIFO has data */
    while ((UART_STATUS_REG & UART_RXFIFO_CNT_MASK) == 0)
        ;
    return (char)(UART_FIFO_REG & 0xFF);
}

int uart_try_getc(char *c)
{
    if ((UART_STATUS_REG & UART_RXFIFO_CNT_MASK) > 0) {
        *c = (char)(UART_FIFO_REG & 0xFF);
        return 1;
    }
    return 0;
}
