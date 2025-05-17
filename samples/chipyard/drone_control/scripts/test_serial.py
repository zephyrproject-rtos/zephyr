import serial
import time

# ==== CONFIGURE THESE ====
PORT = "/dev/ttyUSB1"  # Update for your setup
BAUDRATE = 115200
TIMEOUT = 1  # seconds

# ==== Connect ====
ser = serial.Serial(PORT, BAUDRATE, timeout=TIMEOUT)
time.sleep(2)  # Let board reset if needed

print("Waiting for boot banner from device...")

# ==== Wait for first complete line ====
while True:
    line = ser.readline()
    if line:
        decoded = line.decode(errors="ignore").strip()
        print(f"[BOOT] {decoded}")
        if decoded:  # first non-empty line
            break

# ==== Construct dummy state vector ====
state_floats = [0.1 * (i + 1) for i in range(12)]
state_str = " ".join([f"{v:.4f}" for v in state_floats]) + "\n"

print(f"Sending: {state_str.strip()}")
ser.write(state_str.encode("ascii"))

# ==== Receive response ====
print("Waiting for MPC output...")
start_time = time.time()
while time.time() - start_time < 2:
    line = ser.readline()
    if line:
        decoded = line.decode(errors="ignore").strip()
        print(f"[MPC] {decoded}")
        if decoded.startswith("u = ["):
            break

ser.close()

