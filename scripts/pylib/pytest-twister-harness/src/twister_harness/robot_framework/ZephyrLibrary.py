import pickle

from robot.libraries.BuiltIn import BuiltIn
from twister_harness import DeviceAdapter, Shell
from twister_harness.device.factory import DeviceFactory
from twister_harness.twister_harness_config import TwisterHarnessConfig


class ZephyrLibrary:
    ROBOT_LIBRARY_SCOPE = "SUITE"

    def __init__(self):
        self.device: DeviceAdapter | None = None
        self.shell: Shell | None = None

        self.twister_harness_config = self._get_twister_harness_config()

    def _get_twister_harness_config(self) -> TwisterHarnessConfig:
        all_variables = BuiltIn().get_variables()
        twister_harness_config_file = all_variables.get(
            "${TWISTER_HARNESS_CONFIG_FILE}", None
        )
        if twister_harness_config_file is None:
            raise ValueError(
                "TWISTER_HARNESS_CONFIG_FILE variable must be set to file with pickle"
            )

        with open(twister_harness_config_file, "rb") as twister_harness_file:
            return pickle.load(twister_harness_file)

    def get_a_device(self):
        device_object = DeviceFactory.get_device_object(self.twister_harness_config)
        self.device = device_object
        return device_object

    def run_device(self):
        if self.device is None:
            raise ValueError("Currently no device")

        self.device.launch()

    def run_command(self, command: str) -> list[str]:
        if self.device is None:
            raise ValueError("Currently no device")

        if self.shell is None:
            self.shell = Shell(self.device)
            assert self.shell.wait_for_prompt()

        return self.shell.exec_command(command)

    def close_device(self):
        if self.device is None:
            raise ValueError("Currently no device")

        self.device.close()
        self.device = None
