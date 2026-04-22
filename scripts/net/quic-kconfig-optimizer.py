#!/usr/bin/env python3
#
# Copyright (c) 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0
#

"""
Zephyr QUIC Kconfig Optimizer
==============================
Computes optimal Kconfig values for the QUIC subsystem balancing
memory usage and throughput, given user constraints.

Usage (interactive):
    python quic-kconfig-optimizer.py

Usage (CLI):
    python quic-kconfig-optimizer.py \
        --contexts 2 --streams-bidi 3 --streams-uni 0 \
        --mtu 1500 --ram-budget 65536 \
        --role client --ip-version ipv4 \
        --rtt-ms 50 --loss-rate 0 \
        --app-message-size 4096 \
        --ooo-expected no --network-type lan
"""

import argparse
import math
import sys

# Constants / fixed overheads (bytes)
SENT_PKT_ENTRY_BYTES = 24  # per entry in sent-packet history ring buffer
TLS_CONTEXT_OVERHEAD = 8192  # rough mbedTLS per-connection overhead
STREAM_STATE_OVERHEAD = 128  # misc per-stream state (not buffers)
CONN_STATE_OVERHEAD = 512  # misc per-connection state


def ask(prompt, default=None, cast=str, choices=None, min_val=None, max_val=None):
    hint = f" [{default}]" if default is not None else ""
    if choices:
        hint += f" ({'/'.join(str(c) for c in choices)})"
    while True:
        raw = input(f"{prompt}{hint}: ").strip()
        if raw == "" and default is not None:
            return cast(default)
        try:
            val = cast(raw)
        except (ValueError, TypeError):
            print(f"  x Expected {cast.__name__}. Try again.")
            continue
        if choices and val not in choices:
            print(f"  x Must be one of {choices}.")
            continue
        if min_val is not None and val < min_val:
            print(f"  x Must be >= {min_val}.")
            continue
        if max_val is not None and val > max_val:
            print(f"  x Must be <= {max_val}.")
            continue
        return val


def next_power_of_two(n):
    return 1 << math.ceil(math.log2(max(n, 1)))


def align_up(n, align):
    return ((n + align - 1) // align) * align


def human(n):
    for unit in ("B", "KiB", "MiB", "GiB"):
        if n < 1024:
            return f"{n:.0f} {unit}"
        n /= 1024
    return f"{n:.1f} GiB"


def compute_config(p):
    cfg, mem = {}, {}

    mtu = p["mtu"]
    contexts = p["contexts"]
    streams_bidi = p["streams_bidi"]
    streams_uni = p["streams_uni"]
    role = p["role"]
    ip_ver = p["ip_version"]
    rtt_ms = p["rtt_ms"]
    loss_rate = p["loss_rate"]
    msg_size = p["app_message_size"]
    ooo = p["ooo_expected"]
    net_type = p["network_type"]
    ram_budget = p["ram_budget"]
    total_streams = contexts * (streams_bidi + streams_uni)

    # Endpoints
    endpoints = 3 if (ip_ver == "both" and role in ("server", "both")) else 2
    cfg["QUIC_MAX_ENDPOINTS"] = endpoints
    cfg["QUIC_PKT_COUNT"] = endpoints

    # TX / endpoint buffers, must be at least MTU, clamped to [1280, 1500]
    tx_buf = max(1280, min(mtu, 1500))
    cfg["QUIC_TX_BUFFER_SIZE"] = tx_buf
    cfg["QUIC_ENDPOINT_PENDING_DATA_LEN"] = tx_buf

    # Stream buffers sized to bandwidth-delay product or 2x message size
    bdp_estimate = max(tx_buf, msg_size * 2)
    stream_buf = max(1500, min(align_up(bdp_estimate, 512), 65535))
    if net_type == "wan" or loss_rate >= 2:  # keep pipeline full on lossy/slow links
        stream_buf = min(stream_buf * 2, 65535)
    cfg["QUIC_STREAM_TX_BUFFER_SIZE"] = stream_buf
    cfg["QUIC_STREAM_RX_BUFFER_SIZE"] = stream_buf

    # OOO slots
    ooo_slots = (8 if net_type == "wan" else 4) if ooo else 2
    ooo_seg = max(512, min(mtu, 1280)) if ooo else 512
    cfg["QUIC_STREAM_OOO_SLOTS"] = ooo_slots
    cfg["QUIC_STREAM_OOO_SEG_SIZE"] = ooo_seg

    # CRYPTO stream reassembly buffer is shared across all handshake levels per endpoint.
    # Servers and WAN clients (e.g. interoperating with browsers) need at least 4096 B
    # because Chrome/Firefox may split a ClientHello into 10+ CRYPTO frame fragments.
    # Embedded-to-embedded clients with simple certificates can use 2048 B.
    # Clamped to Kconfig range [1024, 8192].
    if role in ("server", "both") or net_type == "wan":
        crypto_rx_buf = 4096
    elif net_type == "loopback":
        crypto_rx_buf = 1024
    else:
        crypto_rx_buf = 2048
    cfg["QUIC_CRYPTO_RX_BUFFER_SIZE"] = crypto_rx_buf

    # CRYPTO OOO slots tracks metadata for out-of-order CRYPTO frame fragments.
    # Servers and WAN deployments need 8 slots (Chrome sends up to 10+ fragments).
    # Embedded clients talking to embedded peers only need the minimum of 4.
    # Clamped to Kconfig range [4, 16].
    if role in ("server", "both") or net_type == "wan":
        crypto_ooo_slots = 8
    else:
        crypto_ooo_slots = 4
    cfg["QUIC_CRYPTO_OOO_SLOTS"] = crypto_ooo_slots

    # Sent-packet history must cover at least BDP/MTU worth of in-flight packets
    min_hist = max(16, math.ceil(stream_buf / tx_buf) * 2)
    if net_type == "wan":
        min_hist = max(min_hist, 64)
    if loss_rate >= 5:
        min_hist = max(min_hist, 128)
    cfg["QUIC_SENT_PKT_HISTORY_SIZE"] = min(next_power_of_two(min_hist), 256)

    # Flow-control transport parameters match advertised window to our buffer capacity
    cfg["QUIC_INITIAL_MAX_STREAM_DATA_BIDI_LOCAL"] = stream_buf
    cfg["QUIC_INITIAL_MAX_STREAM_DATA_BIDI_REMOTE"] = stream_buf
    cfg["QUIC_INITIAL_MAX_STREAM_DATA_UNI"] = stream_buf if streams_uni > 0 else 16384
    cfg["QUIC_INITIAL_MAX_DATA"] = max(
        streams_bidi * stream_buf + streams_uni * stream_buf, stream_buf
    )
    cfg["QUIC_INITIAL_MAX_STREAMS_BIDI"] = streams_bidi
    cfg["QUIC_INITIAL_MAX_STREAMS_UNI"] = streams_uni

    # Window update, more frequent on high-latency WAN to refill windows sooner
    cfg["QUIC_STREAM_RX_WINDOW_UPDATE_THRESHOLD"] = (
        15 if (net_type == "wan" and rtt_ms > 100) else 25
    )

    # Timeouts
    idle = {"wan": 30000, "lan": 10000, "loopback": 5000}[net_type]
    cfg["QUIC_MAX_IDLE_TIMEOUT"] = idle
    cfg["QUIC_CONNECT_TIMEOUT"] = min(max(3000, rtt_ms * 10), 60000)
    cfg["QUIC_MAX_PTO_TIMEOUT_MS"] = max(1000, min(idle // 2, 10000))

    # TLS transcript buffer will default 4096 in Kconfig.
    # 4096 B covers typical TLS 1.3 handshakes with RSA-2048 / EC P-256 certs.
    # Increase to 6144-8192 for long certificate chains or RSA-4096 certificates.
    cfg["QUIC_TLS_TRANSCRIPT_BUF_LEN"] = 4096

    # Pass-through constraints
    cfg["QUIC_MAX_CONTEXTS"] = contexts
    cfg["QUIC_MAX_STREAMS_BIDI"] = streams_bidi
    cfg["QUIC_MAX_STREAMS_UNI"] = streams_uni

    # Memory accounting
    mem["Endpoint RX staging"] = endpoints * tx_buf * endpoints  # pkts * buf
    mem["Endpoint TX buffers"] = endpoints * tx_buf
    mem["Crypto RX buffers"] = endpoints * crypto_rx_buf
    mem["Crypto OOO slots"] = endpoints * crypto_ooo_slots * 8  # ~8 B per slot
    mem["Stream TX buffers"] = total_streams * stream_buf
    mem["Stream RX buffers"] = total_streams * stream_buf
    mem["Stream OOO buffers"] = total_streams * ooo_slots * ooo_seg
    mem["Stream state overhead"] = total_streams * STREAM_STATE_OVERHEAD
    mem["Sent-packet history"] = contexts * cfg["QUIC_SENT_PKT_HISTORY_SIZE"] * SENT_PKT_ENTRY_BYTES
    mem["TLS context overhead"] = contexts * TLS_CONTEXT_OVERHEAD
    mem["Connection state overhead"] = contexts * CONN_STATE_OVERHEAD
    mem["TLS transcript buffers"] = contexts * cfg["QUIC_TLS_TRANSCRIPT_BUF_LEN"]
    mem["TOTAL"] = sum(mem.values())

    if 0 < ram_budget < mem["TOTAL"]:
        raise ValueError(
            f"Estimated memory {human(mem['TOTAL'])} exceeds budget {human(ram_budget)}.\n"
            "Reduce: contexts, streams, stream_buf sizes, or OOO slots."
        )

    return cfg, mem


def print_report(p, cfg, mem):
    SEP = "-" * 72
    print(f"\n{'=' * 72}")
    print(f"{'QUIC Kconfig Optimizer -- Recommended Settings':^72}")
    print(f"{'=' * 72}")

    print(f"\n{SEP}\n  INPUT PARAMETERS\n{SEP}")
    rows = [
        ("Max connections (QUIC_MAX_CONTEXTS)", p["contexts"]),
        ("Bidi streams per connection", p["streams_bidi"]),
        ("Uni  streams per connection", p["streams_uni"]),
        ("MTU (bytes)", p["mtu"]),
        ("RAM budget", human(p["ram_budget"]) if p["ram_budget"] else "unlimited"),
        ("Device role", p["role"]),
        ("IP version", p["ip_version"]),
        ("Expected RTT (ms)", p["rtt_ms"]),
        ("Expected packet loss (%)", p["loss_rate"]),
        ("Typical message size (bytes)", p["app_message_size"]),
        ("Out-of-order delivery expected", "yes" if p["ooo_expected"] else "no"),
        ("Network type", p["network_type"]),
    ]
    for label, val in rows:
        print(f"  {label:<46} {val}")

    sections = [
        (
            "Connection / stream counts",
            [
                "QUIC_MAX_CONTEXTS",
                "QUIC_MAX_STREAMS_BIDI",
                "QUIC_MAX_STREAMS_UNI",
                "QUIC_MAX_ENDPOINTS",
                "QUIC_PKT_COUNT",
            ],
        ),
        (
            "QUIC transport parameters (flow control)",
            [
                "QUIC_INITIAL_MAX_DATA",
                "QUIC_INITIAL_MAX_STREAM_DATA_BIDI_LOCAL",
                "QUIC_INITIAL_MAX_STREAM_DATA_BIDI_REMOTE",
                "QUIC_INITIAL_MAX_STREAM_DATA_UNI",
                "QUIC_INITIAL_MAX_STREAMS_BIDI",
                "QUIC_INITIAL_MAX_STREAMS_UNI",
                "QUIC_STREAM_RX_WINDOW_UPDATE_THRESHOLD",
            ],
        ),
        (
            "Buffer sizes (RAM)",
            [
                "QUIC_ENDPOINT_PENDING_DATA_LEN",
                "QUIC_TX_BUFFER_SIZE",
                "QUIC_CRYPTO_RX_BUFFER_SIZE",
                "QUIC_CRYPTO_OOO_SLOTS",
                "QUIC_STREAM_TX_BUFFER_SIZE",
                "QUIC_STREAM_RX_BUFFER_SIZE",
                "QUIC_STREAM_OOO_SLOTS",
                "QUIC_STREAM_OOO_SEG_SIZE",
                "QUIC_SENT_PKT_HISTORY_SIZE",
                "QUIC_TLS_TRANSCRIPT_BUF_LEN",
            ],
        ),
        ("Timeouts", ["QUIC_MAX_IDLE_TIMEOUT", "QUIC_CONNECT_TIMEOUT", "QUIC_MAX_PTO_TIMEOUT_MS"]),
    ]

    print(f"\n{SEP}\n  RECOMMENDED Kconfig VALUES\n{SEP}")
    for title, keys in sections:
        print(f"\n  [{title}]")
        for k in keys:
            print(f"    CONFIG_{k:<50} {cfg.get(k, 'n/a')}")

    print(f"\n{SEP}\n  ESTIMATED RAM USAGE\n{SEP}")
    total = mem["TOTAL"]
    for label, n in mem.items():
        bar = "#" * int(n / total * 36)
        marker = " <-- TOTAL" if label == "TOTAL" else ""
        print(f"  {label:<35} {human(n):>10}  {bar}{marker}")

    if p["ram_budget"]:
        pct = total / p["ram_budget"] * 100
        print(f"\n  Budget utilisation: {pct:.1f}%  ({human(total)} / {human(p['ram_budget'])})")

    print(f"\n{SEP}\n  prj.conf SNIPPET\n{SEP}\n")
    for _, keys in sections:
        for k in keys:
            print(f"CONFIG_{k}={cfg.get(k, 'n/a')}")

    print(f"""
{SEP}
  ADDITIONAL NOTES
{SEP}""")

    notes = [
        "QUIC_INITIAL_MAX_DATA must be >= any single stream's INITIAL_MAX_STREAM_DATA"
        " or the connection window becomes the bottleneck.",
        "Set QUIC_STREAM_RX_BUFFER_SIZE = QUIC_STREAM_TX_BUFFER_SIZE for"
        " symmetric request-response / echo patterns.",
        f"TLS transcript total: {cfg['QUIC_TLS_TRANSCRIPT_BUF_LEN']} x"
        f" {p['contexts']} contexts = {human(cfg['QUIC_TLS_TRANSCRIPT_BUF_LEN'] * p['contexts'])}.",
        f"Crypto RX buffer total: {cfg['QUIC_CRYPTO_RX_BUFFER_SIZE']} x"
        f" {cfg['QUIC_MAX_ENDPOINTS']} endpoints ="
        f" {human(cfg['QUIC_CRYPTO_RX_BUFFER_SIZE'] * cfg['QUIC_MAX_ENDPOINTS'])}."
        " Increase for browser interoperability (Chrome/Firefox).",
    ]
    for note in notes:
        print(f"  * {note}")
    print()


def gather_interactive():
    print("\n" + "=" * 60)
    print("  Zephyr QUIC Kconfig Optimizer -- Interactive Mode")
    print("=" * 60)
    print("  Press Enter to accept [default] values.\n")
    p = {}
    p["contexts"] = ask("Max QUIC connections (QUIC_MAX_CONTEXTS)", 1, int, min_val=1, max_val=255)
    p["streams_bidi"] = ask("Bidirectional streams per connection", 3, int, min_val=1, max_val=255)
    p["streams_uni"] = ask("Unidirectional streams per connection", 0, int, min_val=0, max_val=255)
    p["mtu"] = ask("Network MTU in bytes", 1500, int, min_val=1280, max_val=9000)
    p["ram_budget"] = ask("RAM budget in bytes (0 = no limit)", 0, int, min_val=0)
    p["role"] = ask("Device role", "client", str, choices=["client", "server", "both"])
    p["ip_version"] = ask("IP version(s)", "ipv4", str, choices=["ipv4", "ipv6", "both"])
    p["rtt_ms"] = ask("Expected RTT in ms", 10, int, min_val=1)
    p["loss_rate"] = ask("Expected packet loss rate (%)", 0.0, float, min_val=0.0, max_val=100.0)
    p["app_message_size"] = ask("Typical application message size (bytes)", 1024, int, min_val=1)
    p["ooo_expected"] = (
        ask("Expect out-of-order delivery?", "no", str, choices=["yes", "no"]) == "yes"
    )
    p["network_type"] = ask("Network type", "lan", str, choices=["lan", "wan", "loopback"])
    return p


def main():
    ap = argparse.ArgumentParser(
        description="Zephyr QUIC Kconfig Optimizer",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        allow_abbrev=False,
    )
    ap.add_argument("--contexts", type=int)
    ap.add_argument("--streams-bidi", type=int)
    ap.add_argument("--streams-uni", type=int)
    ap.add_argument("--mtu", type=int)
    ap.add_argument("--ram-budget", type=int, default=0, help="bytes; 0 = unlimited")
    ap.add_argument("--role", choices=["client", "server", "both"])
    ap.add_argument("--ip-version", choices=["ipv4", "ipv6", "both"])
    ap.add_argument("--rtt-ms", type=int)
    ap.add_argument("--loss-rate", type=float)
    ap.add_argument("--app-message-size", type=int)
    ap.add_argument("--ooo-expected", choices=["yes", "no"])
    ap.add_argument("--network-type", choices=["lan", "wan", "loopback"])
    args = ap.parse_args()

    required = [
        "contexts",
        "streams_bidi",
        "streams_uni",
        "mtu",
        "role",
        "ip_version",
        "rtt_ms",
        "loss_rate",
        "app_message_size",
        "ooo_expected",
        "network_type",
    ]
    cli = {
        "contexts": args.contexts,
        "streams_bidi": args.streams_bidi,
        "streams_uni": args.streams_uni,
        "mtu": args.mtu,
        "ram_budget": args.ram_budget,
        "role": args.role,
        "ip_version": args.ip_version,
        "rtt_ms": args.rtt_ms,
        "loss_rate": args.loss_rate,
        "app_message_size": args.app_message_size,
        "ooo_expected": args.ooo_expected,
        "network_type": args.network_type,
    }

    if all(cli.get(k) is not None for k in required):
        p = cli
        p["ooo_expected"] = p["ooo_expected"] == "yes"
    else:
        p = gather_interactive()

    try:
        cfg, mem = compute_config(p)
    except ValueError as e:
        print(f"\nERROR: {e}\n", file=sys.stderr)
        sys.exit(1)

    print_report(p, cfg, mem)


if __name__ == "__main__":
    main()
