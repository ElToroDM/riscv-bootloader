# Setup Instructions

## 🪟 Windows Setup (Recommended - Native, No WSL)

> **⚠️ Important:** This is a **generic QEMU RISC-V project**, not ESP32-specific. Do NOT use the ESP-IDF extension - it's for real ESP32 hardware only.

### **Automated Installation (Recommended - 5 minutes)**

Open **PowerShell as Administrator** and run these commands:

**Step 1: Install Chocolatey** (skip if already installed)
```powershell
Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
```

**Step 2: Install Make and QEMU**
```powershell
choco install make -y
choco install qemu -y
```

**Step 3: Download and Install RISC-V Toolchain**
```powershell
# Download xPack RISC-V GCC
$url = "https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v13.3.0-2/xpack-riscv-none-elf-gcc-13.3.0-2-win32-x64.zip"
$output = "$env:TEMP\riscv-toolchain.zip"
Invoke-WebRequest -Uri $url -OutFile $output -UseBasicParsing

# Extract to C:\riscv-toolchain
$destination = "C:\riscv-toolchain"
Expand-Archive -Path $output -DestinationPath $destination -Force

# Add to PATH (permanent)
$toolchainPath = "C:\riscv-toolchain\xpack-riscv-none-elf-gcc-13.3.0-2\bin"
[Environment]::SetEnvironmentVariable("Path", [Environment]::GetEnvironmentVariable("Path", "User") + ";$toolchainPath", "User")

# Add QEMU to PATH (permanent)
[Environment]::SetEnvironmentVariable("Path", [Environment]::GetEnvironmentVariable("Path", "User") + ";C:\Program Files\qemu", "User")

# Update current session
$env:PATH += ";$toolchainPath;C:\Program Files\qemu"
```

**Step 4: Verify Installation**
```powershell
make --version
qemu-system-riscv32 --version
riscv-none-elf-gcc --version
```

✅ You should see version information for all three tools.

---

### **Manual Installation** (Alternative)

1. **RISC-V Toolchain**: Download [xPack RISC-V GCC](https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/latest), extract to `C:\riscv-toolchain\`, add to PATH
2. **Make**: Install via [Chocolatey](https://chocolatey.org/) (`choco install make`) or [GnuWin32](http://gnuwin32.sourceforge.net/packages/make.htm)
3. **QEMU**: Install via [Chocolatey](https://chocolatey.org/) (`choco install qemu`) or [installer](https://qemu.weidai.com.cn/download/)

### **Getting Started After Installation**

1. **Close and reopen PowerShell** after installation to load new PATH variables

2. Navigate to project and verify tools:
   ```powershell
   cd C:\Code\riscv\riscv-bootloader
   make --version              # Should show GNU Make 4.4.1
   qemu-system-riscv32 --version  # Should show QEMU 10.x
   riscv-none-elf-gcc --version   # Should show xPack GCC 15.x
   ```

3. Build and run:
   ```powershell
   make all        # Compile bootloader
   make qemu       # Run in QEMU (Ctrl+C to exit)
   ```

4. **Or use VS Code**: Open project → `Ctrl+Shift+P` → **Tasks: Run Task**
   Available tasks:
   - **"Build Bootloader"** → Compiles the bootloader
   - **"Clean Build"** → Removes build artifacts
   - **"Run in QEMU"** → Launches QEMU with UART output in terminal
   - **"Build & Run QEMU"** → One-step build + test (recommended)

**Expected UART Output:**
```text
BOOT?
[Bootloader waiting for update command 'u'...]
```

Press `Ctrl+C` in the terminal to stop QEMU.

### Visual Live Test Runner (Recommended for demos)

Use the PowerShell wrapper to run protocol validation while tailing UART output in a dedicated window:

```powershell
.\scripts\run-test-with-uart-tail.ps1
```

Useful options:

```powershell
.\scripts\run-test-with-uart-tail.ps1 -DemoMode    # prints recording-oriented layout hints
.\scripts\run-test-with-uart-tail.ps1 -NoTail      # run only validator output in current terminal
.\scripts\run-test-with-uart-tail.ps1 -LiveOnly    # remove UART mirror file at exit
```

Recommended terminal layout for recordings (side by side):

1. Run `.\scripts\run-test-with-uart-tail.ps1 -DemoMode`
2. Use Windows Snap: press `Win + Left` on the main terminal and `Win + Right` on the UART tail window
3. Keep both windows at similar size (around 120x36) and record only the central area

In `-DemoMode`, protocol steps are intentionally paced so you can narrate what is happening (`BOOT?`, enter update mode, erase, upload, CRC, reboot, app boot).

**Alternative (Command Line):**
```powershell
make all        # Build bootloader
make qemu       # Run in QEMU
make clean      # Clean build files
```

**Toolchain Notes:**
- Uses **xPack RISC-V GCC** with prefix `riscv-none-elf-`
- Compiles RV32IM code via `-march=rv32im_zicsr -mabi=ilp32` flags
- This is a **generic QEMU project**, not tied to any specific hardware
- Alternative toolchains work if `CROSS_COMPILE` is adjusted in Makefile

**Status:** ✅ Tested on Windows 11 native (no WSL required)

---

## Setup for Linux / WSL / macOS

**Prerequisites:** 
- `riscv64-unknown-elf-gcc` toolchain
  - **Ubuntu/Debian:** `sudo apt-get install gcc-riscv64-unknown-elf`
  - **macOS (Homebrew):** `brew tap riscv/riscv && brew install riscv-tools`
  - **From source:** [riscv-gnu-toolchain](https://github.com/riscv-collab/riscv-gnu-toolchain)
- QEMU: `sudo apt-get install qemu-system-riscv32` (Linux) or `brew install qemu` (macOS)

**Build and Run:**

```bash
git clone https://github.com/ElToroDM/riscv-bootloader.git
cd riscv-bootloader

make                # Build ELF
make qemu           # Run in QEMU virt (see UART output)
```

**Expected UART Output:**
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

---

## Troubleshooting

**Windows: Toolchain not found**
- **Close and reopen PowerShell/VS Code** after installation
- Verify PATH manually: `$env:PATH -split ';' | Select-String riscv`
- Test: `riscv-none-elf-gcc --version`
- If still fails, add manually:
  ```powershell
  $env:PATH += ";C:\riscv-toolchain\xpack-riscv-none-elf-gcc-13.3.0-2\bin"
  ```

**Windows: QEMU not starting**
- QEMU installs to `C:\Program Files\qemu\` but doesn't always auto-add to PATH
- **Solution**: Add manually (permanent):
  ```powershell
  [Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\Program Files\qemu", "User")
  $env:PATH += ";C:\Program Files\qemu"  # Current session
  ```
- Verify: `qemu-system-riscv32 --version`
- Makefile uses absolute path as fallback

**Windows: Make errors with "The system cannot find the path specified"**
- Ensure all PATH variables are loaded: Close and reopen terminal
- Run `refreshenv` if using Chocolatey PowerShell

**Build errors: "command not found"**
- Linux/WSL: Install build essentials: `sudo apt-get install build-essential`
- Verify toolchain: `riscv64-unknown-elf-gcc --version`

**QEMU shows nothing**
- This is normal — bootloader waits silently for UART input
- Expected first output: `BOOT?` prompt
- Press any key (except 'u') to test, or wait for timeout

**Contact**
- Open an issue to start a conversation or request a fast estimate.
- Let's build simple, dependable boot infrastructure — fast and 100% yours!
