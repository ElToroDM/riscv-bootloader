# exerionbit-riscv-bootloader
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square)](https://opensource.org/licenses/MIT)&nbsp;&nbsp;
[![Build Status](https://github.com/ElToroDM/exerionbit-riscv-bootloader/actions/workflows/ci-build.yml/badge.svg?style=flat-square)](https://github.com/ElToroDM/exerionbit-riscv-bootloader/actions)&nbsp;&nbsp;
[![QEMU Validated](https://img.shields.io/badge/QEMU-Validated-success?style=flat-square&logo=qemu)](https://www.qemu.org/)&nbsp;&nbsp;
[![Size ~6KB](https://img.shields.io/badge/Size-%3C6KB-blue?style=flat-square)](https://github.com/ElToroDM/exerionbit-riscv-bootloader)&nbsp;&nbsp;
[![Custom Paid Ports](https://img.shields.io/badge/Custom-Paid%20Ports-brightgreen?style=flat-square)](https://github.com/ElToroDM/exerionbit-riscv-bootloader/issues)&nbsp;&nbsp;

**Minimal bare-metal RISC-V UART bootloader**  
Assembly entry • CRC32 validation • Portable HAL • QEMU reference for fast real-hardware ports (ESP32-C3 ready)

- Fast, minimal, open-source RISC-V bootloaders — production-ready in days, fully auditable, no proprietary lock-in.  
- This QEMU-validated reference is your clean, portable starting point for custom ESP32-C3 or similar MCUs.

## What this repository proves

- Minimal, auditable RISC-V boot path with explicit assembly-to-C startup
- Deterministic UART update baseline with CRC validation
- Reproducible protocol behavior in QEMU for early architecture validation

## What this repository does not prove

- Production secure boot hardening internals
- Production key lifecycle architecture
- Anti-tamper internals for adversarial environments

## Quick demo

```bash
make all
make qemu
python3 test_validator.py
```

## Known limits

- Default target is QEMU virt, not a specific production board
- Real hardware adaptation still requires board-specific HAL and memory map
- Visual LED contract is not part of this QEMU baseline

→ See [docs/KNOWN_LIMITS.md](docs/KNOWN_LIMITS.md) for the full list.

## Contact

- Open an issue for scoped bootloader adaptation work
- For alignment details, see `BOOT_SEQUENCE.md` and `VALIDATION_PROFILE.md`

## Quick Start

**Prerequisites:**
- RISC-V toolchain (`riscv-none-elf-gcc` or equivalent)
- GNU Make
- QEMU (system emulation)

**Build and run:**
```bash
make all        # Compile bootloader
make qemu       # Run in QEMU
```

**Run automated tests:**
```bash
python3 test_validator.py    # Protocol validation test
```

**Run live visual validation (Windows PowerShell):**
```powershell
.\scripts\run-test-with-uart-tail.ps1
```

**Demo capture mode (for terminal recording/GIF preparation):**
```powershell
.\scripts\run-test-with-uart-tail.ps1 -DemoMode
```

**Expected output:** Bootloader waits for UART input, displays `BOOT?`

> 📖 **Detailed setup instructions:** See [SETUP.md](SETUP.md)

---

## Why this bootloader?

- **Fast adaptation**: Portable HAL + clear separation enable ports in hours or days — ideal for IoT prototypes.
- **No vendor lock-in**: 100% open-source under the MIT License; no proprietary SDKs or vendor binaries.
- **Audit & debug friendly**: Explicit C and assembly with minimal abstractions — excellent for security reviews and compliance.
- **Proven in QEMU virt**: Validated in QEMU virt and designed bottom-up to migrate to real hardware without surprises.
- **Full ownership**: Complete source delivered so you can adapt, audit, and ship with confidence.

Perfect for:
- Hardware startups prototyping IoT devices
- Small OEMs with custom boards needing controlled boot behavior
- Makers & labs moving from Arduino to production-grade firmware
- Consultancies building complete firmware stacks

Perfect base for your next ESP32-C3 product — from prototype to small production runs.

## Why Choose This as Your Base?

- Ready for production: minimal, auditable, and portable code you can trust.
- Accelerates time-to-market: small, focused codebase reduces integration overhead.
- Low-risk migration: QEMU-validated path from prototype to hardware.

## Features

- Reliable bare-metal startup (assembly `_start` with stack/BSS init)
- Human-readable UART update protocol (115200 8N1)
- CRC32 + magic number + size firmware validation
- Compact footprint (default ~64 KB for bootloader)
- Clean handoff to application
- Portable HAL for easy porting

## Need a Custom Port?

- UART/USB update for your specific board
- GPIO/timeout recovery trigger
- LED signaling (e.g., Waveshare ESP32-C3 Zero RGB)
- Fast delivery: 1–3 days with full source and total ownership

Let's build simple, dependable boot infrastructure — fast and 100% yours!

## Project Structure
```text
exerionbit-riscv-bootloader/
├── src/           # Bootloader source (entry, drivers, update protocol)
├── include/       # Definitions & HAL interfaces
├── linker/        # Linker scripts (memory.ld, test_app.ld)
├── boards/        # Board-specific HAL
│   └── qemu_virt/ # Default: QEMU RISC-V Virt
├── scripts/       # Validation/automation scripts
├── docs/
│   ├── evidence/  # Validation evidence by version (docs/evidence/<version>/)
│   └── KNOWN_LIMITS.md
├── Makefile       # Build system
├── BOOT_SEQUENCE.md
├── VALIDATION_PROFILE.md
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

## Porting to Real Hardware

- Create `boards/<your_board>/`
- Implement `platform.c` with HAL functions from `include/boot.h` (`uart_init()`, `uart_putc()`, etc.)
- Optional: GPIO/LED init for signaling
- Adjust `linker/memory.ld` for real flash/RAM map
- Update `Makefile` for new target
- Build & flash with `esptool.py` / OpenOCD

## Status

- ✅ QEMU virt reference fully working
- ✅ Windows native support (tested)
- ✅ Linux / WSL / macOS support

📖 For detailed setup instructions and troubleshooting, see [SETUP.md](SETUP.md)
