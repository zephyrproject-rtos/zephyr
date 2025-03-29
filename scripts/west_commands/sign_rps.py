#SPDX - License - Identifier : Apache - 2.0#!/ usr / bin / env python3
#Copyright(c) 2018 Foundries.io
#
import argparse
import os
import subprocess
from west.configuration import config
parser = argparse.ArgumentParser(description = "Sign a Zephyr binary using SimplicityCommander")
parser.add_argument('--m4_ota_key', help = "M4 OTA Key (hex string)")
parser.add_argument('--m4_private_key', help = "M4 Private Key (file path)")
parser.add_argument('--input', required = True, help = "Input file path")
parser.add_argument('--output', required = True, help = "Output file path")
args = parser.parse_args()
#Get the workspace root by navigating up from $PWD to the 'workspace' directory
workspace_root = os.path.abspath(os.path.join(os.environ['PWD'], '..')) #Go up one level from 'build' to 'workspace'
SL_COMMANDER_PATH = os.path.join(workspace_root, 'SimplicityCommander-Linux/commander/commander')
#Check if SimplicityCommander exists and is executable
if not os.path.isfile(SL_COMMANDER_PATH) or not os.access(SL_COMMANDER_PATH, os.X_OK) :
	raise FileNotFoundError(f "SimplicityCommander not found or not executable at '{SL_COMMANDER_PATH}'. "
"Please ensure it exists and is executable at '<workspace_root>/SimplicityCommander-Linux/commander/commander'.")
#Keep m4_ota_key and m4_private_key configurable via.west / config as in original code
m4_ota_key = args.m4_ota_key or config.get('sign.customtool', 'm4-ota-key', fallback = None)
m4_private_key = args.m4_private_key or config.get('sign.customtool', 'm4-private-key', fallback = None)
if not m4_ota_key or not m4_private_key:
	raise ValueError("Missing required values: m4-ota-key or m4-private-key must be provided via args or .west/config [sign.customtool]")
if not os.path.isabs(m4_private_key) :
	m4_private_key = os.path.abspath(os.path.join(workspace_root, 'zephyr', m4_private_key))
def commander_function(command_list) :
	print(f"Running commander command: {' '.join(command_list)}")
	process = subprocess.run(command_list, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
	stdout = process.stdout.decode('utf-8')
	stderr = process.stderr.decode('utf-8')
	print(f"STDOUT: {stdout}")
	print(f"STDERR: {stderr}")
	process.check_returncode() #Raises CalledProcessError if non - zero exit
SECURE_M4_RPS = 1
if SECURE_M4_RPS:create_m4_secure_rps =[SL_COMMANDER_PATH, 'rps', 'convert', args.output, '--app', args.input, '--mic', m4_ota_key, '--encrypt', m4_ota_key, '--sign', m4_private_key]
if SECURE_M4_RPS:commander_function(create_m4_secure_rps)
print(f"Signed {args.input} -> {args.output}")