import argparse
import contextlib
import subprocess
import tempfile
import time
from pathlib import Path


def wait_for(predicate, processes, deadline):
    while not predicate():
        for process in processes:
            if process.poll() is not None:
                raise RuntimeError(f"Process exited early: {process.args[0]}")
        if time.monotonic() >= deadline:
            raise TimeoutError("Timed out waiting for the server or guests")
        time.sleep(0.05)


def read_log(path):
    return path.read_text(errors="replace")


def run(args, logs):
    processes = []
    deadline = time.monotonic() + args.timeout

    # Keep the file-backed shared memory and socket independent of other runs.
    with (
        tempfile.TemporaryDirectory(prefix="ivshmem-flat-") as temporary,
        contextlib.ExitStack() as stack,
    ):

        def launch(command, name):
            output = stack.enter_context((logs / name).open("w"))
            process = subprocess.Popen(command, stdout=output, stderr=subprocess.STDOUT)
            processes.append(process)

        try:
            socket = Path(temporary) / "server.sock"
            launch(
                [
                    str(args.server),
                    "-F",
                    "-S",
                    str(socket),
                    "-m",
                    temporary,
                    "-l",
                    "1M",
                    "-n",
                    "1",
                ],
                "server.log",
            )
            wait_for(socket.exists, processes, min(deadline, time.monotonic() + 10))
            command = [
                str(args.qemu),
                "-machine",
                "lm3s6965evb",
                "-cpu",
                "cortex-m3",
                "-display",
                "none",
                "-monitor",
                "none",
                "-serial",
                "stdio",
                "-nic",
                "none",
                "-chardev",
                f"socket,id=ivshmem,path={socket}",
                "-kernel",
                str(args.kernel),
            ]
            launch(command, "vm0.log")
            wait_for(lambda: "Peer 0 ready" in read_log(logs / "vm0.log"), processes, deadline)
            launch(command, "vm1.log")

            def completed():
                texts = [read_log(logs / f"vm{peer}.log") for peer in range(2)]
                if any("FAIL:" in text for text in texts):
                    raise RuntimeError("A guest reported failure")
                return all(
                    f"PASS: peer {peer} completed 100 rounds" in text
                    for peer, text in enumerate(texts)
                )

            wait_for(completed, processes, deadline)
        finally:
            for process in reversed(processes):
                if process.poll() is None:
                    process.terminate()
            for process in reversed(processes):
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--qemu", required=True, type=Path, help="Patched QEMU 10.0 executable")
    parser.add_argument("--server", required=True, type=Path, help="ivshmem-server executable")
    parser.add_argument("--kernel", required=True, type=Path, help="Zephyr ELF image")
    parser.add_argument("--log-dir", type=Path, help="New directory in which to retain logs")
    parser.add_argument("--timeout", type=float, default=90, help="Overall timeout in seconds")
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error("--timeout must be positive")
    for name in ("qemu", "server", "kernel"):
        path = getattr(args, name).resolve()
        if not path.is_file():
            parser.error(f"{name} does not exist: {path}")
        setattr(args, name, path)
    if args.log_dir is None:
        logs = Path(tempfile.mkdtemp(prefix="ivshmem-flat-logs-"))
    else:
        logs = args.log_dir.resolve()
        logs.mkdir(parents=True, exist_ok=False)
    print(f"Logs: {logs}", flush=True)
    try:
        run(args, logs)
    except (OSError, RuntimeError, TimeoutError, KeyboardInterrupt) as error:
        print(f"FAIL: {error}")
        return 1
    print("PASS: both guests completed 100 ping-pong rounds")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
