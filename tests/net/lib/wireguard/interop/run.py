# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Run one Zephyr identity and independent Linux WireGuard peers in isolated namespaces."""

import argparse
import json
import os
import selectors
import signal
import socket
import subprocess
import sys
import time
from contextlib import ExitStack, contextmanager
from ipaddress import IPv4Address
from pathlib import Path

SCRIPT = str(Path(__file__).resolve())
PORT = 9000


def run(*args, stdin=None):
    return subprocess.check_output(args, input=stdin, text=True, timeout=10).strip()


@contextmanager
def child(*args):
    with subprocess.Popen(
        args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
    ) as proc:
        try:
            yield proc
        finally:
            if proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    proc.kill()


def in_ns(pid, *args):
    return ('nsenter', '--target', str(pid), '--net', '--', *args)


def overlay_ipv4(leaf):
    if leaf < 254:
        return str(IPv4Address('198.51.100.1') + leaf)
    return str(IPv4Address('203.0.113.1') + (leaf - 254))


def echo(leaf):
    with ExitStack() as stack:
        sel = stack.enter_context(selectors.DefaultSelector())
        for family, ip in [
            (socket.AF_INET, overlay_ipv4(leaf)),
            (socket.AF_INET6, f'2001:db8:1::{leaf + 1:x}'),
        ]:
            sock = stack.enter_context(socket.socket(family, socket.SOCK_DGRAM))
            sock.bind((ip, PORT))
            sel.register(sock, selectors.EVENT_READ)
        print('listening', flush=True)
        while True:
            for key, _ in sel.select():
                data, source = key.fileobj.recvfrom(2048)
                key.fileobj.sendto(data, source)


def setup_peer(stack, leaf):
    ns = stack.enter_context(child('unshare', '--net', '--', sys.executable, SCRIPT, '--hold'))
    if ns.stdout.readline().strip() != 'ready':
        raise RuntimeError('endpoint namespace failed to start')
    name = f'leaf{leaf}'
    run('ip', 'link', 'add', name, 'type', 'wireguard')
    run('ip', 'link', 'set', name, 'netns', str(ns.pid))
    prefix = in_ns(ns.pid)
    run(
        *prefix,
        'wg',
        'set',
        name,
        'private-key',
        '/dev/stdin',
        'listen-port',
        str(51000 + leaf),
        stdin=run('wg', 'genkey') + '\n',
    )
    run(*prefix, 'ip', 'addr', 'add', f'{overlay_ipv4(leaf)}/32', 'dev', name)
    run(*prefix, 'ip', '-6', 'addr', 'add', f'2001:db8:1::{leaf + 1:x}/128', 'dev', name, 'nodad')
    run(*prefix, 'ip', 'link', 'set', name, 'up')
    run(*prefix, 'ip', 'route', 'add', '198.51.100.1/32', 'dev', name)
    run(*prefix, 'ip', '-6', 'route', 'add', '2001:db8:1::1/128', 'dev', name)
    listener = stack.enter_context(child(*prefix, sys.executable, SCRIPT, '--echo', str(leaf)))
    if listener.stdout.readline().strip() != 'listening':
        raise RuntimeError(f'{name} echo server failed to start')
    return (prefix, name, run(*prefix, 'wg', 'show', name, 'public-key'))


def configure_remote(prefix, name, shared):
    run(
        *prefix,
        'wg',
        'set',
        name,
        'peer',
        shared,
        'allowed-ips',
        '198.51.100.1/32,2001:db8:1::1/128',
        'endpoint',
        '192.0.2.2:51820',
        'persistent-keepalive',
        '5',
    )


def verify(executable, peer_count):
    interfaces = json.loads(run('ip', '-j', 'link', 'show'))
    if [iface['ifname'] for iface in interfaces] != ['lo']:
        raise RuntimeError('expected a fresh network namespace')
    run('ip', 'link', 'set', 'lo', 'up')
    run('ip', 'tuntap', 'add', 'dev', 'zeth', 'mode', 'tap')
    run('ip', 'addr', 'add', '192.0.2.1/24', 'dev', 'zeth')
    run('ip', 'link', 'set', 'zeth', 'up')
    with ExitStack() as stack:
        peers = [setup_peer(stack, i) for i in range(1, peer_count + 1)]
        sim = stack.enter_context(
            child(
                'stdbuf',
                '-oL',
                executable,
                '-rt',
                '-eth-if=zeth',
                '-ipv4-addr=192.0.2.2',
                '-ipv4-nm=255.255.255.0',
                '-testargs',
                *(key for _, _, key in peers),
            )
        )
        shared = None
        success = False
        start = None
        for line in sim.stdout:
            if 'IPv4 measurement start' in line:
                start = time.monotonic()
            elif 'Verified 64 IPv4 round trips' in line:
                elapsed = time.monotonic() - start
                print(
                    f'{peer_count} peer(s), 64 verified IPv4 UDP round trips in '
                    f'{elapsed:.3f} wall seconds ({64 / elapsed:.1f} round trips/s)',
                    flush=True,
                )
            key_marker = 'Hub public key '
            if key_marker in line:
                shared = line.split(key_marker, 1)[1].strip()
                for prefix, name, _ in peers:
                    configure_remote(prefix, name, shared)
            elif 'Request fresh Linux peer 2 handshake' in line:
                prefix, name, _ = peers[1]
                run(*prefix, 'wg', 'set', name, 'peer', shared, 'remove')
                configure_remote(prefix, name, shared)
            else:
                print(line.rstrip(), flush=True)
            success |= 'PROJECT EXECUTION SUCCESSFUL' in line
        if sim.wait() != 0 or not shared or not success:
            raise RuntimeError('Zephyr nonce test failed')
        for prefix, name, _ in peers:
            peer, timestamp = run(*prefix, 'wg', 'show', name, 'latest-handshakes').split()
            if peer != shared or int(timestamp) <= 0:
                raise RuntimeError(f'{name} did not handshake with the shared identity')
            print(f'{name} confirmed handshake with the same Zephyr public key')


def timeout(_number, _frame):
    raise TimeoutError('Linux interop test exceeded its per-run deadline')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument('executable', type=Path, nargs='?')
    parser.add_argument('--isolated', action='store_true', help=argparse.SUPPRESS)
    parser.add_argument('--peers', type=int, default=3, choices=range(1, 257), metavar='N')
    parser.add_argument('--hold', action='store_true', help=argparse.SUPPRESS)
    parser.add_argument('--echo', type=int, help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.hold:
        print('ready', flush=True)
        signal.pause()
    elif args.echo:
        echo(args.echo)
    elif args.executable:
        executable = str(args.executable.resolve(strict=True))
        if not args.isolated:
            os.execvp(
                'unshare',
                (
                    'unshare',
                    '--mount',
                    '--net',
                    '--mount-proc',
                    '--propagation',
                    'private',
                    '--',
                    sys.executable,
                    SCRIPT,
                    '--isolated',
                    '--peers',
                    str(args.peers),
                    executable,
                ),
            )
        signal.signal(signal.SIGALRM, timeout)
        signal.alarm(100 + 2 * args.peers)
        verify(executable, args.peers)
    else:
        parser.error('pass the built native_sim executable')
