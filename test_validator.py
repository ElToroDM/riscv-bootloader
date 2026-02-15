#!/usr/bin/env python3
"""RISC-V Bootloader UART Protocol Test & Validation"""
import argparse
import os
import queue
import shutil
import subprocess
import sys
import threading
import time
from binascii import crc32

# Constants
FIRMWARE_SIZE = 1024
PROGRESS_BAR_DELAY = 0.2  # Show progress bar only after 0.2 secs
PROGRESS_BAR_MIN_DURATION = 0.5  # Only if it will take more than 2 secs

# ANSI colors
class C:
    GREEN, RED, CYAN, BOLD, END = '\033[92m', '\033[91m', '\033[96m', '\033[1m', '\033[0m'


def step(num, total, msg):
    print(f"{C.BOLD}[{num}/{total}]{C.END} {C.CYAN}{msg}{C.END}")


def ok(msg):
    print(f"  {C.GREEN}✓{C.END}  {msg}")


def fail(msg):
    print(f"  {C.RED}✗{C.END}  {msg}")


# Progress tracking globals
_progress_start_time = None
_progress_shown = False

# UART mirror globals
_uart_mirror_file = None
_uart_mirror_live_only = False
_uart_mirror_handle = None
_uart_last_was_newline = False

# Demo pacing globals
_demo_step_delay = 0.0
_demo_byte_delay = 0.0003


def progress(curr, total, byte_delay=0.0003):
    global _progress_start_time, _progress_shown

    # Initialize on first call
    if curr == 0 or _progress_start_time is None:
        _progress_start_time = time.time()
        _progress_shown = False

    estimated_total_time = total * byte_delay
    elapsed = time.time() - _progress_start_time

    # Only show if estimated time > PROGRESS_BAR_MIN_DURATION and elapsed > PROGRESS_BAR_DELAY
    if estimated_total_time > PROGRESS_BAR_MIN_DURATION and elapsed > PROGRESS_BAR_DELAY:
        _progress_shown = True
        width = 30
        done = int(width * curr / total)
        bar = '█' * done + '░' * (width - done)
        pct = 100 * curr / total
        print(f"\r  [{bar}] {pct:5.1f}%", end='', flush=True)

    if curr >= total:
        if _progress_shown:
            print("\r" + " " * 50 + "\r", end="")
        _progress_start_time = None
        _progress_shown = False


def animated_wait(duration):
    """Show progress bar while waiting"""
    start = time.time()
    show_progress = duration > PROGRESS_BAR_MIN_DURATION
    progress_started = False

    while time.time() - start < duration:
        elapsed = time.time() - start

        # Only show progress if duration > PROGRESS_BAR_MIN_DURATION and elapsed > PROGRESS_BAR_DELAY
        if show_progress and elapsed > PROGRESS_BAR_DELAY:
            progress_started = True
            pct = min(100, int(100 * elapsed / duration))
            bar_width = 30
            filled = int(bar_width * elapsed / duration)
            bar = '█' * filled + '░' * (bar_width - filled)
            print(f"\r  [{bar}] {pct:5.1f}%", end='', flush=True)

        time.sleep(0.05)

    if progress_started:
        print("\r" + " " * 50 + "\r", end="")


def _mirror_uart_byte(byte):
    """Mirror printable UART bytes to a file if enabled"""
    global _uart_mirror_handle, _uart_last_was_newline
    if not _uart_mirror_handle:
        return

    value = byte[0]

    if value == 13:
        return

    if value == 10:
        if _uart_last_was_newline:
            return
        _uart_mirror_handle.write("\n")
        _uart_mirror_handle.flush()
        _uart_last_was_newline = True
        return

    if value == 9 or 32 <= value <= 126:
        _uart_mirror_handle.write(chr(value))
        _uart_mirror_handle.flush()
        _uart_last_was_newline = False


def setup_uart_mirror(path=None, live_only=False):
    """Initialize UART mirror output"""
    global _uart_mirror_file, _uart_mirror_live_only, _uart_mirror_handle, _uart_last_was_newline
    _uart_mirror_file = path
    _uart_mirror_live_only = live_only
    _uart_last_was_newline = False

    if not path:
        return

    with open(path, "w", encoding="utf-8"):
        pass
    _uart_mirror_handle = open(path, "a", encoding="utf-8", buffering=1)


def cleanup_uart_mirror():
    """Close and optionally remove UART mirror output"""
    global _uart_mirror_handle

    if _uart_mirror_handle:
        _uart_mirror_handle.close()
        _uart_mirror_handle = None

    if _uart_mirror_file and _uart_mirror_live_only and os.path.exists(_uart_mirror_file):
        os.remove(_uart_mirror_file)


def find_qemu():
    """Find QEMU executable"""
    exe = "qemu-system-riscv32" + (".exe" if sys.platform.startswith('win') else "")
    qemu = shutil.which(exe)
    if qemu:
        return qemu
    if sys.platform.startswith('win'):
        for path in [r"C:\Program Files\qemu", r"C:\Program Files (x86)\qemu", r"C:\qemu"]:
            full = f"{path}\\{exe}"
            if os.path.exists(full):
                return full
    return None


def kill_all_qemu():
    """Kill any running QEMU instances"""
    try:
        cmd = ['taskkill', '/F', '/IM', 'qemu-system-riscv32.exe'] if sys.platform.startswith('win') else ['pkill', '-9', 'qemu-system-riscv32']
        subprocess.run(cmd, capture_output=True)
    except:
        pass


def bg_reader(proc, q):
    """Background thread to read QEMU stdout"""
    try:
        while True:
            byte = proc.stdout.read(1)
            if not byte:
                break
            _mirror_uart_byte(byte)
            q.put(byte)
    except Exception:
        # Process shutdown can invalidate stdout while this thread is reading.
        # Ignore to keep demo output clean.
        pass
    finally:
        q.put(None)


# Global queue and reader thread
_reader_queue = None
_reader_thread = None


def init_reader(proc):
    """Start background stdout reader"""
    global _reader_queue, _reader_thread
    _reader_queue = queue.Queue()
    _reader_thread = threading.Thread(target=bg_reader, args=(proc, _reader_queue), daemon=True)
    _reader_thread.start()
    time.sleep(0.03)


def wait_for(proc, pattern, timeout=5.0):
    """Wait for pattern in bootloader output"""
    global _reader_queue
    buf, end, pat = b'', time.time() + timeout, pattern.encode()

    while time.time() < end:
        try:
            b = _reader_queue.get(timeout=0.05)
            if b is None:
                return False, buf.decode('ascii', errors='ignore')
            buf += b
            if pat in buf:
                return True, buf.decode('ascii', errors='ignore')
        except queue.Empty:
            continue
    return False, buf.decode('ascii', errors='ignore')


def wait_for_with_progress(proc, pattern, timeout=5.0, forbidden_patterns=None):
    """Wait for pattern while showing progress based on timeout window."""
    global _reader_queue
    buf = b''
    end = time.time() + timeout
    pat = pattern.encode()
    forbidden = [item.encode() for item in (forbidden_patterns or [])]
    progress_started = False

    while time.time() < end:
        remaining = end - time.time()
        elapsed = timeout - remaining

        if timeout > PROGRESS_BAR_MIN_DURATION and elapsed > PROGRESS_BAR_DELAY:
            progress_started = True
            pct = min(100.0, (elapsed / timeout) * 100.0)
            width = 30
            done = int(width * pct / 100.0)
            bar = '█' * done + '░' * (width - done)
            print(f"\r  [{bar}] {pct:5.1f}%", end='', flush=True)

        try:
            b = _reader_queue.get(timeout=0.05)
            if b is None:
                if progress_started:
                    print("\r" + " " * 50 + "\r", end="")
                return False, buf.decode('ascii', errors='ignore')

            buf += b

            for forbidden_pat in forbidden:
                if forbidden_pat in buf:
                    if progress_started:
                        print("\r" + " " * 50 + "\r", end="")
                    return False, buf.decode('ascii', errors='ignore')

            if pat in buf:
                if progress_started:
                    print("\r" + " " * 50 + "\r", end="")
                return True, buf.decode('ascii', errors='ignore')
        except queue.Empty:
            continue

    if progress_started:
        print("\r" + " " * 50 + "\r", end="")
    return False, buf.decode('ascii', errors='ignore')


def send(proc, data):
    """Send data to bootloader"""
    data = data.encode() if isinstance(data, str) else data
    proc.stdin.write(data)
    proc.stdin.flush()
    return True


def maybe_pause():
    """Optional pacing delay for demo narration."""
    if _demo_step_delay > 0:
        time.sleep(_demo_step_delay)


def try_build_test_app():
    """Build test application binary if make is available."""
    make_cmd = shutil.which("make")
    if not make_cmd:
        return False

    try:
        subprocess.run([make_cmd, "test-app"], check=True, capture_output=True, text=True)
        return os.path.exists("test_app.bin")
    except Exception:
        return False


def make_firmware(size=None):
    """Generate test firmware with CRC
    
    If test_app.bin exists, load it; otherwise generate dummy firmware.
    """
    import os as os_module
    
    test_app_path = "test_app.bin"
    if not os_module.path.exists(test_app_path):
        try_build_test_app()

    if os_module.path.exists(test_app_path):
        with open(test_app_path, "rb") as f:
            app = f.read()
        if size and len(app) > size:
            app = app[:size]
    else:
        if size is None:
            size = FIRMWARE_SIZE
        app = (bytes(range(256)) * (size // 256 + 1))[:size]
    
    return app, crc32(app) & 0xFFFFFFFF


def test():
    kill_all_qemu()

    print(f"\n{C.BOLD}RISC-V Bootloader Validation Test{C.END}\n")

    proc = None
    try:
        step(1, 7, "Starting QEMU bootloader")
        qemu_exe = find_qemu()
        if not qemu_exe:
            fail("QEMU not found. Install QEMU or add to PATH.")
            return False

        cmd = [qemu_exe, "-M", "virt", "-display", "none", "-serial", "stdio",
               "-bios", "none", "-kernel", "bootloader.elf"]

        proc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, bufsize=0)
        time.sleep(0.2)
        ok("QEMU running")
        init_reader(proc)

        step(2, 7, "Waiting for bootloader")
        success, resp = wait_for(proc, "BOOT?", timeout=3)
        if not success:
            fail(f"No BOOT? prompt (got: {repr(resp[:50])})")
            return False
        ok("Bootloader ready")

        step(3, 7, "Entering update mode")
        maybe_pause()
        send(proc, 'u')
        time.sleep(0.03)
        success, resp = wait_for(proc, "OK", timeout=3)
        if not success:
            fail("Update mode failed")
            return False
        ok("Update mode active")

        step(4, 7, "Generating test application")
        firmware, fw_crc = make_firmware()
        ok("Test application ready")

        step(5, 7, "Uploading firmware")
        cmd = f"SEND {len(firmware)}\n"
        maybe_pause()
        for c in cmd:
            send(proc, c)
            time.sleep(0.001)
        time.sleep(0.03)
        success, resp = wait_for(proc, "READY", timeout=5)
        if not success:
            fail("Flash not ready")
            return False
        ok("Flash erased, ready for data")

        for i, byte in enumerate(firmware):
            if not send(proc, bytes([byte])):
                fail(f"Send failed at byte {i}")
                return False
            if i % 40 == 0:
                progress(i, len(firmware))
            time.sleep(_demo_byte_delay)
        progress(len(firmware), len(firmware))
        ok(f"Uploaded {len(firmware)} bytes")

        proc.stdin.write(b'\x00' * 32)
        proc.stdin.flush()

        step(6, 7, "Validating CRC")
        success, resp = wait_for(proc, "CRC?", timeout=20)
        if not success:
            fail("CRC check timeout")
            return False

        success, resp = wait_for_with_progress(
            proc,
            "OK",
            timeout=20,
            forbidden_patterns=["APP_BOOT", "App:"],
        )
        if not success:
            if "APP_BOOT" in resp or "App:" in resp:
                fail("Protocol violation: app started before CRC completed")
            else:
                fail("CRC validation failed")
            return False
        ok("CRC validation passed")

        step(7, 7, "Finalizing")
        success, resp = wait_for(proc, "REBOOT", timeout=2)
        if success:
            ok("Reboot initiated")

        ok("Waiting for application output...")
        success, resp = wait_for(proc, "APP_BOOT", timeout=5)
        if not success:
            fail(f"Application output not detected (got: {repr(resp[:120])})")
            return False
        ok("Application boot banner detected")

        success, resp = wait_for(proc, "App:", timeout=5)
        if not success:
            fail(f"Application heartbeat not detected (got: {repr(resp[:120])})")
            return False
        ok("Application heartbeat detected")

        proc.terminate()
        print(f"\n{C.GREEN}{C.BOLD}✓ ALL TESTS PASSED{C.END}\n")
        return True

    except Exception as e:
        fail(f"Error: {e}")
        return False
    finally:
        if proc:
            try:
                proc.terminate()
                proc.wait(timeout=1)
            except:
                proc.kill()
        cleanup_uart_mirror()


def parse_args():
    """Parse command-line arguments"""
    parser = argparse.ArgumentParser(description="RISC-V Bootloader UART Protocol Test & Validation")
    parser.add_argument(
        "--uart-mirror-file",
        help="Mirror printable UART output to a file for live tailing in another terminal",
    )
    parser.add_argument(
        "--uart-live-only",
        action="store_true",
        help="Delete UART mirror file when test exits",
    )
    parser.add_argument(
        "--demo-step-delay",
        type=float,
        default=0.0,
        help="Optional delay in seconds before key protocol steps",
    )
    parser.add_argument(
        "--demo-byte-delay",
        type=float,
        default=0.0003,
        help="Delay in seconds between firmware bytes during upload",
    )
    return parser.parse_args()


if __name__ == '__main__':
    try:
        args = parse_args()
        _demo_step_delay = max(0.0, args.demo_step_delay)
        _demo_byte_delay = max(0.0, args.demo_byte_delay)
        setup_uart_mirror(path=args.uart_mirror_file, live_only=args.uart_live_only)
        success = test()
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        print("\n\nTest interrupted")
        sys.exit(1)
