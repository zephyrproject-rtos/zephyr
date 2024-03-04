#!/usr/bin/env python3

# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

import os
import subprocess
from pathlib import Path
from west.util import west_topdir
import concurrent.futures

dotslash = Path(__file__).parent.absolute()
os.chdir(dotslash)

BSIM_OUT_PATH = os.environ.get("BSIM_OUT_PATH")
if BSIM_OUT_PATH is None:
    print("ERROR: BSIM_OUT_PATH environment variable not set")
    exit(1)

BOARD = os.environ.get("BOARD", "nrf52_bsim")

ZEPHYR_BASE = west_topdir() + "/zephyr"


def bsim_exe_name(prj_conf: Path) -> str:
    return f"bs_{BOARD}_" + prj_conf.absolute().relative_to(
        ZEPHYR_BASE
    ).as_posix().replace("/", "_").replace(".", "_")


def bsim_exe_path(prj_conf: Path) -> Path:
    return Path(BSIM_OUT_PATH) / "bin" / bsim_exe_name(prj_conf)


devices = [
    "dut/prj.conf",
    "connecter/prj.conf",
    "connecter/prj.conf",
    "connecter/prj.conf",
    "connectable/prj.conf",
]

simulation_id = "resume2"

args_all = [f"-s={simulation_id}"]
args_dev = ["-v=2", "-RealEncryption=1"]
sim_seconds = 120

print(f"Simulation time: {sim_seconds} seconds")

realworld_timeout = 60

os.chdir(f"{BSIM_OUT_PATH}/bin")
with concurrent.futures.ThreadPoolExecutor() as executor:
    ps = set()
    for i, dev in enumerate(devices):
        compile_path = bsim_exe_path(dotslash / dev)
        p = subprocess.Popen(
            [
                compile_path,
                *args_all,
                *args_dev,
                f"-d={i}",
            ]
        )
        ps.add(p)
    p = subprocess.Popen(
        [
            "./bs_2G4_phy_v1",
            *args_all,
            f"-D={i+1}",
            "-v=6",
            f"-sim_length={sim_seconds * 10 ** 6}",
        ]
    )
    ps.add(p)
    done, not_done = concurrent.futures.wait(
        (executor.submit(p.wait) for p in ps),
        timeout=realworld_timeout,
    )
    if not_done:
        print(f"ERROR: Real-world timeout occurred after {realworld_timeout} seconds")
        for p in ps:
            p.kill()
    print("End of simulation")

csv2pcap = f"{BSIM_OUT_PATH}/components/ext_2G4_phy_v1/dump_post_process/csv2pcap"

os.chdir(f"{BSIM_OUT_PATH}/results/{simulation_id}")
subprocess.run(
    [csv2pcap, "-o", "d_2G4_all.Tx.pcap"] + [f"d_2G4_{i:02}.Tx.csv" for i in range(len(devices))],
    check=True,
)
