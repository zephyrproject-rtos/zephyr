import os
import time
import argparse
import numpy as np
import threading
import pybullet as p
import serial
from datetime import datetime

from gym_pybullet_drones.utils.enums import DroneModel, Physics
from gym_pybullet_drones.envs.CtrlAviary import CtrlAviary
from gym_pybullet_drones.control.DSLPIDControl import DSLPIDControl
from gym_pybullet_drones.utils.Logger import Logger
from gym_pybullet_drones.utils.utils import sync, str2bool

UART_PORT = "/dev/ttyUSB1"
DEFAULT_DRONES = DroneModel("cf2x")
DEFAULT_NUM_DRONES = 3
DEFAULT_PHYSICS = Physics("pyb")
DEFAULT_GUI = True
DEFAULT_RECORD_VISION = False
DEFAULT_PLOT = True
DEFAULT_USER_DEBUG_GUI = False
DEFAULT_OBSTACLES = True
DEFAULT_SIMULATION_FREQ_HZ = 400
DEFAULT_CONTROL_FREQ_HZ = 400
DEFAULT_DURATION_SEC = 1000
DEFAULT_OUTPUT_FOLDER = 'results'
DEFAULT_COLAB = False

TIME_SCALE = 0.2




def quaternion_to_RodriguesParam(q, eps=1e-8):
    q1, q2, q3, q4 = q
    if abs(q4) < eps:
        # Fallback: return zero or normalize some other way
        return 0.0, 0.0, 0.0
    return q1/q4, q2/q4, q3/q4

def extract_state_inputs(observation):
    x, y, z, vx, vy, vz = observation[0], observation[1], observation[2], observation[10], observation[11], observation[12]
    r1, r2, r3 = quaternion_to_RodriguesParam(observation[3:7])
    dphi, dtheta, dpsi = observation[13], observation[14], observation[15]
    return f"{x:.5f} {y:.5f} {z:.5f} {r1:.5f} {r2:.5f} {r3:.5f} {vx:.5f} {vy:.5f} {vz:.5f} {dphi:.5f} {dtheta:.5} {dpsi:.5f}"
    #return f"{x:.6f} {y:.6f} {z:.6f} {r1:.6f} {r2:.6f} {r3:.6f} {vx:.6f} {vy:.6f} {vz:.6f} {dphi:.6f} {dtheta:.6f} {dpsi:.6f}"

def calculate_rpm(env, normalized_thrusts):
    max_thrust_N = 0.58 / 4
    actual_thrusts = (normalized_thrusts + 0.583) * max_thrust_N
    actual_thrusts_clipped = np.clip(actual_thrusts, 0, None)
    return np.sqrt(actual_thrusts_clipped / env.KF)


class TinyMPCSerialInterface:
    def __init__(self, port, baudrate=115200, timeout=0.5, wait_for_banner=True, debug=True):
        self.debug = debug
        self.ser = serial.Serial(port, baudrate=baudrate, timeout=timeout)
        time.sleep(2)
        if wait_for_banner and self.debug:
            print("[TinyMPC] Waiting for boot banner...")
            while True:
                line = self.ser.readline().decode(errors="ignore").strip()
                if line:
                    print(f"[TinyMPC][BOOT] {line}")
                    break

    def send_state(self, drone_id, observation):
        state_str = f"{drone_id} " + extract_state_inputs(observation)
        if self.debug:
            print(f"[TinyMPC][TX] {state_str}")
        self.ser.write((state_str + "\n").encode('ascii'))

    def read_action(self):
        line = self.ser.readline().decode(errors='ignore').strip()
        if not line:
            return None
        if self.debug:
            print(f"[TinyMPC][RX] {line}")
        if line.startswith("u = ["):
            parts = line.split('[')[1].split(']')[0].strip().split()
            drone_id = int(parts[0])
            values = np.array([float(v) for v in parts[1:]])
            return drone_id, values
        return None

    def close(self):
        self.ser.close()

def create_target_marker():
    shape = p.createVisualShape(p.GEOM_SPHERE, radius=0.03, rgbaColor=[1, 0, 0, 0.5])
    return p.createMultiBody(0, shape, -1, [0, 0, 0.5])


def run(**kwargs):
    num_drones = kwargs['num_drones']
    assert kwargs['gui'], "GUI must be enabled to see visual markers"

    env = CtrlAviary(
        drone_model=kwargs['drone'],
        num_drones=num_drones,
        initial_xyzs=np.array([[0.5*i, 0, 0.1] for i in range(num_drones)]),
        initial_rpys=np.array([[0, 0, 0] for _ in range(num_drones)]),
        physics=kwargs['physics'],
        neighbourhood_radius=10,
        pyb_freq=kwargs['simulation_freq_hz'],
        ctrl_freq=kwargs['control_freq_hz'],
        gui=kwargs['gui'],
        record=kwargs['record_video'],
        obstacles=kwargs['obstacles'],
        user_debug_gui=kwargs['user_debug_gui']
    )

    
    
    offsets = np.array([[0.5 * i, 0, 0] for i in range(kwargs['num_drones'])])

    p_client = env.getPyBulletClient()
    target_x_slider = p.addUserDebugParameter("Target X", -2, 2, 0)
    target_y_slider = p.addUserDebugParameter("Target Y", -2, 2, 0)
    target_z_slider = p.addUserDebugParameter("Target Z", 0, 2, 1)

    
    



    interface = TinyMPCSerialInterface(port=UART_PORT)
    action = np.zeros((num_drones, 4))
    obs = np.zeros((num_drones, 20))

    obs_lock = threading.Lock()

    def sim_loop():

        text_id = None
        
        next_step = time.time() + env.CTRL_TIMESTEP / TIME_SCALE
        
        while True:
            target_x = p.readUserDebugParameter(target_x_slider)
            target_y = p.readUserDebugParameter(target_y_slider)
            target_z = p.readUserDebugParameter(target_z_slider)
            
            target = [target_x, target_y, target_z]



            obs_local, _, _, _, _ = env.step(action)
            with obs_lock:
                for i in range(num_drones):
                    obs_local[i][0] -= (target_x + offsets[i][0])
                    obs_local[i][1] -= (target_y + offsets[i][1])
                    obs_local[i][2] -= (target_z - 0.1 + offsets[i][2])
                obs[:] = obs_local

            env.render()
            time.sleep(max(0, next_step - time.time()))
            next_step += env.CTRL_TIMESTEP / TIME_SCALE

    def control_loop():
        pending = [True] * num_drones
        
        while True:
            for i in range(num_drones):
                buffer = False
                with obs_lock:
                    if pending[i]:
                        interface.send_state(i, obs[i])
                        pending[i] = False
                        buffer = True
                if buffer:
                    time.sleep(0.015)
                        

            # Check if any action is available without blocking
            result = interface.read_action()
            if result is not None:
                recv_id, forces = result
                if 0 <= recv_id < num_drones:
                    rpm = calculate_rpm(env, forces)
                    action[recv_id] = rpm
                    pending[recv_id] = True
            else:
                # Prevent busy looping
                time.sleep(0.001)

    def marker_loop(num_drones, offsets):
        # Create a shared visual shape for all markers
        target_visual = p.createVisualShape(
            shapeType=p.GEOM_SPHERE, 
            radius=0.02, 
            rgbaColor=[1, 0, 0, 0.5]
        )

        # Create one marker per drone, offset accordingly
        target_marker_ids = [
            p.createMultiBody(
                baseMass=0,
                baseVisualShapeIndex=target_visual,
                baseCollisionShapeIndex=-1,
                basePosition=[0.5 * i, 0, 1]  # default initial position
            )
            for i in range(num_drones)
        ]

        last_sliders = None

        while True:
            # Read sliders
            target_x = p.readUserDebugParameter(target_x_slider)
            target_y = p.readUserDebugParameter(target_y_slider)
            target_z = p.readUserDebugParameter(target_z_slider)

            current_sliders = (target_x, target_y, target_z)
            if current_sliders != last_sliders:
                last_sliders = current_sliders

                for i in range(num_drones):
                    offset = offsets[i]
                    marker_pos = [
                        target_x + offset[0],
                        target_y + offset[1],
                        target_z + offset[2]
                    ]
                    p.resetBasePositionAndOrientation(
                        target_marker_ids[i],
                        marker_pos,
                        [0, 0, 0, 1]
                    )

            time.sleep(0.05)


    def marker_loop_broke():
        prev_target = [None, None, None]
        line_ids = None
        while True:
            target_x = p.readUserDebugParameter(target_x_slider)
            target_y = p.readUserDebugParameter(target_y_slider)
            target_z = p.readUserDebugParameter(target_z_slider)
            
            target = [target_x, target_y, target_z]

            if target != prev_target:
                prev_target = target

                p.resetBasePositionAndOrientation(target_marker_id, target, [0, 0, 0, 1])

                if line_ids is not None:
                    for lid in line_ids:
                        p.removeUserDebugItem(lid)
                line_ids = [
                    p.addUserDebugLine(target, [target_x + 0.05, target_y, target_z], [1, 0, 0], lineWidth=3.0),
                    p.addUserDebugLine(target, [target_x, target_y + 0.05, target_z], [0, 1, 0], lineWidth=3.0),
                    p.addUserDebugLine(target, [target_x, target_y, target_z + 0.05], [0, 0, 1], lineWidth=3.0),
                ]

    sim_thread = threading.Thread(target=sim_loop, daemon=True)
    ctrl_thread = threading.Thread(target=control_loop, daemon=True)
    marker_thread = threading.Thread(target=marker_loop, args=(num_drones, offsets), daemon=True)

    sim_thread.start()
    ctrl_thread.start()
    marker_thread.start()
    sim_thread.join()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--drone', default=DEFAULT_DRONES, type=DroneModel, choices=DroneModel)
    parser.add_argument('--num_drones', default=DEFAULT_NUM_DRONES, type=int)
    parser.add_argument('--physics', default=DEFAULT_PHYSICS, type=Physics, choices=Physics)
    parser.add_argument('--gui', default=DEFAULT_GUI, type=str2bool)
    parser.add_argument('--record_video', default=DEFAULT_RECORD_VISION, type=str2bool)
    parser.add_argument('--plot', default=DEFAULT_PLOT, type=str2bool)
    parser.add_argument('--user_debug_gui', default=DEFAULT_USER_DEBUG_GUI, type=str2bool)
    parser.add_argument('--obstacles', default=DEFAULT_OBSTACLES, type=str2bool)
    parser.add_argument('--simulation_freq_hz', default=DEFAULT_SIMULATION_FREQ_HZ, type=int)
    parser.add_argument('--control_freq_hz', default=DEFAULT_CONTROL_FREQ_HZ, type=int)
    parser.add_argument('--duration_sec', default=DEFAULT_DURATION_SEC, type=int)
    parser.add_argument('--output_folder', default=DEFAULT_OUTPUT_FOLDER, type=str)
    parser.add_argument('--colab', default=DEFAULT_COLAB, type=bool)
    args = parser.parse_args()
    run(**vars(args))
