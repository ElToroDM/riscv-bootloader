/*
 * Minimal Test Application
 * 
 * Demonstrates successful boot from bootloader.
 * Runs at APP_BASE + sizeof(fw_header_t) after bootloader validates firmware.
 */

#include <stdint.h>
#include <stddef.h>

/* Linker-provided symbols */
extern uint8_t __bss_end;

#define UART0_BASE 0x10000000u
#define UART_THR   0u
#define UART_LSR   5u
#define UART_LSR_TX_IDLE 0x20u

static inline volatile uint8_t *uart_reg(uint32_t reg) {
    return (volatile uint8_t *)(uintptr_t)(UART0_BASE + reg);
}

static void uart_putc_raw(char c) {
    while (((*uart_reg(UART_LSR)) & UART_LSR_TX_IDLE) == 0u) {
    }
    *uart_reg(UART_THR) = (uint8_t)c;
}

static void uart_puts_raw(const char *text) {
    while (*text) {
        if (*text == '\n') {
            uart_putc_raw('\r');
        }
        uart_putc_raw(*text++);
    }
}

static void uart_put_u32_dec(uint32_t value) {
    char buf[11];
    int i = 0;

    if (value == 0u) {
        uart_putc_raw('0');
        return;
    }

    while (value > 0u && i < (int)sizeof(buf)) {
        buf[i++] = (char)('0' + (value % 10u));
        value /= 10u;
    }

    while (i > 0) {
        uart_putc_raw(buf[--i]);
    }
}

static void uart_put_hex_nibble(uint8_t value) {
    value &= 0x0Fu;
    if (value < 10u) {
        uart_putc_raw((char)('0' + (char)value));
    } else {
        uart_putc_raw((char)('A' + (char)(value - 10u)));
    }
}

static void uart_put_u32_hex(uint32_t value) {
    uart_puts_raw("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        uart_put_hex_nibble((uint8_t)(value >> shift));
    }
}

static void uart_put_ptr_hex(uintptr_t value) {
    uart_puts_raw("0x");
    for (int shift = (int)(sizeof(uintptr_t) * 8u) - 4; shift >= 0; shift -= 4) {
        uart_put_hex_nibble((uint8_t)(value >> shift));
    }
}

static int is_little_endian(void) {
    const uint32_t v = 1u;
    return *((const uint8_t *)&v) == 1u;
}

static inline uint32_t read_csr_misa(void) {
    uint32_t misa;
    __asm__ volatile ("csrr %0, misa" : "=r"(misa));
    return misa;
}

static inline uintptr_t read_sp(void) {
    uintptr_t sp;
    __asm__ volatile ("mv %0, sp" : "=r"(sp));
    return sp;
}

static void print_isa_extensions(uint32_t misa) {
    /* RISC-V misa: bits [25:0] indicate extension presence (A-Z) */
    int found = 0;
    for (int i = 0; i < 26; i++) {
        if (misa & (1u << i)) {
            if (found) uart_putc_raw(',');
            uart_putc_raw((char)('A' + i));
            found = 1;
        }
    }
    
    if (!found) {
        uart_puts_raw("none");
    }
    uart_puts_raw("\n");
}

/* Read common CSRs for runtime metrics */
static inline uint32_t read_csr_mhartid(void) {
    uint32_t v;
    __asm__ volatile ("csrr %0, mhartid" : "=r"(v));
    return v;
}

void app_main(void) {
    uart_puts_raw("APP_BOOT\n");
    uart_puts_raw("========================================\n");
    uart_puts_raw("   Test Application Running\n");
    uart_puts_raw("   Successfully handed off from bootloader!\n");
    uart_puts_raw("========================================\n\n");

    uart_puts_raw("Runtime Info:\n");
    uart_puts_raw("  XLEN:       ");
    uart_put_u32_dec((uint32_t)(sizeof(uintptr_t) * 8u));
    uart_puts_raw(" bits\n");

    uart_puts_raw("  Endianness: ");
    uart_puts_raw(is_little_endian() ? "Little" : "Big");
    uart_puts_raw("\n");

    uart_puts_raw("  UART0:      ");
    uart_put_u32_hex((uint32_t)UART0_BASE);
    uart_puts_raw("\n\n");

    uart_puts_raw("Memory Layout:\n");
    uart_puts_raw("  app_main:   ");
    uart_put_ptr_hex((uintptr_t)app_main);
    uart_puts_raw("\n");

    uintptr_t current_sp = read_sp();
    uart_puts_raw("  Stack (SP): ");
    uart_put_ptr_hex(current_sp);
    uart_puts_raw("\n");

    uintptr_t heap_start = (uintptr_t)&__bss_end;
    uart_puts_raw("  Heap start: ");
    uart_put_ptr_hex(heap_start);
    uart_puts_raw("\n");

    if (current_sp > heap_start) {
        uint32_t stack_margin = (uint32_t)(current_sp - heap_start);
        uart_puts_raw("  Stack margin: ~");
        uart_put_u32_dec(stack_margin);
        uart_puts_raw(" bytes\n");
    }

    uart_puts_raw("\nISA Profile:\n");
    uint32_t misa = read_csr_misa();
    uart_puts_raw("  MISA:       ");
    uart_put_u32_hex(misa);
    uart_puts_raw("\n");
    uart_puts_raw("  Extensions: ");
    print_isa_extensions(misa);

    uart_puts_raw("  Hart ID:    ");
    uart_put_u32_dec(read_csr_mhartid());
    uart_puts_raw("\n\n");

    uart_puts_raw("App: online\n");

    while (1) {
    }
}
