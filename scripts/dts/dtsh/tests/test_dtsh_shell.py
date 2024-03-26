# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.shell module."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-function-docstring
# pylint: disable=missing-class-docstring
# pylint: disable=too-many-statements
# pylint: disable=protected-access


import pytest

from dtsh.model import DTNode, DTNodeCriterion, DTNodeCriteria
from dtsh.shell import (
    DTSh,
    DTShOption,
    DTShArg,
    DTShCommand,
    DTShFlag,
    DTShFlagHelp,
    DTShParameter,
    DTShError,
    DTShUsageError,
    DTPathNotFoundError,
    DTShCommandNotFoundError,
)

from .dtsh_uthelpers import DTShTests


def test_dtshoption() -> None:
    class MockOption(DTShOption):
        BRIEF = "mock option"
        SHORTNAME = "m"
        LONGNAME = "mock"

    opt = MockOption()
    assert "m" == opt.shortname
    assert "mock" == opt.longname
    assert "mock option" == opt.brief

    # No suffix expected.
    assert "m" == opt.getopt_short
    assert "mock" == opt.getopt_long
    assert "-m --mock" == opt.usage

    assert not opt.isset

    class MockOptionOther(DTShOption):
        SHORTNAME = "m"
        LONGNAME = "mock"

    # Same option names.
    assert opt == MockOptionOther()

    # Different option names.
    MockOptionOther.SHORTNAME = "x"
    assert opt != MockOptionOther()


def test_dtshflag() -> None:
    class MockFlag(DTShFlag):
        BRIEF = "mock flag"
        SHORTNAME = "m"
        LONGNAME = "mock"

    flag = MockFlag()
    assert "m" == flag.shortname
    assert "mock" == flag.longname
    assert "mock flag" == flag.brief

    # No suffix expected.
    assert "m" == flag.getopt_short
    assert "mock" == flag.getopt_long
    assert "-m --mock" == flag.usage

    # State life-cycle.
    assert not flag.isset
    flag.parsed()
    assert flag.isset
    flag.reset()
    assert not flag.isset

    with pytest.raises(ValueError):
        # Flags do not expect values.
        flag.parsed("value")

    class MockFlagOther(DTShFlag):
        SHORTNAME = "m"
        LONGNAME = "mock"

    # Same option names.
    assert flag == MockFlagOther()

    # Different option names.
    MockFlagOther.SHORTNAME = "x"
    assert flag != MockFlagOther()


def test_dtsharg() -> None:
    class MockArg(DTShArg):
        BRIEF = "mock argument"
        SHORTNAME = "m"
        LONGNAME = "mock"

        def __init__(self) -> None:
            super().__init__(argname="arg")

    arg = MockArg()
    assert "m" == arg.shortname
    assert "mock" == arg.longname
    assert "mock argument" == arg.brief

    # Suffix expected.
    assert "m:" == arg.getopt_short
    assert "mock=" == arg.getopt_long
    assert "-m --mock ARG" == arg.usage

    # State life-cycle.
    assert not arg.isset
    assert arg.raw is None
    arg.parsed("value")
    assert arg.isset
    assert "value" == arg.raw
    arg.reset()
    assert not arg.isset
    assert arg.raw is None

    with pytest.raises(ValueError):
        # Arguments expect values.
        arg.parsed()

    class MockArgOther(DTShArg):
        SHORTNAME = "m"
        LONGNAME = "mock"

        def __init__(self) -> None:
            super().__init__(argname="arg")

    # Same option names.
    assert arg == MockArgOther()

    # Different option names.
    MockArgOther.SHORTNAME = "x"
    assert arg != MockArgOther()


def test_dtshparameter() -> None:
    # Optional parameter.
    param = DTShParameter("param", "?", "mock param")
    assert "mock param" == param.brief
    assert "?" == param.multiplicity
    assert "[PARAM]" == param.usage
    DTShTests.check_param(param)

    # State life-cycle.
    param.reset()
    assert not param.raw
    param.parsed(["value"])
    assert ["value"] == param.raw
    param.reset()
    assert not param.raw
    # Parameter is optional.
    param.parsed([])
    assert not param.raw
    with pytest.raises(DTShError):
        # At most one value.
        param.parsed(["value", "value"])

    # Optional parameter, zero or more values
    param = DTShParameter("param", "*", "mock param")
    assert "*" == param.multiplicity
    assert "[PARAM ...]" == param.usage
    DTShTests.check_param(param)

    param.parsed([])
    assert not param.raw
    param.parsed(["value"])
    assert ["value"] == param.raw
    param.parsed(["value", "value"])
    assert ["value", "value"] == param.raw

    # Required parameter.
    param = DTShParameter("param", "+", "mock param")
    DTShTests.check_param(param)

    assert "+" == param.multiplicity
    assert "PARAM [PARAM ...]" == param.usage
    # Allows more than one value.
    param.parsed(["value"])
    assert ["value"] == param.raw
    param.parsed(["value", "value"])

    # Required parameter, N values.
    param = DTShParameter("param", 2, "mock param")
    DTShTests.check_param(param)

    assert 2 == param.multiplicity
    assert "PARAM PARAM" == param.usage
    # Expects exactly two values.
    param.parsed(["value", "value"])
    assert ["value", "value"] == param.raw
    with pytest.raises(DTShError):
        param.parsed([])
    with pytest.raises(DTShError):
        param.parsed(["value"])
    with pytest.raises(DTShError):
        param.parsed(["value", "value", "value"])


def test_dtshcommand() -> None:
    class MockFlag(DTShFlag):
        SHORTNAME = "f"
        LONGNAME = "flag"

    class MockArg(DTShArg):
        SHORTNAME = "a"
        LONGNAME = "argument"

        def __init__(self) -> None:
            super().__init__(argname="arg")

    class MockParam(DTShParameter):
        def __init__(self) -> None:
            super().__init__(name="param", multiplicity="?", brief="mock param")

    class MockCmd(DTShCommand):
        _param: MockParam

        def __init__(self) -> None:
            super().__init__(
                "mock", "mock command", [mock_flag, mock_arg], mock_param
            )

    mock_flag = MockFlag()
    mock_arg = MockArg()
    mock_param = MockParam()
    cmd = MockCmd()

    assert "mock" == cmd.name
    assert "mock command" == cmd.brief
    assert (
        "mock [-h --help] [-f --flag] [-a --argument ARG] [PARAM]"
        == cmd.synopsis
    )
    assert "hfa:" == cmd.getopt_short
    assert ["help", "flag", "argument="] == cmd.getopt_long

    assert [DTShFlagHelp(), mock_flag, mock_arg] == cmd.options
    assert cmd.param is mock_param

    assert mock_flag == cmd.option("-f") == cmd.option("--flag")
    assert mock_arg == cmd.option("-a") == cmd.option("--argument")
    assert cmd.option("not_an_option") is None

    assert mock_flag is cmd.with_option(MockFlag)
    # The flag is not set.
    assert not cmd.with_flag(MockFlag)

    assert mock_arg is cmd.with_arg(MockArg)
    assert mock_param is cmd.with_param(MockParam)

    class MockFlagInval(DTShFlag):
        pass

    with pytest.raises(KeyError):
        # Flag must exist.
        cmd.with_flag(MockFlagInval)

    class MockArgInval(DTShArg):
        pass

    with pytest.raises(KeyError):
        # Argument must exist.
        cmd.with_arg(MockArgInval)

    # Equality.
    assert DTShCommand("mock", "", [], None) == DTShCommand(
        "mock", "", [], None
    )
    assert DTShCommand("mock", "", [], None) != DTShCommand(
        "other", "", [], None
    )

    # Default order.
    assert DTShCommand("a", "", [], None) < DTShCommand("b", "", [], None)


def test_dtshcommand_parse_argv() -> None:
    class MockFlag(DTShFlag):
        SHORTNAME = "f"
        LONGNAME = "flag"

    class MockArg(DTShArg):
        SHORTNAME = "a"
        LONGNAME = "argument"

        def __init__(self) -> None:
            super().__init__(argname="arg")

    class MockParam(DTShParameter):
        def __init__(self) -> None:
            super().__init__(name="param", multiplicity="?", brief="mock param")

    cmd = DTShCommand(
        "mock", "mock command", [MockFlag(), MockArg()], MockParam()
    )
    assert not cmd.with_option(MockFlag).isset
    assert not cmd.with_flag(MockFlag)
    assert not cmd.with_flag(DTShFlagHelp)
    assert not cmd.with_arg(MockArg).isset
    assert cmd.with_arg(MockArg).raw is None
    assert [] == cmd.with_param(MockParam).raw

    cmd.parse_argv(["-f", "-a", "arg", "param"])
    assert cmd.with_flag(MockFlag)
    assert cmd.with_arg(MockArg).isset
    assert "arg" == cmd.with_arg(MockArg).raw
    assert ["param"] == cmd.with_param(MockParam).raw

    cmd.reset()
    assert not cmd.with_flag(MockFlag)
    assert not cmd.with_arg(MockArg).isset
    assert not cmd.with_param(MockParam).raw

    with pytest.raises(DTShUsageError):
        # Undefined option "-x".
        cmd.parse_argv(["-x"])

    cmd = DTShCommand("mock", "mock command", [], None)
    with pytest.raises(DTShUsageError):
        # Does not expect a parameter.
        cmd.parse_argv(["param"])

    with pytest.raises(DTShUsageError):
        # Requested help.
        cmd.parse_argv(["-h"])
        assert cmd.with_flag(DTShFlagHelp)


def test_dtsh_pathway() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    sh = DTSh(dtmodel, [])
    dt_soc = dtmodel["/soc"]
    dt_leds = dtmodel["/leds"]
    dt_flash0 = dtmodel["/soc/flash-controller@4001e000/flash@0"]
    dt_partitions = dt_flash0.get_child("partitions")

    # Working branch is "/":
    assert dt_soc.path == sh.pathway(dt_soc, dt_soc.path)
    assert dt_soc.name == sh.pathway(dt_soc, dt_soc.name)
    assert "soc" == sh.pathway(dt_soc, "")
    assert "../soc" == sh.pathway(dt_soc, "..")
    assert "../leds" == sh.pathway(dt_leds, "..")

    # Change working branch to "/soc":
    sh.cd(dt_soc.path)
    assert dt_soc.path == sh.pathway(dt_soc, dt_soc.path)
    assert "../soc" == sh.pathway(dt_soc, "..")
    assert "../leds" == sh.pathway(dt_leds, "..")
    # Nonsense: pathway from "/soc" to "/soc" with a empty path
    # representing "/soc" (e.g. "ls" would list the "/soc" children not soc).
    assert "" == sh.pathway(dt_soc, "")

    # Change working branch to "/soc/flash-controller@4001e000/flash@0".
    sh.cd(dt_flash0.path)
    assert dt_partitions.path == sh.pathway(dt_partitions, dt_flash0.path)
    assert dt_partitions.name == sh.pathway(dt_partitions, dt_partitions.name)
    assert "partitions" == sh.pathway(dt_partitions, "")
    assert "../flash@0/partitions" == sh.pathway(dt_partitions, "..")
    assert "../flash@0/partitions" == sh.pathway(dt_partitions, "../flash@0")

    with pytest.raises(DTPathNotFoundError):
        sh.pathway(dt_partitions, "/invalid/prefix")


def test_dtsh_path_expansion() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    dt_leds = dtmodel["/leds"]
    dt_pwmleds = dtmodel["/pwmleds"]
    sh = DTSh(dtmodel, [])

    assert DTSh.PathExpansion("", [dt_leds, dt_pwmleds]) == sh.path_expansion(
        "*ed*"
    )
    assert DTSh.PathExpansion("leds", dt_leds.children) == sh.path_expansion(
        "leds/*"
    )

    sh.cd(dt_leds.path)
    assert DTSh.PathExpansion("", dt_leds.children) == sh.path_expansion("led*")

    assert DTSh.PathExpansion("../leds", dt_leds.children) == sh.path_expansion(
        "../leds/led*"
    )
    # Empty expansions are errors.
    with pytest.raises(DTPathNotFoundError):
        sh.path_expansion("pwmled*")

    # Error when failed to resolve the expansion's prefix.
    with pytest.raises(DTPathNotFoundError):
        sh.path_expansion("/not/a/node")


def test_dtsh_realpath() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    dt_flash0 = dtmodel["/soc/flash-controller@4001e000/flash@0"]
    dt_partitions = dt_flash0.get_child("partitions")
    dt_partition0 = dt_partitions.get_child("partition@0")
    sh = DTSh(dtmodel, [])

    assert "/" == sh.realpath("")
    assert "/soc" == sh.realpath("soc")

    sh.cd("soc")
    assert "/soc" == sh.realpath("")
    assert "/soc" == sh.realpath(".")
    assert "/" == sh.realpath("..")

    assert dt_flash0.path == sh.realpath("&flash0")
    assert dt_partitions.path == sh.realpath("&flash0/partitions")

    sh.cd(dt_partitions.path)
    assert dt_partition0.path == sh.realpath("../partitions/partition@0")

    sh.cd()
    # Won't fault if realpath is evenutally not a path to a devicetree node.
    assert "/node/not/found" == sh.realpath("node/not/found")

    # But will fault when a DT label resolution fails.
    with pytest.raises(DTPathNotFoundError):
        sh.realpath("&label_not_found")


def test_dtsh() -> None:
    cmd_ls = DTShCommand("ls", "", [], None)
    cmd_tree = DTShCommand("tree", "", [], None)

    dtmodel = DTShTests.get_sample_dtmodel()
    sh = DTSh(dtmodel, [cmd_ls, cmd_tree])

    assert dtmodel is sh.dt
    assert dtmodel.root is sh.cwd
    assert "/" == sh.pwd
    assert [cmd_ls, cmd_tree] == sorted(sh.commands)
    assert cmd_ls is sh._commands["ls"]
    assert cmd_tree is sh._commands["tree"]


def test_dtsh_normpath() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    sh = DTSh(dtmodel, [])

    # Default to current working branch.
    assert sh.pwd == sh.realpath("")
    sh.cd("/soc")
    assert "/soc" == sh.pwd == sh.realpath("")
    sh.cd()
    assert "/" == sh.pwd == sh.realpath("")

    # Won't fail on undefined node names.
    assert "/" == sh.realpath("./a/../")
    assert "/a/b" == sh.realpath("a/b/")

    # Resolve devicetree label.
    assert "/buttons/button_0/b" == sh.realpath("&button0/b")

    # Fault on undefined label.
    with pytest.raises(DTPathNotFoundError) as e:
        sh.realpath("&not_a_label/b")
    assert "&not_a_label" == e.value.path


def test_dtsh_node_at() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    sh = DTSh(dtmodel, [])

    # Default to current working branch.
    assert sh.cwd is sh.node_at("")
    sh.cd("/soc")
    assert dtmodel["/soc"] is sh.cwd is sh.node_at("")
    sh.cd()
    assert dtmodel.root is sh.cwd is sh.node_at("")

    # Resolve path references.
    assert dtmodel.root is sh.node_at("./soc/../")
    assert dtmodel["/soc"] == sh.node_at("soc/uicr@10001000/.././")

    # Fault on undefined node names.
    with pytest.raises(DTPathNotFoundError) as e:
        sh.node_at("not/a/node")
    assert "/not/a/node" == e.value.path

    # Resolve devicetree label.
    assert dtmodel["/buttons/button_0"] is sh.node_at("&button0")

    # Fault on undefined label.
    with pytest.raises(DTPathNotFoundError) as e:
        sh.node_at("&not_a_label")
    assert "&not_a_label" == e.value.path


def test_dtsh_cd() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    sh = DTSh(dtmodel, [])
    assert dtmodel.root is sh.cwd
    assert "/" == sh.pwd

    # Absolute path.
    sh.cd("/soc/i2c@40003000")
    assert dtmodel["/soc/i2c@40003000"] is sh.cwd
    assert "/soc/i2c@40003000" == sh.pwd

    # Path references.
    sh.cd()
    assert dtmodel.root is sh.cwd
    sh.cd("/./soc/i2c@40003000/..")
    assert dtmodel["/soc"] is sh.cwd

    # Relative path.
    sh.cd()
    sh.cd("soc")
    assert dtmodel["/soc"] is sh.cwd

    # Devicetree labels.
    sh.cd("&i2c0")
    assert dtmodel["/soc/i2c@40003000"] is sh.cwd

    sh.cd()
    with pytest.raises(DTPathNotFoundError) as e:
        sh.cd("not/a/node")
    assert "/not/a/node" == e.value.path
    with pytest.raises(DTPathNotFoundError) as e:
        sh.cd("&not_a_label/path")
    assert "&not_a_label" == e.value.path


def test_dtsh_find() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    dt_partitions = dtmodel["/soc/flash-controller@4001e000/flash@0/partitions"]
    dt_i2c = dtmodel["/soc/i2c@40003000"]
    sh = DTSh(dtmodel, [])

    # An Empty criterion chain matches all nodes: POSIX-like "find".
    assert sorted(dtmodel.root.walk()) == sorted(
        sh.find("", DTNodeCriteria([]))
    )

    class CriterionBME680(DTNodeCriterion):
        def match(self, node: DTNode) -> bool:
            return "bme680" == node.unit_name

    # Relative path with wild-cards.
    assert [dt_i2c.get_child("bme680@76")] == sh.find(
        dt_i2c.path, CriterionBME680()
    )

    # Devicetree labels.
    assert [dt_i2c.get_child("bme680@76")] == sh.find(
        "&i2c0", CriterionBME680()
    )

    class CriterionPartition(DTNodeCriterion):
        def match(self, node: DTNode) -> bool:
            return "partition" == node.unit_name

    criterion = CriterionPartition()
    assert sorted(dt_partitions.children) == sorted(sh.find("/soc", criterion))
    assert [] == sh.find(dt_i2c.path, criterion)

    # ORed criterion chain.
    criteria = DTNodeCriteria(
        [CriterionPartition(), CriterionBME680()], ored_chain=True
    )
    assert sorted(
        [
            dt_i2c.get_child("bme680@76"),
            dtmodel["/soc/spi@40004000/bme680@0"],
            *dt_partitions.children,
        ]
    ) == sorted(sh.find("/soc", criteria))

    # Negate criterion chain.
    criteria = DTNodeCriteria([CriterionPartition()], negative_chain=True)
    # The unit name "partitions" does not equal to "paratition".
    assert [dt_partitions] == sh.find(dt_partitions.path, criteria)


def test_dtsh_parse_cmdline() -> None:
    class MockFlag(DTShFlag):
        SHORTNAME: str = "f"
        LONGNAME: str = "flag"

    class MockArg(DTShArg):
        SHORTNAME = "a"
        LONGNAME = "argument"

        def __init__(self) -> None:
            super().__init__(argname="arg")

    class MockParam(DTShParameter):
        def __init__(self) -> None:
            super().__init__(name="param", multiplicity=0, brief="mock param")

    class MockCommand(DTShCommand):
        def __init__(self) -> None:
            super().__init__("mock", "", [MockFlag(), MockArg()], MockParam())

    dtmodel = DTShTests.get_sample_dtmodel()
    cmd_mock = MockCommand()
    sh = DTSh(dtmodel, [cmd_mock])

    # Won't parse an empty command line.
    with pytest.raises(ValueError):
        sh.parse_cmdline("")

    # First lex is always parsed into a command name.
    with pytest.raises(DTShCommandNotFoundError) as e:
        sh.parse_cmdline(">")
        assert ">" == e.value.name
    with pytest.raises(DTShCommandNotFoundError) as e:
        sh.parse_cmdline(" > ")
        assert ">" == e.value.name
    with pytest.raises(DTShCommandNotFoundError) as e:
        sh.parse_cmdline(" >> ")
        assert ">>" == e.value.name
    with pytest.raises(DTShCommandNotFoundError) as e:
        sh.parse_cmdline(">x")
        assert ">x" == e.value.name
    with pytest.raises(DTShCommandNotFoundError) as e:
        sh.parse_cmdline("> x")
        assert ">" == e.value.name
    # No space before ">", no  redirection.
    with pytest.raises(DTShCommandNotFoundError) as e:
        sh.parse_cmdline("mock>x")
        assert "mock>x" == e.value.name
    with pytest.raises(DTShCommandNotFoundError) as e:
        sh.parse_cmdline("mock>> x")
        assert "mock>>" == e.value.name

    # Command line without redirection.
    cmd, cmd_argv, redir2 = sh.parse_cmdline("  mock  ")
    assert cmd_mock == cmd
    assert [] == cmd_argv
    assert redir2 is None

    # Command line with empty redirection path.
    cmd, cmd_argv, redir2 = sh.parse_cmdline("mock >")
    assert cmd_mock == cmd
    assert [] == cmd_argv
    assert ">" == redir2

    cmd, cmd_argv, redir2 = sh.parse_cmdline("mock >> ")
    assert cmd_mock == cmd
    assert [] == cmd_argv
    assert ">>" == redir2

    # Not a redirection, expects a value.
    cmd, cmd_argv, redir2 = sh.parse_cmdline("mock --argument >")
    assert cmd_mock == cmd
    assert ["--argument", ">"] == cmd_argv
    assert redir2 is None
    # Value provided, parsed redirection.
    cmd, cmd_argv, redir2 = sh.parse_cmdline("mock --argument >1 >path")
    assert cmd_mock == cmd
    assert ["--argument", ">1"] == cmd_argv
    assert ">path" == redir2
    # The flag does not expect a value, parsed redirection.
    cmd, cmd_argv, redir2 = sh.parse_cmdline("mock --flag >> path")
    assert cmd_mock == cmd
    assert ["--flag"] == cmd_argv
    assert ">> path" == redir2
