import logging
from strictyaml import load as strictyaml_load
from strictyaml import Map, MapPattern, Optional, Seq, Str, YAMLError

LOGGER_NAME = f"bsim_plugin.{__name__.split('.')[-1]}"
logger = logging.getLogger(LOGGER_NAME)


class TestsuiteYamlParser:
    bsim_config_schema = Map(
        {
            "devices": Seq(Map(
                {
                    "testid": Str(),
                    Optional("extra_run_args"): Seq(Str()),
                }
            )),
            "physical_layer": Map(
                {
                    "name": Str(),
                    "sim_length": Str(),
                    Optional("extra_run_args"): Seq(Str()),
                }
            ),
            Optional("built_exe_name"): Str(),
        }
    )

    testscenario_schema = Map(
        {
            Optional("bsim_config"): bsim_config_schema,
            Optional("extra_args"): Seq(Str()),
        }
    )

    tests_schema = MapPattern(
        Str(),  # testscenario name
        testscenario_schema,
        minimum_keys=1,  # tests require minimum one testscenario
    )

    testsuite_yaml_schema = Map(
        {
            Optional("common"): testscenario_schema,
            "tests": tests_schema,
        }
    )

    def __init__(self):
        pass

    def load(self, testsuite_yaml_path):
        with open(testsuite_yaml_path, "r") as file_handler:
            try:
                yaml_data = strictyaml_load(
                    file_handler.read(),
                    self.testsuite_yaml_schema
                )
            except YAMLError as err:
                if hasattr(err, "context"):
                    err.context = f"while parsing a file \n" \
                                  f"{testsuite_yaml_path}\n" + err.context
                raise err
            yaml_data = yaml_data.data
        return yaml_data
