#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

"""Exercise the Zephyr CANopen sample node from a Linux SocketCAN host."""

import argparse
import struct
import sys
import time

import canopen

OD_COUNTER = 0x2000
OD_OUTPUT = 0x2001
OD_DATA = 0x5600
DOMAIN_TEST_LEN = 200
DOMAIN_TOO_LONG = 300
DEVICE_NAME = "Zephyr CANopen sample"
EMCY_NO_ERROR = 0x0000
EMCY_HEARTBEAT = 0x8130
EMCY_PDO_LENGTH = 0x8210
ERR_REG_COMMUNICATION = 0x11
MONITORED_ID = 0x7E
MONITORED_HB_MS = 300
SDO_ABORT_NO_DATA = 0x08000024
GUARD_TIME_MS = 100
GUARD_FACTOR = 3
TIME_COB_ID = 0x100
TIME_CONSUMER = 0x80000100
TIME_BOTH = 0xC0000100
TIME_EPOCH = 441763200
MS_PER_DAY = 86400000


class Checker:
    """Collect and print the result of each check."""

    def __init__(self):
        self.failed = 0

    def check(self, name, ok, detail=""):
        print(f"{'PASS' if ok else 'FAIL'}: {name}" + (f" ({detail})" if detail else ""))
        if not ok:
            self.failed += 1


def read_int(node, index, sub):
    return int.from_bytes(node.sdo.upload(index, sub), "little")


def expect_abort(node, index, sub, data=None):
    try:
        if data is None:
            node.sdo.upload(index, sub)
        else:
            node.sdo.download(index, sub, data)
    except canopen.SdoAbortedError as err:
        while isinstance(err.__context__, canopen.SdoAbortedError):
            err = err.__context__
        return err.code
    return None


def initiate_download(node, index, sub, size):
    try:
        node.sdo.request_response(struct.pack("<BHBL", 0x21, index, sub, size))
    except canopen.SdoAbortedError as err:
        return err.code
    node.sdo.abort()
    return None


def abort_text(code):
    return "no abort" if code is None else f"0x{code:08X}"


def emcy_since(node, start, code, timeout=2.0):
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        if any(emcy.code == code for emcy in node.emcy.log[start:]):
            return True
        time.sleep(0.01)
    return False


def collect(network, cob_id, duration, syncs=0):
    frames = []

    def on_frame(can_id, data, timestamp):
        frames.append(bytes(data))

    network.subscribe(cob_id, on_frame)
    for _ in range(syncs):
        network.sync.transmit()
        time.sleep(duration)
    if syncs == 0:
        time.sleep(duration)
    network.unsubscribe(cob_id, on_frame)

    return frames


def check_time(network, node, result):
    result.check("1012h COB-ID TIME, consumer", read_int(node, 0x1012, 0) == TIME_CONSUMER)
    sent = time.time()
    network.time.transmit(sent)
    time.sleep(0.1)
    node.sdo.download(0x1012, 0, TIME_BOTH.to_bytes(4, "little"))
    frames = collect(network, TIME_COB_ID, 2.5)
    node.sdo.download(0x1012, 0, TIME_CONSUMER.to_bytes(4, "little"))
    expected = (sent - TIME_EPOCH) * 1000
    received = [
        int.from_bytes(f[4:6], "little") * MS_PER_DAY
        + (int.from_bytes(f[0:4], "little") & 0xFFFFFFF)
        for f in frames
        if len(f) == 6
    ]
    result.check(
        "TIME produced from the received time",
        len(received) >= 2 and all(0 <= t - expected <= 4000 for t in received),
        f"{len(received)} frames, {[int(t - expected) for t in received]} ms after the sent time",
    )


def check_guarding(network, node, result):
    cob_id = 0x700 + node.id
    node.sdo.download(0x1017, 0, bytes(2))
    node.sdo.download(0x100C, 0, GUARD_TIME_MS.to_bytes(2, "little"))
    node.sdo.download(0x100D, 0, bytes([GUARD_FACTOR]))
    node.nmt.state = "OPERATIONAL"
    time.sleep(0.1)

    guard = network.send_periodic(cob_id, None, GUARD_TIME_MS / 1000, remote=True)
    answers = [f[0] for f in collect(network, cob_id, 0.55) if len(f) == 1]
    toggles = [a >> 7 for a in answers]
    result.check(
        "node guarding answers with alternating toggle in OPERATIONAL",
        len(answers) >= 4
        and all(a & 0x7F == 0x05 for a in answers)
        and all(toggles[i] != toggles[i + 1] for i in range(len(toggles) - 1)),
        " ".join(f"{a:02X}" for a in answers),
    )

    mark = len(node.emcy.log)
    guard.stop()
    result.check(
        "EMCY 8130h when the guarding stops (life guarding)",
        emcy_since(node, mark, EMCY_HEARTBEAT),
    )

    mark = len(node.emcy.log)
    guard = network.send_periodic(cob_id, None, GUARD_TIME_MS / 1000, remote=True)
    answers = [f[0] for f in collect(network, cob_id, 0.25) if len(f) == 1]
    guard.stop()
    node.sdo.download(0x100D, 0, bytes(1))
    result.check(
        "error behavior: OPERATIONAL to PRE-OPERATIONAL after life guarding",
        len(answers) > 0 and all(a & 0x7F == 0x7F for a in answers),
        " ".join(f"{a:02X}" for a in answers),
    )
    result.check("EMCY reset when the guarding resumes", emcy_since(node, mark, EMCY_NO_ERROR))
    node.sdo.download(0x1017, 0, (1000).to_bytes(2, "little"))


def check_domain(node, result):
    data = bytes((i * 37 + 11) & 0xFF for i in range(DOMAIN_TEST_LEN))
    node.sdo.download(OD_DATA, 0, data)
    back = node.sdo.upload(OD_DATA, 0)
    result.check(f"5600h DOMAIN, {len(data)} bytes written and read back", back == data)
    code = initiate_download(node, OD_DATA, 0, DOMAIN_TOO_LONG)
    result.check(
        f"5600h DOMAIN, {DOMAIN_TOO_LONG} bytes refused at initiate",
        code == 0x06070012,
        abort_text(code),
    )


def check_block(node, result):
    data = bytes((i * 53 + 7) & 0xFF for i in range(DOMAIN_TEST_LEN))
    with node.sdo.open(OD_DATA, 0, "wb", size=len(data), block_transfer=True) as fp:
        fp.write(data)
    with node.sdo.open(OD_DATA, 0, "rb", block_transfer=True) as fp:
        back = fp.read()
    result.check(f"5600h DOMAIN, {len(data)} bytes block download and upload", back == data)
    with node.sdo.open(0x1008, 0, "rb", block_transfer=True) as fp:
        name = fp.read().decode()
    result.check("1008h device name, block upload", name == DEVICE_NAME, name)


def eds_variables(od):
    for obj in od.values():
        if isinstance(obj, canopen.objectdictionary.ODVariable):
            if obj.index > 0x1B:
                yield obj
        else:
            yield from obj.values()


def check_eds(node, path, result):
    try:
        od = canopen.import_od(path, node.id)
    except (OSError, ValueError) as err:
        result.check(f"EDS {path} loaded", False, str(err))
        return
    count = 0
    for var in eds_variables(od):
        if var.access_type == "wo":
            continue
        name = f"EDS {var.index:04X}h sub {var.subindex}"
        try:
            raw = node.sdo.upload(var.index, var.subindex)
        except canopen.SdoAbortedError as err:
            if var.index == 0x1003 and err.code == SDO_ABORT_NO_DATA:
                continue
            result.check(name, False, abort_text(err.code))
            continue
        count += 1
        if var.data_type in canopen.objectdictionary.datatypes.DATA_TYPES:
            continue
        expected = var.encode_raw(var.default)
        if len(raw) != len(expected):
            result.check(name, False, f"{len(raw)} bytes, EDS has {len(expected)}")
        elif (
            (var.access_type == "const" or 0x1000 <= var.index <= 0x1FFF)
            and var.index not in (0x1001, 0x1003)
            and raw != expected
        ):
            result.check(name, False, f"{raw.hex()}, EDS default {expected.hex()}")
    result.check(f"every readable object of the EDS read over SDO ({count})", count > 0)


def main():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("--channel", default="can0", help="SocketCAN interface")
    parser.add_argument("--node-id", type=lambda v: int(v, 0), default=0x10, help="node-ID")
    parser.add_argument("--eds", help="canopen.eds of the build, checked against the node")
    args = parser.parse_args()

    tpdo1 = 0x180 + args.node_id
    rpdo1 = 0x200 + args.node_id
    result = Checker()

    network = canopen.Network()
    network.connect(interface="socketcan", channel=args.channel)
    node = network.add_node(args.node_id, canopen.ObjectDictionary())

    try:
        node.nmt.send_command(0x82)
        node.nmt.wait_for_bootup(5)
        result.check("boot-up after NMT reset communication", True)

        state = node.nmt.wait_for_heartbeat(3)
        result.check("heartbeat in PRE-OPERATIONAL", state == "PRE-OPERATIONAL", state)

        result.check("1000h device type", read_int(node, 0x1000, 0) == 0)
        result.check("1018h highest sub-index", read_int(node, 0x1018, 0) == 4)
        result.check("1018h vendor-ID", read_int(node, 0x1018, 1) == 0)
        name = node.sdo.upload(0x1008, 0).decode("ascii", "replace")
        result.check("1008h device name (segmented)", name == DEVICE_NAME, name)
        result.check("1017h producer heartbeat time", read_int(node, 0x1017, 0) == 1000)

        if args.eds is not None:
            check_eds(node, args.eds, result)

        node.sdo.download(OD_OUTPUT, 0, bytes([0x55]))
        result.check("SDO download and upload 2001h", read_int(node, OD_OUTPUT, 0) == 0x55)

        code = expect_abort(node, 0x2FFF, 0)
        result.check("abort for missing object", code == 0x06020000, abort_text(code))
        code = expect_abort(node, OD_COUNTER, 0, bytes(4))
        result.check("abort for read-only object", code == 0x06010002, abort_text(code))
        check_domain(node, result)
        check_block(node, result)

        node.nmt.state = "OPERATIONAL"
        state = node.nmt.wait_for_heartbeat(3)
        result.check("heartbeat in OPERATIONAL", state == "OPERATIONAL", state)

        frames = collect(network, tpdo1, 2.5)
        counters = [int.from_bytes(data, "little") for data in frames]
        result.check(
            "TPDO1 on event timer",
            len(counters) >= 2 and counters == sorted(counters),
            f"{len(counters)} frames, counter {counters}",
        )

        network.send_message(rpdo1, bytes([0xA5]))
        time.sleep(0.1)
        result.check("RPDO1 writes 2001h", read_int(node, OD_OUTPUT, 0) == 0xA5)

        node.sdo.download(0x1800, 2, bytes([1]))
        frames = collect(network, tpdo1, 0.1, syncs=3)
        result.check("TPDO1 on every SYNC", len(frames) == 3, f"{len(frames)} frames")
        node.sdo.download(0x1800, 2, bytes([0xFE]))

        mark = len(node.emcy.log)
        network.send_message(rpdo1, b"")
        result.check("EMCY 8210h for a too short RPDO", emcy_since(node, mark, EMCY_PDO_LENGTH))
        result.check(
            "1001h error register while RPDO error is active",
            read_int(node, 0x1001, 0) == ERR_REG_COMMUNICATION,
        )
        history = read_int(node, 0x1003, 1)
        result.check("1003h error history", history & 0xFFFF == EMCY_PDO_LENGTH, f"0x{history:08X}")
        mark = len(node.emcy.log)
        network.send_message(rpdo1, bytes([0x5A]))
        result.check("EMCY reset after a correct RPDO", emcy_since(node, mark, EMCY_NO_ERROR))
        result.check("1001h cleared", read_int(node, 0x1001, 0) == 0)

        entry = (MONITORED_ID << 16) | MONITORED_HB_MS
        node.sdo.download(0x1016, 1, entry.to_bytes(4, "little"))
        heartbeat = network.send_periodic(0x700 + MONITORED_ID, bytes([0x05]), 0.1)
        time.sleep(0.5)
        mark = len(node.emcy.log)
        heartbeat.stop()
        result.check(
            "EMCY 8130h when the monitored heartbeat stops",
            emcy_since(node, mark, EMCY_HEARTBEAT),
        )
        state = node.nmt.wait_for_heartbeat(3)
        result.check(
            "error behavior: OPERATIONAL to PRE-OPERATIONAL", state == "PRE-OPERATIONAL", state
        )
        mark = len(node.emcy.log)
        heartbeat = network.send_periodic(0x700 + MONITORED_ID, bytes([0x05]), 0.1)
        result.check("EMCY reset when the heartbeat returns", emcy_since(node, mark, EMCY_NO_ERROR))
        heartbeat.stop()
        node.sdo.download(0x1016, 1, bytes(4))

        check_time(network, node, result)
        check_guarding(network, node, result)

        node.nmt.state = "STOPPED"
        state = node.nmt.wait_for_heartbeat(3)
        result.check("heartbeat in STOPPED", state == "STOPPED", state)
        try:
            node.sdo.upload(0x1000, 0)
            result.check("no SDO in STOPPED", False)
        except canopen.SdoCommunicationError:
            result.check("no SDO in STOPPED", True)

        node.nmt.state = "PRE-OPERATIONAL"
    except (canopen.SdoAbortedError, canopen.SdoCommunicationError, canopen.nmt.NmtError) as err:
        result.check("communication", False, str(err))
    finally:
        network.disconnect()

    print(f"\n{'ALL CHECKS PASSED' if result.failed == 0 else f'{result.failed} CHECK(S) FAILED'}")

    return 0 if result.failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
