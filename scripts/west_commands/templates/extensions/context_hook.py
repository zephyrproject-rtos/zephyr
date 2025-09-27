import subprocess
import sys

from copier_template_extensions import ContextHook

# Cache for boards list to avoid repeated subprocess calls
_cached_boards = None


class ZephyrContextHook(ContextHook):
    def hook(self, context):
        context["categories"] = {
            "Application Development": "application_development",
            "Architecture-dependent Samples": "arch",
            "Basic": "basic",
            "Bluetooth": "bluetooth",
            "Boards": "boards",
            "C++": "cpp",
            "Data Structures": "data_structures",
            "Drivers": "drivers",
            "Hello World": "hello_world",
            "Kernel and Scheduler": "kernel",
            "External modules": "modules",
            "Networking": "net",
            "Philosophers": "philosophers",
            "POSIX API": "posix",
            "PSA": "psa",
            "Sensors": "sensor",
            "Shields": "shields",
            "Subsystems": "subsys",
            "Synchronization": "synchronization",
            "Sysbuild": "sysbuild",
            "Test": "test",
            "TF-M Integration": "tfm_integration",
            "Userspace": "userspace",
        }

        global _cached_boards
        if _cached_boards is None:
            try:
                # a bit dirty but will do for now :)
                result = subprocess.run(
                    ["west", "boards", "--format={name}"],
                    capture_output=True,
                    text=True,
                    check=True,
                )
                boards = [
                    line.strip() for line in result.stdout.strip().split('\n') if line.strip()
                ]
                _cached_boards = boards
            except (subprocess.CalledProcessError, FileNotFoundError) as e:
                # Fallback to a default list if west command fails
                print(f"Warning: Failed to get board list from 'west boards': {e}", file=sys.stderr)
                _cached_boards = ["wio_terminal"]

        context["valid_boards"] = _cached_boards
