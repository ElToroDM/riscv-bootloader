# riscv-bootloader

**Minimal bare-metal RISC-V UART bootloader**  
Assembly entry • CRC32 validation • Portable HAL • QEMU reference for real hardware ports

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Fast, minimal, open-source RISC-V bootloaders for ESP32-C3 and similar hardware — production-ready in days, fully auditable, no proprietary lock-in.

This is the **QEMU-validated reference implementation** — a clean base for porting to real RISC-V MCUs.

## Why this bootloader?

- **Auditability & simplicity**: C99 + assembly, no dynamic allocation, no interrupts, no frameworks — explicit and readable code.
- **Clear separation**: Bootloader / Application / Board-specific code fully isolated.
- **Portability**: Hardware abstraction layer (HAL) makes adaptation fast and safe.
- **Bottom-up approach**: Validated in QEMU virt → designed for real hardware.
- **Ownership**: Full source delivered — you control and adapt everything.

Perfect for:
- Hardware startups prototyping IoT devices
- Small OEMs with custom boards needing controlled boot behavior
- Makers & labs moving from Arduino to production-grade firmware
- Consultancies building complete firmware stacks

## Features

- Reliable bare-metal startup (assembly `_start` with stack/BSS init)
- Human-readable UART update protocol (115200 8N1)
- CRC32 + magic number + size firmware validation
- Compact footprint (default ~64 KB for bootloader)
- Clean handoff to application
- Portable HAL for easy porting

## Project Structure
```text
riscv-bootloader/
├── src/           # Core logic (OS-agnostic)
├── include/       # Definitions & HAL interfaces
├── linker/        # Linker scripts (memory.ld)
├── boards/        # Board-specific HAL
│   └── qemu_virt/ # Default: QEMU RISC-V Virt
├── Makefile       # Build system
└── README.md
```

## Memory Layout (QEMU virt default – adjustable)

| Region | Address | Size | Description |
| --- | --- | --- | --- |
| FLASH | 0x00000000 | 64 KB | Bootloader code |
| APP | 0x00010000 | 448 KB | Application binary partition |
| RAM | 0x80000000 | 128 KB | Runtime (stack, data, BSS) |

See `linker/memory.ld` and `include/boot.h` for details.

## Update Protocol (UART 115200 8N1)

1. Bootloader sends: `BOOT?`
2. Host sends any char (e.g. `u`) → enter update mode
3. Bootloader: `OK`
4. Host: `SEND <size>\n` (decimal size)
5. Bootloader: `READY` (after flash erase)
6. Host sends raw binary data
7. Bootloader: `CRC?` → `OK` → `REBOOT`

## Quick Start

Prerequisites: `riscv64-unknown-elf-gcc` toolchain (e.g. from Espressif or riscv-gnu-toolchain)

```bash
git clone https://github.com/ElToroDM/riscv-bootloader.git
cd riscv-bootloader

make                # Build ELF
make qemu           # Run in QEMU virt (see UART output)
```

Expected output example:
```text
BOOT?
[send 'u' to enter update]
OK
SEND 12345
READY
[send binary...]
CRC? OK
REBOOT
```

## Porting to Real Hardware

- Create `boards/<your_board>/`
- Implement `platform.c` with HAL functions from `include/boot.h` (`uart_init()`, `uart_putc()`, etc.)
- Optional: GPIO/LED init for signaling
- Adjust `linker/memory.ld` for real flash/RAM map
- Update `Makefile` for new target
- Build & flash with `esptool.py` / OpenOCD

## Status

- ✅ QEMU virt reference fully working

## Custom Adaptations Available

Need it tailored to your board?

- UART/USB firmware update for your specific hardware
- GPIO recovery trigger
- Custom LED patterns for status
- Quick delivery (1–3 days) with full source ownership

Contact me: Open an issue.

Let's build simple, dependable boot infrastructure — fast and yours.
