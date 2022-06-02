import os
import sys
import logging
import shutil
import pytest
import json
from test_utils.TestsuiteYamlParser import TestsuiteYamlParser
from test_utils.BabbleSimBuild import BabbleSimBuild
from test_utils.BabbleSimError import BabbleSimError

LOGGER_NAME = "bsim_plugin"
logger = logging.getLogger(LOGGER_NAME)

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
if not ZEPHYR_BASE:
    sys.exit("$ZEPHYR_BASE environment variable undefined")

BSIM_TESTS_OUT_DIR_NAME = "bsim_tests_out"
BSIM_TESTS_OUT_DIR_PATH = os.path.join(ZEPHYR_BASE, BSIM_TESTS_OUT_DIR_NAME)
BUILD_INFO_FILE_NAME = "build_info.json"
BUILD_INFO_FILE_PATH = \
    os.path.join(BSIM_TESTS_OUT_DIR_PATH, BUILD_INFO_FILE_NAME)


def prepare_bsim_test_out_dir():
    """
    Move previous (if exists) bsim_tests_out dir into historical one (like for
    example bsim_tests_out_2). Create new clean output dir, where all tests
    output will be stored (logs, build output, reports).
    """
    # TODO: refactor this to simplify out dir preparation and write "smarter"
    #  detection of historical directories
    if os.path.exists(BSIM_TESTS_OUT_DIR_PATH):
        max_dir_number = 100
        for idx in range(1, max_dir_number):
            new_dir_path = f"{BSIM_TESTS_OUT_DIR_PATH}_{idx}"
            if not os.path.exists(new_dir_path):
                # Cannot use logger yet, so just use print
                print(f"Copy output directories: "
                      f"\nsrc: {BSIM_TESTS_OUT_DIR_PATH} "
                      f"\ndst: {new_dir_path}")
                shutil.move(BSIM_TESTS_OUT_DIR_PATH, new_dir_path)
                break
        else:
            err_msg = f'Too many historical directories. Remove some ' \
                      f'"{BSIM_TESTS_OUT_DIR_NAME}" folders.'
            logger.error(err_msg)
            sys.exit(err_msg)
    os.mkdir(BSIM_TESTS_OUT_DIR_PATH)


def setup_logger(config):
    """
    If tests are run in parallel (with pytest's xdist plugin), then create
    several output log file (for each run worker/process).
    """
    if is_main_worker(config):
        log_file_name = "bsim_test_plugin.log"
    else:
        worker_id = config.workerinput['workerid']
        log_file_name = f'bsim_test_plugin_{worker_id}.log'
    log_file_path = os.path.join(BSIM_TESTS_OUT_DIR_PATH, log_file_name)
    file_handler = logging.FileHandler(log_file_path, mode="w")
    formatter_file = logging.Formatter(
        '%(asctime)s - %(levelname)s - %(message)s',
        datefmt="%H:%M:%S"
    )
    file_handler.setFormatter(formatter_file)
    logger.addHandler(file_handler)
    logger.setLevel(logging.DEBUG)


def pytest_configure(config):
    if is_main_worker(config):
        prepare_bsim_test_out_dir()
        with open(BUILD_INFO_FILE_PATH, 'w', encoding='utf-8') as file:
            json.dump({}, file, indent=4)
    setup_logger(config)


def pytest_unconfigure(config):
    if is_main_worker(config):
        build_info_lock_path = f"{BUILD_INFO_FILE_PATH}.lock"
        if os.path.exists(build_info_lock_path) and \
                os.path.isfile(build_info_lock_path):
            os.remove(build_info_lock_path)


def is_main_worker(config):
    """
    When tests are run in parallel (by xdist plugin), then several workers/
    processes are created. This function help to verify if current process is
    main worker/process, which is run at the very beginning of tests (before
    "children" workers start) and is finished when all tests are performed
    (after "children" workers will end). When tests are run sequentially (not by
    xdist plugin), then only one process are run, which is "main worker".
    """
    is_worker_input = hasattr(config, 'workerinput')
    return not is_worker_input


def pytest_collect_file(parent, file_path):
    """
    Analyse only bs_testsuite.yaml or bs_testsuite.yml files.
    """
    if file_path.name.startswith("bs_testsuite") and \
            (file_path.suffix in (".yaml", ".yml")):
        return YamlFile.from_parent(parent, path=file_path)


def pytest_collection_finish(session):
    """
    Verify uniqueness of collected test scenario names.
    """
    duplication_flag = False
    test_scenario_names = [item.name for item in session.items]
    for test_scenario in session.items:
        if test_scenario_names.count(test_scenario.name) > 1:
            duplication_flag = True
            logger.error("Test scenario name is not unique: %s from:\n%s",
                         test_scenario.name, test_scenario.test_src_path)
    if duplication_flag:
        exception_msg = "Test scenario names duplication found."
        raise YamlException(exception_msg)


class YamlFile(pytest.File):
    def collect(self):
        test_src_path = self.path.parent

        parser = TestsuiteYamlParser()
        yaml_data = parser.load(self.path)
        common_config = yaml_data.get("common", {})
        testscenarios = yaml_data.get("tests", {})
        for testscenario_name, testscenario_config in testscenarios.items():
            yield YamlItem.from_parent(
                self,
                test_src_path=test_src_path,
                common_config=common_config,
                testscenario_name=testscenario_name,
                testscenario_config=testscenario_config
            )


class YamlItem(pytest.Item):
    def __init__(self, parent, test_src_path, testscenario_name, common_config,
                 testscenario_config):
        super().__init__(testscenario_name, parent)

        logger.debug("Found test scenario: %s", testscenario_name)

        self.test_src_path = test_src_path

        self.sim_id = testscenario_name.replace(".", "_")
        self.test_out_path = os.path.join(BSIM_TESTS_OUT_DIR_PATH, self.sim_id)

        self._update_testscenario_config(common_config, testscenario_config)

        self.extra_build_args = testscenario_config.get("extra_args", [])
        bsim_config = testscenario_config["bsim_config"]
        self.devices = bsim_config.get("devices", [])
        self.phy = bsim_config.get("physical_layer", {})
        default_general_exe_name = f"bs_nrf52_bsim_{self.sim_id}"
        self.general_exe_name = \
            bsim_config.get("built_exe_name", default_general_exe_name)

        self.build_info_file_path = BUILD_INFO_FILE_PATH

    @staticmethod
    def _update_testscenario_config(common_config, testscenario_config):
        """
        When "common" entry is used in bs_testsuite.yaml file, then update
        testscenario configs with following rules:
        1. If the same config occurs in "common" and testscenario entries and
        they are a list (like for example "extra_args") then join them together.
        2. If the same config occurs in "common" and testscenario entries and
        they are NOT a list (like for example "bsim_config"), then do NOT
        overwrite testscenario config by common config.
        3. If some config occurs in "common" and not occur testscenario entry,
        then add this config to testscenario configs.
        """
        if not common_config:
            return

        for config_name, config_value in common_config.items():
            if config_name in testscenario_config:
                if isinstance(testscenario_config[config_name], list) and \
                        isinstance(config_value, list):
                    # join testscenario and common configs if they are lists
                    testscenario_config[config_name] += config_value
                else:
                    # do not overwrite testscenario_config
                    pass
            else:
                testscenario_config[config_name] = config_value

    def runtest(self):
        logger.info("Start test: %s", self.name)

        os.makedirs(self.test_out_path)

        bs_builder = BabbleSimBuild(
            self.test_src_path,
            self.test_out_path,
            self.build_info_file_path,
            self.general_exe_name,
            self.extra_build_args
        )
        bs_builder.build()

    def repr_failure(self, excinfo, style=None):
        """Called when self.runtest() raises an exception."""
        if isinstance(excinfo.value, BabbleSimError):
            failure_msg = excinfo.value.args[0]
            return failure_msg


class YamlException(Exception):
    pass
