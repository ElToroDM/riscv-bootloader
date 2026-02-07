#include "boot.h"

/*
 * Flash Abstraction Layer
 * 
 * Provides safe flash operations with bounds checking and application-level API
 * Protects against accidental bootloader corruption
 */

int flash_write(uint32_t addr, const void *data, size_t size) {
    /* Defensive checks: ensure write is within APP partition */
    if (addr < APP_BASE || (addr + size) > (APP_BASE + APP_MAX_SIZE)) {
        return -1;
    }
    
    /* Delegate to platform implementation */
    return platform_flash_write(addr, data, size);
}

int flash_erase_app(void) {
    /* Erase entire application partition */
    return platform_flash_erase(APP_BASE, APP_MAX_SIZE);
}

int flash_write_header(const fw_header_t *header) {
    /* Write header at the beginning of APP partition */
    /* This should be atomic to prevent invalid state on power loss */
    return platform_flash_write(APP_BASE, header, sizeof(fw_header_t));
}
