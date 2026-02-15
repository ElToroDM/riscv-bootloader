#===============================================================================
# RISC-V UART Bootloader - Makefile
# Builds bootloader for QEMU virt by default
#===============================================================================

# Toolchain
# Try riscv-none-elf- first (xPack), fallback to riscv64-unknown-elf-
CROSS_COMPILE ?= riscv-none-elf-
CC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

# Directories
SRC_DIR = src
INC_DIR = include
LNK_DIR = linker
BRD_DIR = boards/qemu_virt
OBJ_DIR = obj

# Target
TARGET = bootloader.elf
BINARY = bootloader.bin

# Compilation Flags
# RV32IM, no standard library, freestanding
CFLAGS = -march=rv32im_zicsr -mabi=ilp32 -ffreestanding -nostdlib -O2 -Wall -Wextra -I$(INC_DIR) -I$(BRD_DIR)
LDFLAGS = -T $(LNK_DIR)/memory.ld -nostdlib -nostartfiles

# Source Files
SRCS = $(SRC_DIR)/start.S \
       $(SRC_DIR)/main.c \
       $(SRC_DIR)/uart.c \
       $(SRC_DIR)/flash.c \
       $(SRC_DIR)/crc32.c \
       $(BRD_DIR)/platform.c

# Object Files
OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(filter %.c, $(SRCS)))
OBJS += $(patsubst %.S, $(OBJ_DIR)/%.o, $(filter %.S, $(SRCS)))

# Rules
.PHONY: all clean qemu

all: $(BINARY)

$(BINARY): $(TARGET)
	$(OBJCOPY) -O binary $< $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build complete: $@"

$(OBJ_DIR)/%.o: %.c
ifeq ($(OS),Windows_NT)
	@powershell -Command "New-Item -ItemType Directory -Force -Path '$(subst /,\,$(dir $@))' | Out-Null"
else
	@mkdir -p $(dir $@)
endif
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.S
ifeq ($(OS),Windows_NT)
	@powershell -Command "New-Item -ItemType Directory -Force -Path '$(subst /,\,$(dir $@))' | Out-Null"
else
	@mkdir -p $(dir $@)
endif
	$(CC) $(CFLAGS) -c $< -o $@

clean:
ifeq ($(OS),Windows_NT)
	@if exist $(OBJ_DIR) rmdir /S /Q $(OBJ_DIR)
	@if exist $(TARGET) del /Q $(TARGET)
	@if exist $(BINARY) del /Q $(BINARY)
	@if exist test_app.elf del /Q test_app.elf
	@if exist test_app.bin del /Q test_app.bin
else
	rm -rf $(OBJ_DIR) $(TARGET) $(BINARY) test_app.elf test_app.bin
endif

# Test application targets
.PHONY: test-app test-app-clean

TEST_APP_ELF = test_app.elf
TEST_APP_BIN = test_app.bin
TEST_APP_SRCS = $(SRC_DIR)/test_app_start.S $(SRC_DIR)/test_app.c $(SRC_DIR)/uart.c $(BRD_DIR)/platform.c
TEST_APP_OBJS = $(patsubst %.c, $(OBJ_DIR)/%.test.o, $(filter %.c, $(TEST_APP_SRCS)))
TEST_APP_OBJS += $(patsubst %.S, $(OBJ_DIR)/%.test.o, $(filter %.S, $(TEST_APP_SRCS)))
TEST_APP_LDFLAGS = -T $(LNK_DIR)/test_app.ld -nostdlib -nostartfiles

test-app: $(TEST_APP_BIN)

$(TEST_APP_BIN): $(TEST_APP_ELF)
	$(OBJCOPY) -O binary $< $@
	@echo "Test app binary: $@"

$(TEST_APP_ELF): $(TEST_APP_OBJS)
	$(CC) $(CFLAGS) $(TEST_APP_OBJS) -o $@ $(TEST_APP_LDFLAGS)
	@echo "Test app ELF: $@"

$(OBJ_DIR)/%.test.o: %.c
ifeq ($(OS),Windows_NT)
	@powershell -Command "New-Item -ItemType Directory -Force -Path '$(subst /,\,$(dir $@))' | Out-Null"
else
	@mkdir -p $(dir $@)
endif
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.test.o: %.S
ifeq ($(OS),Windows_NT)
	@powershell -Command "New-Item -ItemType Directory -Force -Path '$(subst /,\,$(dir $@))' | Out-Null"
else
	@mkdir -p $(dir $@)
endif
	$(CC) $(CFLAGS) -c $< -o $@

# Helper to run in QEMU (bare-metal virt machine)
qemu: $(TARGET)
ifeq ($(OS),Windows_NT)
	"C:\Program Files\qemu\qemu-system-riscv32.exe" -M virt -display none -serial stdio -bios none -kernel $(TARGET)
else
	qemu-system-riscv32 -M virt -display none -serial stdio -bios none -kernel $(TARGET)
endif

# Run in QEMU with TCP serial for testing
.PHONY: qemu-tcp
qemu-tcp: $(TARGET)
ifeq ($(OS),Windows_NT)
	"C:\Program Files\qemu\qemu-system-riscv32.exe" -M virt -display none -serial tcp:localhost:10000,server,nowait -bios none -kernel $(TARGET)
else
	qemu-system-riscv32 -M virt -display none -serial tcp:localhost:10000,server,nowait -bios none -kernel $(TARGET)
endif
