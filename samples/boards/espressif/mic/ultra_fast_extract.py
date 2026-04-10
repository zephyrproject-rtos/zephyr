#!/usr/bin/env python3
# Copyright (c) 2026 NotioNext Ltd.
# SPDX-License-Identifier: Apache-2.0

"""
Ultra-fast audio extraction from ESP32-S3-BOX-3 (Zephyr shell bincat command).

Usage:
    python ultra_fast_extract.py              # auto-detect port, extract all audio_*.raw
    python ultra_fast_extract.py /dev/ttyACM0 # specify port
"""

import serial
import serial.tools.list_ports
import time
import os
import wave
import sys
import re

# ── Serial settings ────────────────────────────────────────────────────────────
BAUD = 115200
TIMEOUT = 30          # seconds waiting for BINEND

# ── Audio format (must match Zephyr main.c) ────────────────────────────────────
SAMPLE_RATE   = 16000
CHANNELS      = 2
SAMPLE_WIDTH  = 2     # bytes (int16)

# ── Output directory ───────────────────────────────────────────────────────────
OUT_DIR = "extracted_audio"


def find_serial_port():
    preferred = ["/dev/ttyACM0", "/dev/ttyUSB0"]
    for p in preferred:
        if os.path.exists(p):
            return p
    for port in serial.tools.list_ports.comports():
        if "USB" in port.description or "ACM" in port.device:
            return port.device
    return None


def send_cmd(ser, cmd, wait=0.5):
    """Send a command and wait briefly for output to arrive."""
    ser.reset_input_buffer()
    ser.write((cmd + "\n").encode())
    time.sleep(wait)


def list_audio_files(ser):
    """Return list of (filename, size) tuples for audio_*.raw files in /lfs."""
    send_cmd(ser, "fs ls /lfs", wait=1.0)
    raw = ser.read_all().decode("utf-8", errors="ignore")
    print("[FS listing]\n" + raw)

    files = []
    for line in raw.splitlines():
        # Zephyr shell line looks like:   "     1024 audio_000.raw"
        # (may have ANSI escapes or shell prompt fragments — strip them)
        clean = re.sub(r'\x1b\[[0-9;]*m', '', line).strip()
        m = re.match(r'(\d+)\s+(audio_\S+\.raw)', clean)
        if m:
            size = int(m.group(1))
            name = m.group(2)
            files.append((name, size))
    return files


def extract_file(ser, remote_name, local_raw_path):
    """
    Run 'bincat /lfs/<remote_name>' and capture the hex payload.
    Returns number of bytes written (0 on failure).
    """
    print(f"\n🚀 Extracting /lfs/{remote_name} ...")
    ser.reset_input_buffer()

    cmd = f"bincat /lfs/{remote_name}\n"
    ser.write(cmd.encode())

    in_binary = False
    bytes_written = 0
    start = time.time()
    deadline = start + TIMEOUT

    with open(local_raw_path, "wb") as f:
        while time.time() < deadline:
            raw_line = ser.readline()
            if not raw_line:
                continue

            # Decode and strip ANSI + shell prompt artifacts
            line = raw_line.decode("utf-8", errors="ignore")
            line = re.sub(r'\x1b\[[0-9;]*m', '', line)  # ANSI colours
            line = re.sub(r'uart:~\$\s*', '', line)      # shell prompt
            line = line.strip()

            if not line:
                continue

            if line == "BINSTART":
                in_binary = True
                print("📡 Binary transfer started...")
                deadline = time.time() + TIMEOUT  # reset timeout
                continue

            if line == "BINEND":
                print("✅ Transfer complete!")
                break

            if in_binary:
                # Must be a pure hex string (even number of hex chars)
                hex_clean = re.sub(r'[^0-9a-fA-F]', '', line)
                if len(hex_clean) % 2 != 0 or len(hex_clean) == 0:
                    continue
                try:
                    data = bytes.fromhex(hex_clean)
                    f.write(data)
                    bytes_written += len(data)
                    if bytes_written % (10 * 1024) < len(data):
                        elapsed = time.time() - start
                        speed = bytes_written / elapsed / 1024 if elapsed else 0
                        print(f"  📊 {bytes_written} bytes  ({speed:.1f} KB/s)")
                except ValueError:
                    continue

    elapsed = time.time() - start
    speed = bytes_written / elapsed / 1024 if elapsed else 0
    print(f"  🎯 {bytes_written} bytes in {elapsed:.1f}s  ({speed:.1f} KB/s)")
    return bytes_written


def raw_to_wav(raw_path, wav_path):
    """Wrap raw PCM data in a WAV header."""
    if not os.path.exists(raw_path) or os.path.getsize(raw_path) == 0:
        print(f"❌ {raw_path} is empty or missing")
        return False
    with open(raw_path, "rb") as rf:
        data = rf.read()
    with wave.open(wav_path, "wb") as wf:
        wf.setnchannels(CHANNELS)
        wf.setsampwidth(SAMPLE_WIDTH)
        wf.setframerate(SAMPLE_RATE)
        wf.writeframes(data)
    duration = len(data) / (SAMPLE_RATE * CHANNELS * SAMPLE_WIDTH)
    print(f"✅ WAV: {wav_path}  ({len(data)} bytes, {duration:.1f}s)")
    return True


def main():
    print("🎙️  ESP32-S3-BOX-3 Audio Extractor")
    print(f"   Sample rate: {SAMPLE_RATE} Hz | Channels: {CHANNELS} | Bit depth: {SAMPLE_WIDTH*8}")

    port = sys.argv[1] if len(sys.argv) > 1 else find_serial_port()
    if not port:
        print("❌ No ESP32 serial port found. Pass port as argument.")
        sys.exit(1)

    print(f"\n📡 Connecting to {port} at {BAUD} baud ...")
    try:
        ser = serial.Serial(port, BAUD, timeout=2)
    except serial.SerialException as e:
        print(f"❌ Cannot open {port}: {e}")
        sys.exit(1)

    time.sleep(1)
    ser.reset_input_buffer()

    # List files on device
    files = list_audio_files(ser)
    if not files:
        print("\n❌ No audio_*.raw files found in /lfs.")
        print("   Make sure the firmware has completed at least one recording cycle.")
        ser.close()
        sys.exit(1)

    print(f"\n📂 Found {len(files)} file(s):")
    for name, size in files:
        print(f"   {name}  ({size} bytes)")

    os.makedirs(OUT_DIR, exist_ok=True)

    ok = 0
    for name, _ in files:
        raw_path = os.path.join(OUT_DIR, name)
        wav_path = raw_path.replace(".raw", ".wav")

        n = extract_file(ser, name, raw_path)
        if n > 0 and raw_to_wav(raw_path, wav_path):
            ok += 1
        else:
            print(f"❌ Failed: {name}")

    ser.close()

    print(f"\n{'='*50}")
    print(f"✅ {ok}/{len(files)} files extracted to ./{OUT_DIR}/")
    if ok:
        print("\n🎵 Play with:")
        for name, _ in files:
            wav = name.replace(".raw", ".wav")
            print(f"   aplay -r {SAMPLE_RATE} -c {CHANNELS} -f S16_LE {OUT_DIR}/{wav}")
            print(f"   # or: play {OUT_DIR}/{wav}")


if __name__ == "__main__":
    main()
