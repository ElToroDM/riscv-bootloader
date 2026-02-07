#include "boot.h"

/* 
 * QEMU Virt Platform Implementation
 * 
 * Hardware: QEMU RISC-V 'virt' machine
 * UART: 16550A compatible at 0x10000000
 * 
 * PORTING NOTES:
 * - Real hardware will need clock/PLL init in platform_early_init()
 * - Flash operations here are simplified (direct RAM access)
 * - Real flash needs: sector erase, page write, status polling, write enable
 * - Consider adding watchdog disable in early_init for long operations
 */

#define UART0_BASE 0x10000000

/* Use explicit volatile cast to prevent compiler optimizations on register pooling */
#define UART_REG(r) (*(volatile uint8_t *)(UART0_BASE + (r)))

#define UART_THR 0
#define UART_RBR 0
#define UART_IER 1
#define UART_FCR 2
#define UART_LCR 3
#define UART_LSR 5

#define UART_LSR_RX_READY 0x01
#define UART_LSR_TX_IDLE  0x20

/* =============================================================================
 * Platform Initialization
 * ============================================================================= */

void platform_early_init(void) {
    /* 
     * Early hardware setup - called before BSS clear
     * 
     * For real hardware, add here:
     * - System clock/PLL configuration
     * - Power domain enablement
     * - Watchdog disable (if needed for long flash operations)
     * - Critical GPIO configuration
     * 
     * QEMU: No clock setup needed
     */
}

/* =============================================================================
 * UART Implementation
 * ============================================================================= */

void platform_uart_init(void) {
    /* 
     * 16550A UART Initialization
     * 
     * For real hardware with different UART:
     * - Configure baudrate generator
     * - Set data format (8N1)
     * - Enable FIFOs if available
     * - Configure GPIO pins for UART function
     */
    UART_REG(UART_IER) = 0x00; /* Disable interrupts */
    UART_REG(UART_LCR) = 0x03; /* 8N1 */
    UART_REG(UART_FCR) = 0x07; /* Enable FIFO, clear TX/RX */
}

void platform_uart_putc(char c) {
    /* Wait for TX to be empty */
    while (!(UART_REG(UART_LSR) & UART_LSR_TX_IDLE));
    UART_REG(UART_THR) = (uint8_t)c;
}

char platform_uart_getc(void) {
    /* Wait for RX to be ready */
    while (!(UART_REG(UART_LSR) & UART_LSR_RX_READY));
    return (char)UART_REG(UART_RBR);
}

/* =============================================================================
 * Flash Implementation
 * ============================================================================= */

int platform_flash_write(uint32_t addr, const void *data, size_t size) {
    /*
     * QEMU: Direct memory write (RAM-backed)
     * 
     * For real SPI/embedded flash:
     * 1. Send write enable command
     * 2. Wait for flash ready (WIP bit)
     * 3. Write data in page-aligned chunks (typically 256 bytes)
     * 4. Poll status register until write completes
     * 5. Verify if critical
     * 
     * Requirements:
     * - addr must be aligned to flash page size
     * - size should be multiple of page size (or handle partial pages)
     * - Target must be erased first (all 0xFF)
     * - May need to disable interrupts during write
     */
    uint8_t *dest = (uint8_t *)(uintptr_t)addr;
    const uint8_t *src = (const uint8_t *)data;
    for (size_t i = 0; i < size; i++) {
        dest[i] = src[i];
    }
    return 0;
}

int platform_flash_erase(uint32_t addr, size_t size) {
    /*
     * QEMU: Fill with 0xFF (simulated erase)
     * 
     * For real SPI/embedded flash:
     * 1. Send write enable command
     * 2. Send sector/block erase command
     * 3. Wait for erase complete (can take ms to seconds)
     * 4. Repeat for each sector in range
     * 
     * Requirements:
     * - addr must be sector-aligned (typical: 4KB sectors)
     * - size must be multiple of sector size
     * - May need to disable watchdog for large erases
     * - Consider feeding watchdog between sectors
     * 
     * Typical sector sizes: 4KB (uniform) or mixed (4KB + 32KB + 64KB)
     */
    uint8_t *dest = (uint8_t *)(uintptr_t)addr;
    for (size_t i = 0; i < size; i++) {
        dest[i] = 0xFF;
    }
    return 0;
}

/* =============================================================================
 * System Control
 * ============================================================================= */

void platform_reset(void) {
    /*
     * QEMU: Write to test device for poweroff
     * 
     * For real hardware:
     * - Use watchdog reset
     * - Or write to system reset register
     * - Or call ROM bootloader function
     * 
     * Example for typical MCU:
     * SYSRESET->CTRL = RESET_KEY | RESET_REQ;
     */
    volatile uint32_t *test_device = (uint32_t *)0x100000;
    *test_device = 0x7777; /* QEMU poweroff/reset */
    while(1);
}
