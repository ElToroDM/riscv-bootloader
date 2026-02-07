#include "boot.h"

/* Professional RISC-V UART Bootloader - Main Logic */

static void print_banner(void) {
    uart_puts("\n\r======================================\n\r");
    uart_puts("   Professional RISC-V Bootloader    \n\r");
    uart_puts("   Target: " PLATFORM_NAME "        \n\r");
    uart_puts("======================================\n\r");
}

static int validate_app(void) {
    const fw_header_t *header = (const fw_header_t *)APP_BASE;
    
    if (header->magic != BOOT_MAGIC) {
        uart_puts("Error: Invalid magic number\n\r");
        return -1;
    }
    
    if (header->size == 0 || header->size > APP_MAX_SIZE - sizeof(fw_header_t)) {
        uart_puts("Error: Invalid firmware size\n\r");
        return -1;
    }
    
    uint32_t calc_crc = crc32((const uint8_t *)(APP_BASE + sizeof(fw_header_t)), header->size);
    if (calc_crc != header->crc32) {
        uart_puts("Error: CRC mismatch\n\r");
        return -1;
    }
    
    return 0;
}

static void jump_to_app(void) {
    uart_puts("Jumping to application...\n\r");
    
    /* The application entry point is right after the header */
    void (*app_entry)(void) = (void (*)(void))(APP_BASE + sizeof(fw_header_t));
    
    /* Basic cleanup before jump */
    /* (In more complex loaders, we'd disable peripherals/interrupts here) */
    
    app_entry();
}

static void uart_update(void) {
    uint32_t size = 0;

    uart_puts("OK\n\r");
    
    /* Expecting SEND <size> */
    /* Simple parser for "SEND " */
    const char *cmd = "SEND ";
    for (int j = 0; j < 5; j++) {
        if (uart_getc() != cmd[j]) {
            uart_puts("ERR: CMD\n\r");
            return;
        }
    }
    
    /* Read size */
    while (1) {
        char c = uart_getc();
        if (c == '\r' || c == '\n') break;
        if (c >= '0' && c <= '9') {
            size = size * 10 + (c - '0');
        }
    }
    
    if (size == 0 || size > APP_MAX_SIZE - sizeof(fw_header_t)) {
        uart_puts("ERR: SIZE\n\r");
        return;
    }

    /* Prepare Header */
    fw_header_t header;
    header.magic = BOOT_MAGIC;
    header.size = size;
    header.version = 1;

    /* Erase App area using HAL */
    uart_puts("ERASING...\n\r");
    if (flash_erase_app() != 0) {
        uart_puts("ERR: ERASE\n\r");
        return;
    }

    /* Receive raw binary and write to flash */
    uart_puts("READY\n\r");
    uint8_t *dest = (uint8_t *)(APP_BASE + sizeof(fw_header_t));
    for (uint32_t j = 0; j < size; j++) {
        dest[j] = (uint8_t)uart_getc();
    }

    /* Calculate CRC of received data */
    header.crc32 = crc32((const uint8_t *)(APP_BASE + sizeof(fw_header_t)), size);
    
    /* Write header last for atomicity using HAL */
    if (flash_write_header(&header) != 0) {
        uart_puts("ERR: HEADER\n\r");
        return;
    }

    uart_puts("CRC?\n\r");
    uart_puts("OK\n\r");
    uart_puts("REBOOT\n\r");
    
    platform_reset();
}

int main(void) {
    uart_init();
    print_banner();

    uart_puts("BOOT?\n\r");
    
    /* Wait for user choice with echo */
    while(1) {
        char choice = uart_getc();
        uart_putc(choice); /* Echo for visibility */

        if (choice == 'u' || choice == 'U') {
            uart_update();
        } else if (choice == '\r' || choice == '\n') {
            /* Try to boot if user presses Enter */
            break;
        } else {
            /* For any other key, show a small hint if app validation fails later */
            break;
        }
    }

    if (validate_app() == 0) {
        jump_to_app();
    } else {
        uart_puts("Recovery Loop: No valid app found. Press 'u' to update.\n\r");
        while(1) {
            if (uart_getc() == 'u') {
                uart_update();
            }
        }
    }

    return 0;
}
