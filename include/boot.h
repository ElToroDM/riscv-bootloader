#ifndef BOOT_H
#define BOOT_H

#include <stdint.h>
#include <stddef.h>

/* Platform-specific configuration - Import from board */
#include "../boards/qemu_virt/platform.h"

/* Bootloader Configuration */
#define BOOT_MAGIC          0x5256424C /* "RVBL" */

/* Firmware Header */
typedef struct {
    uint32_t magic;         /* Must be BOOT_MAGIC */
    uint32_t size;          /* Body size in bytes */
    uint32_t crc32;         /* CRC32 of the body */
    uint32_t version;       /* Firmware version */
} fw_header_t;

/* =============================================================================
 * HAL Layer 1: Platform-Specific (implemented in boards/<board>/platform.c)
 * ============================================================================= */

/**
 * platform_early_init - Early hardware initialization
 * 
 * Called first, before any other initialization. Use for:
 * - Clock/PLL configuration
 * - Power management setup
 * - Critical system registers
 * 
 * Note: Should not depend on BSS being cleared or data being initialized
 */
void platform_early_init(void);

/**
 * platform_uart_init - Initialize UART hardware
 * 
 * Configure UART peripheral (baudrate, format, etc.)
 * Must be ready for putc/getc after this call
 */
void platform_uart_init(void);

/**
 * platform_uart_putc - Send one character via UART
 * @c: Character to send
 * 
 * Blocks until TX is ready. No buffering.
 */
void platform_uart_putc(char c);

/**
 * platform_uart_getc - Receive one character via UART
 * 
 * Blocks until RX has data. No buffering.
 * Returns: Character received
 */
char platform_uart_getc(void);

/**
 * platform_flash_write - Write to flash memory
 * @addr: Absolute physical address to write
 * @data: Source buffer
 * @size: Number of bytes to write
 * 
 * Requirements for real hardware:
 * - addr/size must respect flash page alignment
 * - Target area must be erased first
 * - May need to disable cache/interrupts during write
 * 
 * Returns: 0 on success, -1 on error
 */
int platform_flash_write(uint32_t addr, const void *data, size_t size);

/**
 * platform_flash_erase - Erase flash memory
 * @addr: Absolute physical address (must be sector-aligned)
 * @size: Number of bytes to erase (must be multiple of sector size)
 * 
 * Requirements for real hardware:
 * - addr must be sector-aligned
 * - size must be multiple of sector size
 * - May take significant time (ms to seconds)
 * - May need to disable interrupts/watchdog
 * 
 * Returns: 0 on success, -1 on error
 */
int platform_flash_erase(uint32_t addr, size_t size);

/**
 * platform_reset - Perform system reset
 * 
 * This function should not return
 */
void platform_reset(void);

/* =============================================================================
 * HAL Layer 2: Generic Hardware Abstraction (implemented in src/)
 * ============================================================================= */

/**
 * uart_init - Initialize UART subsystem
 * 
 * Performs platform-specific early init and UART setup
 */
void uart_init(void);

/**
 * uart_putc - Send one character (with newline normalization)
 * @c: Character to send
 * 
 * Converts \n to \r\n automatically for terminal compatibility
 */
void uart_putc(char c);

/**
 * uart_getc - Receive one character
 * 
 * Returns: Character received from UART
 */
char uart_getc(void);

/**
 * uart_puts - Send a null-terminated string
 * @s: String to send
 */
void uart_puts(const char *s);

/**
 * flash_write - Safe flash write with bounds checking
 * @addr: Address to write (must be within APP region)
 * @data: Source buffer
 * @size: Number of bytes to write
 * 
 * Returns: 0 on success, -1 if out of bounds or write fails
 */
int flash_write(uint32_t addr, const void *data, size_t size);

/**
 * flash_erase_app - Erase entire application partition
 * 
 * Returns: 0 on success, -1 on error
 */
int flash_erase_app(void);

/**
 * flash_write_header - Write firmware header atomically
 * @header: Header structure to write
 * 
 * Writes header to beginning of APP partition
 * Returns: 0 on success, -1 on error
 */
int flash_write_header(const fw_header_t *header);

/* =============================================================================
 * Utility Functions
 * ============================================================================= */

/**
 * crc32 - Calculate CRC32 checksum
 * @data: Data buffer
 * @len: Length in bytes
 * 
 * Returns: CRC32 value
 */
uint32_t crc32(const uint8_t *data, size_t len);

/* Helper macros */
#define UNUSED(x) (void)(x)

#endif /* BOOT_H */
