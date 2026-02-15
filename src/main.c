#include "boot.h"

/*
 * Professional RISC-V UART Bootloader - Main Logic
 *
 * Responsibilities:
 * - Present a simple UART-based update protocol
 * - Validate firmware image (magic, size, CRC)
 * - Perform flash erase/write via HAL and jump to the application
 *
 * Design goals: small, auditable, and explicit behavior for reviews
 */

static void print_banner(void) {
    /* Small human-friendly banner printed at boot */
    uart_puts("======================================\n");
    uart_puts("   Professional RISC-V Bootloader    \n");
    uart_puts("   Target: " PLATFORM_NAME "        \n");
    uart_puts("======================================\n");
}

/*
 * validate_app - Verify the firmware header at APP_BASE
 * Checks: magic number, plausibility of size, and CRC32 over payload
 * Returns 0 on success, -1 on failure
 */
static int validate_app(void) {
    const fw_header_t *header = (const fw_header_t *)APP_BASE;
    
    /* Basic header sanity checks */
    if (header->magic != BOOT_MAGIC) {
        uart_puts("Error: Invalid magic number\n");
        return -1;
    }
    
    /* Ensure reported size fits within the application partition */
    if (header->size == 0 || header->size > APP_MAX_SIZE - sizeof(fw_header_t)) {
        uart_puts("Error: Invalid firmware size\n");
        return -1;
    }
    
    /* Compute CRC and compare with header CRC */
    uint32_t calc_crc = crc32((const uint8_t *)(APP_BASE + sizeof(fw_header_t)), header->size);
    if (calc_crc != header->crc32) {
        uart_puts("Error: CRC mismatch\n");
        return -1;
    }
    
    return 0;
}

/*
 * jump_to_app - Transfer control to application entry point
 * Notes:
 * - The application entry is placed directly after the fw_header_t
 * - In a real loader you might: flush caches, disable interrupts, remap vectors
 */
static void jump_to_app(void) {
    uart_puts("Jumping to application...\n");
    uart_puts("APP_HANDOFF\n");

    /* The application entry point is right after the header */
    void (*app_entry)(void) = (void (*)(void))(APP_BASE + sizeof(fw_header_t));
    
    /* Basic cleanup before jump (kept minimal and explicit)
     * For safety you'd typically: disable IRQs, turn off peripherals, etc.
     */
    app_entry();
}

/*
 * uart_update - Implements the simple UART update protocol
 * Protocol (human-friendly):
 *  - Bootloader sends: OK
 *  - Host sends: SEND <size>\n
 *  - Bootloader: READY
 *  - Host sends raw binary of <size> bytes
 *  - Bootloader computes CRC, writes header atomically, and reboots
 *
 * Note: For QEMU this implementation writes payload directly to memory at
 * APP_BASE (simplified). On real hardware, writes must go through flash APIs
 * and respect page/sector alignment and write/erase constraints.
 */
static void uart_update(void) {
    uint32_t size = 0;

    uart_puts("OK\n");
    
    /* Expecting "SEND " literal (very simple parser) */
    const char *cmd = "SEND ";
    for (int j = 0; j < 5; j++) {
        if (uart_getc() != cmd[j]) {
            uart_puts("ERR: CMD\n");
            return;
        }
    }
    
    /* Read decimal size until newline */
    while (1) {
        char c = uart_getc();
        if (c == '\r' || c == '\n') break;
        if (c >= '0' && c <= '9') {
            size = size * 10 + (c - '0');
        }
    }
    
    /* Validate reported size against partition limits */
    if (size == 0 || size > APP_MAX_SIZE - sizeof(fw_header_t)) {
        uart_puts("ERR: SIZE\n");
        return;
    }

    /* Prepare header (header written last for atomic update) */
    fw_header_t header;
    header.magic = BOOT_MAGIC;
    header.size = size;
    header.version = 1;

    /* Erase application partition via HAL (may be time-consuming) */
    uart_puts("ERASING...\n");
    if (flash_erase_app() != 0) {
        uart_puts("ERR: ERASE\n");
        return;
    }

    /* Receive payload. QEMU: writes directly to memory for simplicity. */
    uart_puts("READY\n");
    uint8_t *dest = (uint8_t *)(APP_BASE + sizeof(fw_header_t));
    for (uint32_t j = 0; j < size; j++) {
        /* Blocking read per byte; keep it simple and deterministic */
        dest[j] = (uint8_t)uart_getc();
    }

    /* Compute CRC over received payload and store into header */
    header.crc32 = crc32((const uint8_t *)(APP_BASE + sizeof(fw_header_t)), size);
    
    /* Write header last to mark a valid firmware image atomically */
    if (flash_write_header(&header) != 0) {
        uart_puts("ERR: HEADER\n");
        return;
    }

    uart_puts("CRC?\n");
    uart_puts("OK\n");
    uart_puts("REBOOT\n");

#if PLATFORM_DIRECT_BOOT_AFTER_UPDATE
    /* QEMU demo flow: jump directly so UART can show app output immediately. */
    jump_to_app();
#else
    /* Perform system reset using platform abstraction */
    platform_reset();
#endif
}

int main(void) {
    /* Initialize UART subsystem and show a human-friendly banner */
    uart_init();
    print_banner();

    uart_puts("BOOT?\n");
    
    /* Wait for user decision. Echo character to improve UX over serial. */
    while(1) {
        char choice = uart_getc();
        uart_putc(choice); /* Echo for visibility */
        if (choice != '\r' && choice != '\n') {
            uart_puts("\n");
        }

        if (choice == 'u' || choice == 'U') {
            /* Enter firmware update mode */
            uart_update();
        } else if (choice == '\r' || choice == '\n') {
            /* Treat Enter as a request to boot the app */
            break;
        } else {
            /* Any other key falls through to attempt boot */
            break;
        }
    }

    /* Validate the on-flash application and jump if valid */
    if (validate_app() == 0) {
        jump_to_app();
    } else {
        /* If no valid app, stay in recovery mode and allow updates */
        uart_puts("Recovery Loop: No valid app found. Press 'u' to update.\n");
        while(1) {
            if (uart_getc() == 'u') {
                uart_update();
            }
        }
    }

    return 0;
}
