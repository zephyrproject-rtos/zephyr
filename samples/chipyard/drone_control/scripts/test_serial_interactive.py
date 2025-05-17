import serial
import threading
import sys

# ==== CONFIGURE THIS ====
PORT = "/dev/ttyUSB1"  # Update for your board
BAUDRATE = 115200

# ==== Connect ====
ser = serial.Serial(PORT, BAUDRATE, timeout=0.1)

def read_from_serial():
    while True:
        try:
            line = ser.readline()
            if line:
                decoded = line.decode(errors="ignore").strip()
                print(f"\r[SoC] {decoded}\n> ", end="")
        except serial.SerialException:
            break

# ==== Start background RX thread ====
rx_thread = threading.Thread(target=read_from_serial, daemon=True)
rx_thread.start()

print("=== TinyMPC Serial Console ===")
print("Type and press Enter to send. Press Ctrl+C to quit.\n")

try:
    while True:
        user_input = input("> ")
        if user_input:
            ser.write((user_input + "\n").encode("ascii"))
except KeyboardInterrupt:
    print("\nExiting serial console.")
finally:
    ser.close()

