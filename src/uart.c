#include "boot.h"

/*
 * UART Abstraction Layer
 * 
 * Provides generic UART interface with terminal-friendly formatting
 * Delegates hardware control to platform-specific implementation
 */

void uart_init(void) {
    /* Early platform initialization (clocks, power, etc.) */
    platform_early_init();
    
    /* UART-specific hardware setup */
    platform_uart_init();
}

void uart_putc(char c) {
    /* Normalize line endings for terminal compatibility */
    if (c == '\n') {
        platform_uart_putc('\r');
    }
    platform_uart_putc(c);
}

char uart_getc(void) {
    return platform_uart_getc();
}

void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}
