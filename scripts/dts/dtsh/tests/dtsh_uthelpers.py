# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Helpers for DTSh unit tests."""

from typing import Optional, Dict, List, Generator

from pathlib import Path
import os
import contextlib

import pytest

from devicetree import edtlib

from dtsh.model import DTModel
from dtsh.io import DTShOutput
from dtsh.shell import (
    DTSh,
    DTShCommand,
    DTShOption,
    DTShFlag,
    DTShArg,
    DTShParameter,
    DTShFlagHelp,
    DTShError,
    DTShUsageError,
    DTShCommandError,
)
from dtsh.shellutils import (
    DTShArgFixedDepth,
    DTShArgOrderBy,
    DTShParamDTPath,
    DTShParamDTPaths,
    DTShParamAlias,
    DTShParamChosen,
    DTSH_NODE_ORDER_BY,
)
from dtsh.rich.shellutils import DTShArgLongFmt, DTSH_NODE_FMT_SPEC


class DTShTests:
    """Access test resource files, data samples, etc."""

    ZEPHYR_BASE = os.environ.get("ZEPHYR_BASE") or str(
        Path(__file__).parent.parent.parent.parent.parent
    )
    """A ZEPHYR_BASE test runners may explicitly set."""

    RES_BASE = os.path.join(Path(__file__).parent.parent, "tests", "res")
    """Unit tests resource files directory."""

    ANON_ZEPHYR_BASE = "/path/to/zephyr_base"
    """Anonymized ZEPHYR_BASE that appears in CMake cache files."""

    ANON_ZEPHYR_SDK = "/path/to/zephyr-sdk"
    """Anonymized Zephyr SDK path that appears in CMake cache files."""

    ANON_GNUARMEMB = "/path/to/gnuarmemb"
    """Anonymized Arm toolchain that path appears in CMake cache files."""

    ZEPHYR_VERSION = "3.4.99"
    """Zephyr project version that appears in CMake cache files."""

    BOARD = "nrf52840dk_nrf52840"
    """The unit tests board model."""

    _sample_edt: Optional[edtlib.EDT] = None
    _sample_dtmodel: Optional[DTModel] = None

    @classmethod
    @contextlib.contextmanager
    def mock_env(
        cls, tmpenv: Dict[str, Optional[str]]
    ) -> Generator[None, None, None]:
        """Temporarily change the OS environment.

           None values will unset the variables.

            with mock_env({"ZEPHYR_BASE": None, "NAME": "VALUE"}):
                # Unit test for API behaviors that depend on environment variables.

        Args:
            tmpenv: The OS environment changes.
        """
        # Save the environment subset the mock will change or unset the values of.
        diffenv = {name: os.environ.get(name) for name in tmpenv}
        for name, value in tmpenv.items():
            if value is not None:
                # Change or add environment values.
                os.environ[name] = value
            elif name in os.environ:
                # Unset defined environment variables.
                del os.environ[name]

        try:
            # Work with temp OS environment.
            yield

        finally:
            for name, value in diffenv.items():
                if value is not None:
                    # Restore the environment subset the mock had changed
                    # the values of.
                    os.environ[name] = value
                elif name in os.environ:
                    # Unset the environment subset the mock added.
                    del os.environ[name]

    @classmethod
    @contextlib.contextmanager
    def from_res(cls) -> Generator[None, None, None]:
        """Temporarily change the working directory to the resources location.

        Implementation idea borrowed from the "from_here()"  pattern
        in Zephyr's python-devicetree (test_edtlib.py).
        """
        cwd = os.getcwd()
        try:
            os.chdir(cls.RES_BASE)
            yield
        finally:
            os.chdir(cwd)

    @classmethod
    @contextlib.contextmanager
    def from_there(cls, *name: str) -> Generator[None, None, None]:
        """Temporarily change the working directory."""
        cwd = os.getcwd()
        try:
            os.chdir(os.path.join(cls.RES_BASE, *name))
            yield
        finally:
            os.chdir(cwd)

    @classmethod
    def get_resource_path(cls, *name: str) -> str:
        """Translate path name components to an absolute resource file path.

        Args:
            *name: Path components.

        Returns:
            The absolute path to the resource file.
        """
        return os.path.join(cls.RES_BASE, *name)

    @classmethod
    def get_sample_edt(cls, force_reload: bool = False) -> edtlib.EDT:
        """Get the sample Devicetree model (EDT).

        Args:
            force_reload: Force reloading the model (default is to return
              a cached model).

        Returns:
            The sample Devicetree model (EDT).
        """
        if force_reload:
            return cls._read_sample_edt()
        if not cls._sample_edt:
            cls._sample_edt = cls._read_sample_edt()
        return cls._sample_edt

    @classmethod
    def get_sample_dtmodel(cls, force_reload: bool = False) -> DTModel:
        """Get the sample Devicetree model (DTModel).

        Args:
            force_reload: Force reloading the model (default is to return
              a cached model).

        Returns:
            The sample Devicetree model (DTModel).
        """
        if force_reload:
            return cls._read_sample_dtmodel()
        if not cls._sample_dtmodel:
            cls._sample_dtmodel = cls._read_sample_dtmodel()
        return cls._sample_dtmodel

    @classmethod
    def check_option_meta(cls, opt: DTShOption) -> None:
        """Check meta-data all options must provide.

        Args:
            opt: The shell option under test.
        """
        assert opt.shortname or opt.longname
        if opt.shortname:
            assert 1 == len(opt.shortname)
        if opt.longname:
            assert 1 < len(opt.longname)
        assert opt.brief

    @classmethod
    def check_flag(cls, flag: DTShFlag) -> None:
        """Check flag meta-data and state life-cycle.

        The flag is cleared on return.

        Args:
            flag: The shell flag under test.
        """
        cls.check_option_meta(flag)

        flag.reset()
        assert not flag.isset
        flag.parsed()
        assert flag.isset
        flag.reset()

    @classmethod
    def check_arg(cls, arg: DTShArg, parsed: str) -> None:
        """Check argument meta-data and state life-cycle.

        The argument value parsed is retained on return.

        Args:
            arg: The shell argument under test.
            parsed: A valid argument to parse.
        """
        cls.check_option_meta(arg)

        arg.reset()
        assert not arg.isset
        assert arg.raw is None

        arg.parsed(parsed)
        assert arg.isset
        assert parsed == arg.raw

    @classmethod
    def get_param_placeholder(cls, param: DTShParameter) -> List[str]:
        """Get a parameter placeholder with valid multiplicity."""
        if param.multiplicity in ["?", "*"]:
            return []
        if param.multiplicity == "+":
            return ["value"]
        if isinstance(param.multiplicity, int):
            n: int = param.multiplicity
            return n * ["value"]
        raise ValueError(param.multiplicity)

    @classmethod
    def get_param_placeholder_inval(cls, multiplicity: int) -> List[str]:
        """Get a parameter placeholder with invalid multiplicity."""
        n_inval: int = multiplicity - 1
        return n_inval * ["param"]

    @classmethod
    def check_param(cls, param: DTShParameter) -> None:
        """Check parameter's life-cycle and multiplicity.

        The parameter is reset on return.

        Args:
            The parameter to test.
        """
        assert param.multiplicity in ["*", "?", "+"] or isinstance(
            param.multiplicity, int
        )
        if isinstance(param.multiplicity, int):
            assert param.multiplicity > 0

        assert [] == param.raw
        values = cls.get_param_placeholder(param)
        param.parsed(values)
        assert values == param.raw
        param.reset()
        assert [] == param.raw

        if param.multiplicity == "?":
            # Allows zero or one value.
            param.parsed([])
            assert [] == param.raw
            param.parsed(["value"])
            assert ["value"] == param.raw
            # Fails with more than one value.
            with pytest.raises(DTShError):
                param.parsed(["param", "param"])

        elif param.multiplicity == "+":
            # Fails without value.
            with pytest.raises(DTShError):
                param.parsed([])
            # Allows more than one value.
            param.parsed(["value"])
            assert ["value"] == param.raw
            param.parsed(["value", "value"])
            assert ["value", "value"] == param.raw

        elif isinstance(param.multiplicity, int):
            # Fails when the number of values does not match multiplicity.
            with pytest.raises(DTShError):
                param.parsed(
                    cls.get_param_placeholder_inval(param.multiplicity)
                )

        param.reset()

    @classmethod
    def check_cmd_meta(cls, cmd: DTShCommand, name: str) -> None:
        """Check meta-data all options must provide.

        Args:
            opt: The shell option under test.
        """
        assert name == cmd.name
        assert cmd.brief

        assert DTShCommand(name, "", [], None) == cmd
        assert DTShCommand("other", "", [], None) != cmd

    @classmethod
    def check_cmd_flags(
        cls, cmd: DTShCommand, sh: DTSh, out: DTShOutput
    ) -> None:
        """Check flags state upon command execution.

        On return, all the command's flags are set.

        Args:
            cmd: The command under test.
            sh: The shell that will execute the command.
            out: An output stream.
        """
        # Retrieve all command flags.
        flags: List[DTShFlag] = [
            opt for opt in cmd.options if isinstance(opt, DTShFlag)
        ]

        # Assert all flags are initially unset.
        cmd.reset()
        for flag in flags:
            assert not flag.isset
            assert not cmd.with_flag(flag.__class__)

        # Parse command arguments that set all flags under test.
        argv: List[str] = [
            f"-{flag.shortname}" if flag.shortname else f"--{flag.longname}"
            for flag in flags
            # Would raise DTShUsageError to trigger command help.
            if not isinstance(flag, DTShFlagHelp)
        ]
        if cmd.param:
            argv += cls.get_param_placeholder(cmd.param)

        try:
            cmd.execute(argv, sh, out)
        except DTShCommandError as e:
            # We won't craft valid parameter values,
            # we just want to parse flags.
            assert not isinstance(e, DTShUsageError)

        # Assert all flags are now set.
        for flag in flags:
            if not isinstance(flag, DTShFlagHelp):
                assert flag.isset
                assert cmd.with_flag(flag.__class__)

    @classmethod
    def check_cmd_arg_order_by(
        cls, cmd: DTShCommand, sh: DTSh, out: DTShOutput
    ) -> None:
        """Check "--order-by KEY" argument upon command execution.

        Test:
        - the expected sorter argument for all valid keys
        - expect DTShUsageError for invalid keys

        Args:
            cmd: The command under test.
            sh: The context shell.
            out: An output stream.
        """
        arg = cmd.with_arg(DTShArgOrderBy)

        for key in DTSH_NODE_ORDER_BY:
            cmd.execute(["--order-by", key], sh, out)
            assert DTSH_NODE_ORDER_BY[key].sorter is arg.sorter
            assert (
                DTSH_NODE_ORDER_BY[key].sorter
                is cmd.with_arg(DTShArgOrderBy).sorter
            )

        with pytest.raises(DTShUsageError):
            cmd.execute(["--order-by", "not a key"], sh, out)

    @classmethod
    def check_cmd_arg_fixed_depth(
        cls, cmd: DTShCommand, sh: DTSh, out: DTShOutput
    ) -> None:
        """Check "--fixed-depth DEPTH" argument upon command execution.

        Test:
        - the expected depth for valid argument values
        - expect DTShUsageError for invalid argument values

        Args:
            cmd: The command under test.
            sh: The context shell.
            out: An output stream.
        """
        arg = cmd.with_arg(DTShArgFixedDepth)

        cmd.execute(["--fixed-depth", "2"], sh, out)
        assert arg.isset
        assert 2 == arg.depth
        assert 2 == cmd.with_arg(DTShArgFixedDepth).depth

        with pytest.raises(DTShUsageError):
            cmd.execute(["--fixed-depth", "not an int"], sh, out)
        with pytest.raises(DTShUsageError):
            # Negative values not allowed.
            cmd.execute(["--fixed-depth", "-2"], sh, out)

    @classmethod
    def check_cmd_arg_longfmt(
        cls, cmd: DTShCommand, sh: DTSh, out: DTShOutput
    ) -> None:
        """Check "--format FMT" argument when executing the command.

        Test:
        - the expected fields argument for all valid specifiers
        - expect DTShUsageError for invalid format strings

        Args:
            cmd: The command under test.
            sh: The shell that will execute the command.
            out: An output stream.
        """
        arg = cmd.with_arg(DTShArgLongFmt)

        for key, spec in DTSH_NODE_FMT_SPEC.items():
            cmd.execute(["--format", key], sh, out)
            assert arg.fmt
            assert spec.col is arg.fmt[0]

        all_fmt = "".join(spec for spec in DTSH_NODE_FMT_SPEC)
        cmd.execute(["--format", all_fmt], sh, out)
        assert [DTSH_NODE_FMT_SPEC[spec].col for spec in all_fmt] == arg.fmt

        with pytest.raises(DTShCommandError):
            cmd.execute(["--format", "invalid format string"], sh, out)

    @classmethod
    def check_cmd_param_dtpath(
        cls, cmd: DTShCommand, sh: DTSh, out: DTShOutput
    ) -> None:
        """Check the PATH parameter upon command execution.

        Will test:
        - parameter's state and multiplicity
        - invalid parameter values (i.e. invalid paths)

        The metadata and parameter specific properties of DTShParamDTPath
        are tested in test_dtsh_shellutils.py.

        Args:
            cmd: The command under test, that does not support path expansion.
            sh: The context shell.
            out: An output stream.
        """
        param = cmd.with_param(DTShParamDTPath)

        # Parameter's state and multiplicity.
        cls.check_param(param)

        with pytest.raises(DTShCommandError):
            # Path not found.
            cmd.execute(["/not/a/node"], sh, out)
        with pytest.raises(DTShCommandError):
            # No path expansion: despite "/leds" exists, this should fail.
            cmd.execute(["/leds*"], sh, out)

    @classmethod
    def check_cmd_param_dtpaths(
        cls, cmd: DTShCommand, sh: DTSh, out: DTShOutput
    ) -> None:
        """Check the PATHs parameter upon command execution.

        Will test:
        - parameter's state and multiplicity
        - invalid parameter values (i.e. invalid paths)

        The metadata and parameter specific properties of DTShParamDTPaths
        are tested in test_dtsh_shellutils.py.

        Args:
            cmd: The command under test, that does support path expansion.
            sh: The context shell.
            out: An output stream.
        """
        param = cmd.with_param(DTShParamDTPaths)

        # Parameter's state and multiplicity.
        cls.check_param(param)

        with pytest.raises(DTShCommandError):
            # Path not found.
            cmd.execute(["/not/a/node"], sh, out)
        with pytest.raises(DTShCommandError):
            # Empty expansions are errors.
            cmd.execute(["/soc/empty*"], sh, out)

    @classmethod
    def check_cmd_param_alias(
        cls, cmd: DTShCommand, sh: DTSh, out: DTShOutput
    ) -> None:
        """Check the ALIAS parameter upon command execution.

        Will test:
        - parameter's state and multiplicity
        - invalid parameter values (i.e. invalid alias names)

        The metadata and parameter specific properties of DTShParamAlias
        are tested in test_dtsh_shellutils.py.

        Args:
            cmd: The command under test, that does support path expansion.
            sh: The context shell.
            out: An output stream.
        """
        param = cmd.with_param(DTShParamAlias)

        # Parameter's state and multiplicity.
        cls.check_param(param)

        alias = "led0"
        cmd.execute([alias], sh, out)
        assert alias == param.alias
        assert alias == cmd.with_param(DTShParamAlias).alias

        # Should not fail: the parameter is interpreted as a search string.
        cmd.execute(["not-an-alias"], sh, out)

    @classmethod
    def check_cmd_param_chosen(
        cls, cmd: DTShCommand, sh: DTSh, out: DTShOutput
    ) -> None:
        """Check the CHOSEN parameter upon command execution.

        Will test:
        - parameter's state and multiplicity
        - invalid parameter values (i.e. invalid chosen names)

        The metadata and parameter specific properties of DTShParamChosen
        are tested in test_dtsh_shellutils.py.

        Args:
            cmd: The command under test, that does support path expansion.
            sh: The context shell.
            out: An output stream.
        """
        param = cmd.with_param(DTShParamChosen)

        # Parameter's state and multiplicity.
        cls.check_param(param)
        assert "" == param.chosen

        # Must be a valid choice.
        chosen = "zephyr,entropy"
        cmd.execute([chosen], sh, out)
        assert chosen == param.chosen
        assert chosen == cmd.with_param(DTShParamChosen).chosen

        # Should not fail: the parameter is interpreted as a search string.
        cmd.execute(["not-a-chosen"], sh, out)

    @classmethod
    def check_cmd_execute(
        cls, cmd: DTShCommand, sh: DTSh, out: DTShOutput
    ) -> None:
        """Check common exception cases when executing a command.

        Test:
        - triggering help
        - expect DTShUsageError if undefined option
        - expect DTShUsageError if invalid parameter multiplicity

        Args:
            cmd: The command under test.
            sh: The shell that will execute the command.
            out: An output stream.
        """
        # Trigger help.
        with pytest.raises(DTShUsageError):
            cmd.execute(["-h"], sh, out)
        assert cmd.with_flag(DTShFlagHelp)

        with pytest.raises(DTShUsageError):
            cmd.execute(["--not-an-option"], sh, out)

        if cmd.param:
            # When can test multiplicity with string parameters since these
            # should fail before the actual parameter type/value is invovled.
            multiplicity = cmd.param.multiplicity

            if multiplicity == "?":
                # 0 <= N <= 1
                with pytest.raises(DTShUsageError):
                    cmd.execute(["value", "value"], sh, out)

            elif multiplicity == "+":
                # N = 1
                with pytest.raises(DTShUsageError):
                    cmd.execute([], sh, out)
                with pytest.raises(DTShUsageError):
                    cmd.execute(["value", "value"], sh, out)

            elif isinstance(multiplicity, int):
                param_values = cls.get_param_placeholder_inval(multiplicity)
                with pytest.raises(DTShUsageError):
                    cmd.execute(param_values, sh, out)

    @classmethod
    def _read_sample_edt(cls) -> edtlib.EDT:
        with cls.from_res():
            return edtlib.EDT(
                dts="zephyr.dts",
                bindings_dirs=[
                    os.path.join(cls.ZEPHYR_BASE, "dts", "bindings")
                ],
            )

    @classmethod
    def _read_sample_dtmodel(cls) -> DTModel:
        with cls.from_res():
            return DTModel.create(
                dts_path="zephyr.dts",
                binding_dirs=[os.path.join(cls.ZEPHYR_BASE, "dts", "bindings")],
            )
