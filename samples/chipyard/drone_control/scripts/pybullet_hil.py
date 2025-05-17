"""Script demonstrating the joint use of simulation and control.

The simulation is run by a `CtrlAviary` environment.
The control is given by the PID implementation in `DSLPIDControl`.

Example
-------
In a terminal, run as:

    $ python pid.py

Notes
-----
The drones move, at different altitudes, along cicular trajectories 
in the X-Y plane, around point (0, -.3).
# read_action()
"""
import os
import time
import argparse
from datetime import datetime
import pdb
import math
import random
import numpy as np
import pybullet as p
import matplotlib.pyplot as plt
from scipy.spatial.transform import Rotation
import re

from gym_pybullet_drones.utils.enums import DroneModel, Physics
from gym_pybullet_drones.envs.CtrlAviary import CtrlAviary
from gym_pybullet_drones.control.DSLPIDControl import DSLPIDControl
from gym_pybullet_drones.utils.Logger import Logger
from gym_pybullet_drones.utils.utils import sync, str2bool

import time
import subprocess
import serial

UART_PORT = "/dev/ttyUSB1"

NOISE = False

DEFAULT_DRONES = DroneModel("cf2x")
DEFAULT_NUM_DRONES = 1
DEFAULT_PHYSICS = Physics("pyb")
DEFAULT_GUI = True
DEFAULT_RECORD_VISION = False
DEFAULT_PLOT = True
# DEFAULT_PLOT = False
DEFAULT_USER_DEBUG_GUI = False
DEFAULT_OBSTACLES = True
#DEFAULT_SIMULATION_FREQ_HZ = 240
#DEFAULT_CONTROL_FREQ_HZ = 48
DEFAULT_SIMULATION_FREQ_HZ = 400
#DEFAULT_CONTROL_FREQ_HZ = 50
DEFAULT_CONTROL_FREQ_HZ = 100
#DEFAULT_DURATION_SEC = 12
#DEFAULT_DURATION_SEC = 5
DEFAULT_DURATION_SEC = 1000
DEFAULT_OUTPUT_FOLDER = 'results'
DEFAULT_COLAB = False

def quaternion_to_RodriguesParam(q):
    """
    Convert a quaternion into RodriguesParam (r1,r2,r3)
    q: Quaternion represented as [q1, q2, q3, q4]
    """
    # Extract the values from the quaternion
    q1, q2, q3, q4 = q
    # RodriguesParam calculation
    r1 = q1/q4
    r2 = q2/q4
    r3 = q3/q4
    return r1,r2,r3

def extract_state_inputs(observation):
    """
    quaternion, angular rate -> x,y,z Rodriguez parameters
    Extracts the state from the Gym observation.
    observation: ndarray from Gym environment
    """
    # print(f'TYPE OF OBS: {type(observation)}')
    observation = observation[0]
    # Position and linear velocities
    x, y, z, vx, vy, vz = observation[0], observation[1], observation[2], observation[10], observation[11], observation[12]

    # Quaternion to RodriguesParam angles
    quaternion = observation[3:7]
    r1,r2,r3 = quaternion_to_RodriguesParam(quaternion)

    # Angular velocities
    dphi, dtheta, dpsi = observation[13], observation[14], observation[15]

    formatted_state_string = f"{x:.6f} {y:.6f} {z:.6f} {r1:.6f} {r2:.6f} {r3:.6f} {vx:.6f} {vy:.6f} {vz:.6f} {dphi:.6f} {dtheta:.6f} {dpsi:.6f}"

    # print (f"Formatted_State_String =========== {type(formatted_state_string)} {formatted_state_string}")
    return formatted_state_string


def get_action_from_command(output):

    # Split the output into lines and get the last line
    lines = output.strip().split('\n')
    last_line = lines[-1]

    # Extract the numbers from the last line
    numbers_str = last_line.split()[-4:]  # Split the last line and take the last 4 elements
    numbers = [float(num) for num in numbers_str]

    return np.array(numbers)

def calculate_rpm(env, normalized_thrusts):
    """Convert TinyMPC output (normalized thrusts) to RPMs"""
    #print(f"======= Normalized thrusts {normalized_thrusts}")

    max_thrust_N = 0.58 / 4  # Total 0.58 N across 4 motors
    actual_thrusts = (normalized_thrusts + 0.583) * max_thrust_N

    actual_thrusts_clipped = np.clip(actual_thrusts, 0, None)
    #print(f"======= Calculated force {actual_thrusts_clipped}")
    #print(f"======== KF {env.KF}, MAX_THRUST_ENV {env.MAX_THRUST}")

    rpm = np.sqrt(actual_thrusts_clipped / env.KF)
    return rpm

def parse_string_to_numbers(input_data):
    """
    Parse an input (string or numpy array) to extract four numbers and return them in a NumPy array.
    The input string is expected to contain four numbers separated by spaces or colons.
    """
    # Handle the case where the input is a NumPy array
    if isinstance(input_data, np.ndarray):
        # Flatten the array and take the first four elements
        float_numbers = list(input_data.flatten()[:4])
    elif isinstance(input_data, str):
        # Replace colons with spaces and split the string
        parts = input_data.replace(":", " ").split()

        # Filter and convert to float
        float_numbers = [float(part) for part in parts if part.replace('.', '', 1).replace('-', '', 1).isdigit()]
    else:
        raise TypeError("Input must be a string or a numpy array")

    # Ensure we have exactly 4 numbers, pad with zeros if necessary/f
    while len(float_numbers) < 4:
        float_numbers.append(0.0)

    return np.array(float_numbers[:4])

class TinyMPCSerialInterface:
    def __init__(self, port, baudrate=115200, timeout=0.5, wait_for_banner=True, debug=True):
        self.debug = debug
        self.ser = serial.Serial(port, baudrate=baudrate, timeout=timeout)
        time.sleep(2)  # Allow board reset

        if wait_for_banner:
            if self.debug:
                print("[TinyMPC] Waiting for boot banner...")
            while True:
                line = self.ser.readline().decode(errors="ignore").strip()
                if line:
                    if self.debug:
                        print(f"[TinyMPC][BOOT] {line}")
                    break

    def send_state(self, observation):
        state_str = extract_state_inputs(observation)
        if self.debug:
            print(f"[TinyMPC][TX] {state_str}")
        self.ser.write((state_str + "\n").encode('ascii'))

    def read_action(self):
        while True:
            line = self.ser.readline().decode(errors='ignore').strip()
            if not line:
                continue
            if self.debug:
                print(f"[TinyMPC][RX] {line}")
            if line.startswith("u = ["):
                try:
                    parts = line.split('[')[1].split(']')[0]
                    values = [float(s.strip()) for s in parts.split()]
                    return np.array(values)
                except Exception as e:
                    raise ValueError(f"Malformed response: {line}") from e
            else:
                if self.debug:
                    print(f"[TinyMPC][WARN] Unexpected line: {line}")

    def format_state(self, observation):
        obs = observation[0]
        x, y, z = obs[0:3]
        qx, qy, qz, qw = obs[3:7]
        r1, r2, r3 = qx/qw, qy/qw, qz/qw
        vx, vy, vz = obs[10:13]
        dphi, dtheta, dpsi = obs[13:16]
        return f"{x} {y} {z} {r1} {r2} {r3} {vx} {vy} {vz} {dphi} {dtheta} {dpsi}\n"

    def close(self):
        self.ser.close()


def run(
        drone=DEFAULT_DRONES,
        num_drones=DEFAULT_NUM_DRONES,
        physics=DEFAULT_PHYSICS,
        gui=DEFAULT_GUI,
        record_video=DEFAULT_RECORD_VISION,
        plot=DEFAULT_PLOT,
        user_debug_gui=DEFAULT_USER_DEBUG_GUI,
        obstacles=DEFAULT_OBSTACLES,
        simulation_freq_hz=DEFAULT_SIMULATION_FREQ_HZ,
        control_freq_hz=DEFAULT_CONTROL_FREQ_HZ,
        duration_sec=DEFAULT_DURATION_SEC,
        output_folder=DEFAULT_OUTPUT_FOLDER,
        colab=DEFAULT_COLAB
        ):
    #### Initialize the simulation #############################
    # H = .1
    H = .02
    H_STEP = .05
    R = .3

    MOTOR_TAU = 0.7 * 10e-6

    INIT_XYZS = np.array([[R*np.cos((i/6)*2*np.pi+np.pi/2), R*np.sin((i/6)*2*np.pi+np.pi/2)-R, H+i*H_STEP] for i in range(num_drones)])
    INIT_RPYS = np.array([[0.1, 0.1,  i * (np.pi/2)/num_drones] for i in range(num_drones)])


    #### Create the environment ################################
    env = CtrlAviary(drone_model=drone,
                        num_drones=num_drones,
                        initial_xyzs=INIT_XYZS,
                        initial_rpys=INIT_RPYS,
                        physics=physics,
                        neighbourhood_radius=10,
                        pyb_freq=simulation_freq_hz,
                        ctrl_freq=control_freq_hz,
                        gui=gui,
                        record=record_video,
                        obstacles=obstacles,
                        user_debug_gui=user_debug_gui
                        )

    #### Obtain the PyBullet Client ID from the environment ####
    PYB_CLIENT = env.getPyBulletClient()

    # Create sliders in the PyBullet UI
    target_x_slider = p.addUserDebugParameter("Target X", -2, 2, 0)
    target_y_slider = p.addUserDebugParameter("Target Y", -2, 2, 0)
    target_z_slider = p.addUserDebugParameter("Target Z", 0, 2, 1)

    # Create a purely visual target marker (no collision)
    target_visual = p.createVisualShape(shapeType=p.GEOM_SPHERE, radius=0.02, rgbaColor=[1, 0, 0, 0.5])
    target_marker_id = p.createMultiBody(
        baseMass=0,  # static
        baseVisualShapeIndex=target_visual,
        baseCollisionShapeIndex=-1,  # no collision
        basePosition=[0, 0, 1]
    )

    #### Initialize the logger #################################
    logger = Logger(logging_freq_hz=control_freq_hz,
                    num_drones=num_drones,
                    output_folder=output_folder,
                    colab=colab
                    )


    #### Run the simulation ####################################
    action = np.zeros((num_drones,4))
    START = time.time()

    tinympc = TinyMPCSerialInterface(port=UART_PORT)


    for i in range(0, int(duration_sec*env.CTRL_FREQ)):
        #### Step the simulation ###################################
        obs, _, _, _, _ = env.step(action)
        # print(f"obs: {obs}")

        target_x = p.readUserDebugParameter(target_x_slider)
        target_y = p.readUserDebugParameter(target_y_slider)
        target_z = p.readUserDebugParameter(target_z_slider)

        target_pos = np.array([target_x, target_y, target_z])

        # Update visual marker position
        p.resetBasePositionAndOrientation(target_marker_id, target_pos, [0, 0, 0, 1])

        obs[0][0] -= target_x;
        obs[0][1] -= target_y;
        obs[0][2] -= target_z - 0.3;

        tinympc.send_state(obs)
        try:
            forces = tinympc.read_action()
        except ValueError as e:
            print(e)
            forces = np.zeros(4)
        rpm = calculate_rpm(env, forces)
        # print(f"=========== RPM {rpm}")
        action[0] = rpm
        #time.sleep(0.5)
        


        #### Printout ##############################################
        env.render()

        #### Sync the simulation ###################################
        if gui:
            sync(i, START, env.CTRL_TIMESTEP)

    #### Close the environment #################################
    env.close()

    # #### Save the simulation results ###########################
    logger.save()
    logger.save_as_csv("TinyMPC") # Optional CSV save

    #### Plot the simulation results ###########################
    if plot:
        logger.plot()

if __name__ == "__main__":
    #### Define and parse (optional) arguments for the script ##
    parser = argparse.ArgumentParser(description='Helix flight script using CtrlAviary and DSLPIDControl')
    parser.add_argument('--drone',              default=DEFAULT_DRONES,     type=DroneModel,    help='Drone model (default: CF2X)', metavar='', choices=DroneModel)
    parser.add_argument('--num_drones',         default=DEFAULT_NUM_DRONES,          type=int,           help='Number of drones (default: 3)', metavar='')
    parser.add_argument('--physics',            default=DEFAULT_PHYSICS,      type=Physics,       help='Physics updates (default: PYB)', metavar='', choices=Physics)
    parser.add_argument('--gui',                default=DEFAULT_GUI,       type=str2bool,      help='Whether to use PyBullet GUI (default: True)', metavar='')
    parser.add_argument('--record_video',       default=DEFAULT_RECORD_VISION,      type=str2bool,      help='Whether to record a video (default: False)', metavar='')
    parser.add_argument('--plot',               default=DEFAULT_PLOT,       type=str2bool,      help='Whether to plot the simulation results (default: True)', metavar='')
    parser.add_argument('--user_debug_gui',     default=DEFAULT_USER_DEBUG_GUI,      type=str2bool,      help='Whether to add debug lines and parameters to the GUI (default: False)', metavar='')
    parser.add_argument('--obstacles',          default=DEFAULT_OBSTACLES,       type=str2bool,      help='Whether to add obstacles to the environment (default: True)', metavar='')
    parser.add_argument('--simulation_freq_hz', default=DEFAULT_SIMULATION_FREQ_HZ,        type=int,           help='Simulation frequency in Hz (default: 240)', metavar='')
    parser.add_argument('--control_freq_hz',    default=DEFAULT_CONTROL_FREQ_HZ,         type=int,           help='Control frequency in Hz (default: 48)', metavar='')
    parser.add_argument('--duration_sec',       default=DEFAULT_DURATION_SEC,         type=int,           help='Duration of the simulation in seconds (default: 5)', metavar='')
    parser.add_argument('--output_folder',     default=DEFAULT_OUTPUT_FOLDER, type=str,           help='Folder where to save logs (default: "results")', metavar='')
    parser.add_argument('--colab',              default=DEFAULT_COLAB, type=bool,           help='Whether example is being run by a notebook (default: "False")', metavar='')
    ARGS = parser.parse_args()

    run(**vars(ARGS))
        
        
        
        # print(f'motor observation: {obs[0][16:20]}')
        # action[j,:] = update_motor_rpm(obs[0][16:20], action[j,:], MOTOR_TAU,control_freq_hz)
        # print(f'esp32  action (filtered)[{j}]: {action[j,:]}')
