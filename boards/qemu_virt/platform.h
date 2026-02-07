#ifndef PLATFORM_H
#define PLATFORM_H

/*
 * QEMU RISC-V Virt Platform Configuration
 * 
 * This file contains hardware-specific constants for the target platform.
 * When porting to real hardware, create a new board directory with its own platform.h
 */

/* Memory Map - Adjust for your target hardware */
#define FLASH_BASE          0x80000000
#define APP_BASE            0x80010000
#define FLASH_SIZE          (64 * 1024)
#define APP_MAX_SIZE        (448 * 1024)

/* UART Configuration */
#define UART0_BASE          0x10000000
#define UART_BAUDRATE       115200

/* Platform Identification */
#define PLATFORM_NAME       "QEMU Virt (RV32IM)"

#endif /* PLATFORM_H */
