import argparse
import pickle

import robot
from twister_harness.argument_handler import ArgumentHandler
from twister_harness.twister_harness_config import TwisterHarnessConfig

ROBOT_TWISTER_VAR_FILE = "robot_twister_var.pickle"


def _parse_arguments():
    parser = argparse.ArgumentParser()

    handler = ArgumentHandler(parser.add_argument)
    handler.add_common_arguments()

    options, rest_args = parser.parse_known_args()

    handler.sanitize_options(options)

    return options, rest_args


def _serialize_config(twister_harness_config: TwisterHarnessConfig):
    device = twister_harness_config.devices[0]

    filepath = device.build_dir / ROBOT_TWISTER_VAR_FILE

    with open(device.build_dir / ROBOT_TWISTER_VAR_FILE, "wb") as robot_twister_vars:
        pickle.dump(twister_harness_config, robot_twister_vars)

    return filepath


def main():
    options, rest_args = _parse_arguments()
    twister_harness_config = TwisterHarnessConfig.create(options)
    variables_filepath = _serialize_config(twister_harness_config)

    robot.run_cli(
        ["--variable", f"TWISTER_HARNESS_CONFIG_FILE:{variables_filepath}"] + rest_args
    )


if __name__ == "__main__":
    main()
