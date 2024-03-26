# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Devicetree shell.

Shell-like interface with a devicetree model:

- parse command lines into commands, arguments and parameters
- walk the devicetree through a hierarchical file-system metaphor
  (current working branch, relative paths and path references,
  Devicetree labels)
- match nodes with flexible criteria
- sort and filter nodes

Unit tests and examples: tests/test_dtsh_shell.py
"""


from typing import (
    cast,
    Type,
    TypeVar,
    Union,
    Optional,
    Sequence,
    List,
    Tuple,
    Dict,
)

import getopt
import re
import shlex

from dtsh.model import DTPath, DTModel, DTNode, DTNodeSorter, DTNodeCriterion
from dtsh.io import DTShOutput
from dtsh.rl import DTShReadline


DTShOptionT = TypeVar("DTShOptionT", bound="DTShOption")
"""Type variable used for polymorphic access to command options.

    Automatically downcast an option to its specialized type.

    See DTShCommand.with_option().
"""

DTShParamT = TypeVar("DTShParamT", bound="DTShParameter")
"""Type variable used for polymorphic access to command parameters.

    Automatically downcast a parameter to its specialized type.

    See DTShCommand.with_param().
"""


class DTShOption:
    """Base definition for shell command options.

    All options are optional (sic): an option is either a boolean flag that
    defaults to False, or an optional argument with or without default value.

    Please, assume these are const-qualified class attributes: BRIEF, SHORTNAME,
    and LONGNAME.
    """

    BRIEF: str = ""
    """Brief description.

    Constant to be defined by concrete options.
    """

    SHORTNAME: Optional[str] = None
    """Single letter option name, e.g. "h".

    Constant to be defined by concrete options that accept a short getopt form.
    """

    LONGNAME: Optional[str] = None
    """Long option name, e.g. "help".

    Constant to be defined by concrete options that accept a long getopt form.
    """

    def __init__(self) -> None:
        """Initialize the command option, reset state."""
        self.reset()

    @property
    def shortname(self) -> Optional[str]:
        """Single letter option name (without the "-" prefix)."""
        return type(self).SHORTNAME

    @property
    def longname(self) -> Optional[str]:
        """Long option name (without the "--" prefix)."""
        return type(self).LONGNAME

    @property
    def brief(self) -> Optional[str]:
        """Brief description."""
        return type(self).BRIEF

    @property
    def getopt_short(self) -> Optional[str]:
        """Short GNU getopt option form.

        If the option is a flag, this is its short name.

        If the option is an argument, the short name is post-fixed with ":",
        see DTShArg.getopt_short().
        """
        return self.shortname

    @property
    def getopt_long(self) -> Optional[str]:
        """Long GNU getopt option form.

        If the option is a flag, this is its long name.

        If the option is an argument, the long name is post-fixed with "=",
        see DTShArg.getopt_long().
        """
        return self.longname

    @property
    def usage(self) -> str:
        """Usage string.

        If the option is a flag, this is the space separated list of
        the option's lexical forms, e.g. "-h --help".

        If the option is an argument,
        the usage is post-fixed with the argument's name, see DTShArg.usage().
        """
        tokens = []
        if self.shortname:
            tokens.append(f"-{self.shortname}")
        if self.longname:
            tokens.append(f"--{self.longname}")
        return " ".join(tokens)

    @property
    def isset(self) -> bool:
        """Whether this option has been set when parsing the command string."""
        return False

    def reset(self) -> None:
        """Reset this option before parsing a new command string.

        This will unset a flag.
        An argument is either unset or reset to a default value.
        """

    def parsed(self, value: Optional[str] = None) -> None:
        """Set this option.

        If the option is a flag, set the flag.

        If the option is an argument that expects a value,
        set the argument raw valued parsed from the command line.

        Invoked by the supporting command when parsing
        a command string into options and parameters,
        after the GNU getopt parser has successfully finished.

        Args:
            value: The option's value parsed from the command string.
              A flag does not expect any value.
        """

    def __eq__(self, other: object) -> bool:
        """Options equal when their names equal."""
        if isinstance(other, DTShOption):
            return (other.shortname == self.shortname) and (
                other.longname == self.longname
            )
        return False

    def __hash__(self) -> int:
        """An option identity is based on its names."""
        return hash(f"-{self.shortname}--{self.longname}")


class DTShFlag(DTShOption):
    """Devicetree shell command flag.

    A flag enabled a command's option and does not expect any value.
    """

    # The flag's state.
    _on: bool

    @property
    def isset(self) -> bool:
        """Whether this flag has been set when parsing the command string.

        Overrides DTShOption.isset().
        """
        return self._on

    def reset(self) -> None:
        """Unset this flag.

        Overrides DTShOption.reset().
        """
        self._on = False

    def parsed(self, value: Optional[str] = None) -> None:
        """Set this flag.

        Overrides DTShOption.parsed().

        Args:
            value: A flag does not expect a value.
        """
        if value:
            # Should be None since DTShFlag.parsed() is called once
            # the GNU getopt parser has successfully finished.
            raise ValueError(value)
        self._on = True

    def __repr__(self) -> str:
        return f"{type(self)}: {self.isset}"


class DTShArg(DTShOption):
    """Devicetree shell command argument.

    An argument is a command option that expects a value.
    """

    # Name the argument will appear with in the command synopsis.
    _name: str

    # See raw().
    _raw: Optional[str]

    def __init__(self, argname: str) -> None:
        """
        Initialize argument.

        Args:
            name: Name the argument will appear with in the command synopsis.
        """
        super().__init__()
        self._name = argname.upper()

    @property
    def getopt_short(self) -> Optional[str]:
        """Add argument's post-fix to the short getopt option form.

        Overrides DTShOption.getopt_short().
        """
        return f"{self.shortname}:" if self.shortname else None

    @property
    def getopt_long(self) -> Optional[str]:
        """Add argument's post-fix to the long getopt option form.

        Overrides DTShOption.getopt_long().
        """
        return f"{self.longname}=" if self.longname else None

    @property
    def usage(self) -> str:
        """Add argument's name to the usage string.

        Overrides DTShOption.usage().
        """
        return " ".join([super().usage, self._name])

    @property
    def raw(self) -> Optional[str]:
        """Raw argument value parsed on the command string.

        None if the argument has not been parsed().
        """
        return self._raw

    @property
    def isset(self) -> bool:
        """Whether this argument has been set when parsing the command string.

        Overrides DTShOption.isset().
        """
        return self._raw is not None

    def reset(self) -> None:
        """Reset this argument.

        Overrides DTShOption.reset().
        """
        self._raw = None

    def parsed(self, value: Optional[str] = None) -> None:
        """Parsing the command string has set this argument.

        Overrides DTShOption.parsed().

        Args:
            value: The raw argument value parsed from the command string.

        Raises:
            DTShError: The value does not match the option's usage.
        """
        # Should neither be None nor an empty string, since DTShArg.parsed()
        # is called once the GNU getopt parser has successfully finished.
        if not value:
            raise ValueError(None)
        self._raw = value

    def autocomp(
        self, txt: str, sh: "DTSh"
    ) -> List[DTShReadline.CompleterState]:
        """Auto-complete with possible values for this argument.

        Args:
            txt: The input text to complete.
            sh: The shell context.

        Returns:
            A list of argument values that are valid completion states.
        """
        del txt
        del sh
        return []


class DTShParameter:
    """Devicetree shell command parameter.

    Devicetree shell commands support at most one parameter,
    that may expect one or several values,
    depending on its multiplicity.
    """

    MultiplicityT = Union[str, int]

    # Parameter definition.
    _name: str
    _multiplicity: "DTShParameter.MultiplicityT"
    _brief: str

    # Raw parameter values parsed from the command string.
    _raw: List[str]

    def __init__(
        self, name: str, multiplicity: "DTShParameter.MultiplicityT", brief: str
    ) -> None:
        """Initialize the command's parameter.

        Args:
            name: Name the parameter will appear with in the command synopsis.
            multiplicity: Parameter multiplicity.
            brief: Brief description of the parameter.
        """
        self._name = name.upper()
        self._multiplicity = multiplicity
        self._brief = brief
        self.reset()

    @property
    def brief(self) -> str:
        """Brief parameter description."""
        return self._brief

    @property
    def multiplicity(self) -> "DTShParameter.MultiplicityT":
        """Parameter multiplicity.

        - "?": optional parameter, at most one value
        - "+": required parameter, at least one value
        - "*": optional parameter, any number of values
        - N: requirement parameter, exactly N values
        """
        return self._multiplicity

    @property
    def usage(self) -> str:
        """Parameter usage string, e.g. "[PATH]"."""
        if self._multiplicity == "?":
            return f"[{self._name}]"
        if self._multiplicity == "+":
            return f"{self._name} [{self._name} ...]"
        if self._multiplicity == "*":
            return f"[{self._name} ...]"
        if isinstance(self._multiplicity, int):
            N: int = self._multiplicity
            return " ".join(N * [self._name])
        raise ValueError(self._multiplicity)

    @property
    def raw(self) -> Sequence[str]:
        """Raw parameter values parsed from the command string."""
        return self._raw

    def parsed(self, values: List[str]) -> None:
        """Parsing the command string has set this parameter.

        Args:
            values: The parameter value(s) parsed from the command line.

        Raises:
            DTShError: Parameter multiplicity error.
        """
        argc = len(values)

        if self._multiplicity == "?":
            # Expect argc in [0, 1].
            if argc > 1:
                raise DTShError(
                    f"{self._name}: expects at most one value (got {argc})"
                )

        elif self._multiplicity == "+":
            # Expect argc >= 1
            if argc == 0:
                raise DTShError(f"{self._name}: missing parameter value")

        elif self._multiplicity == "*":
            # Any number of values is allowed.
            pass

        elif isinstance(self._multiplicity, int):
            N: int = self._multiplicity
            if argc != N:
                raise DTShError(
                    f"{self._name}: expects {N} value(s) (got {argc})"
                )

        else:
            # Invalid parameter definition.
            raise ValueError(self._multiplicity)

        self._raw = values

    def reset(self) -> None:
        """Reset this parameter values before parsing the command line."""
        self._raw = []

    def autocomp(
        self, txt: str, sh: "DTSh"
    ) -> List[DTShReadline.CompleterState]:
        """Auto-complete parameter value.

        Args:
            txt: The input text to complete.
            sh: The shell context.

        Returns:
            A list of parameter values that are valid completion states.
        """
        del txt
        del sh
        return []


class DTShCommand:
    """Devicetree shell command."""

    # See name().
    _name: str

    # See brief().
    _brief: str

    # See options().
    _options: List[DTShOption]

    # See param().
    _param: Optional[DTShParameter]

    def __init__(
        self,
        name: str,
        brief: str,
        options: Optional[Sequence[DTShOption]],
        parameter: Optional[DTShParameter],
    ) -> None:
        """Initialize a new devicetree shell command.

        Args:
            name: The command's name.
            brief: Brief command description.
            options: The options supported by this command.
            parameter: The parameter expected by this command; if any.
        """
        self._name = name
        self._brief = brief
        self._options = list(options) if options else []
        self._options.insert(0, DTShFlagHelp())
        self._param = parameter

    @property
    def name(self) -> str:
        """Command name."""
        return self._name

    @property
    def brief(self) -> str:
        """Brief description."""
        return self._brief

    @property
    def options(self) -> Sequence[DTShOption]:
        """Supported options.

        All commands support the "-h --help" flag to request
        the command's usage.
        """
        return self._options

    @property
    def param(self) -> Optional[DTShParameter]:
        """Expected parameter."""
        return self._param

    @property
    def synopsis(self) -> str:
        """The command's synopsis."""
        tokens = [self._name]
        for opt in self._options:
            # All [option]s are optional (sic).
            tokens.append(f"[{opt.usage}]")
        if self._param:
            tokens.append(self._param.usage)
        return " ".join(tokens)

    @property
    def getopt_short(self) -> str:
        """Short options specification string compatible with GNU getopt.

        E.g. "hL:" for a command that supports a flag "-h"
        and an argument "-L".
        """
        return "".join(
            # getopt_short should be defined, the or clause is added
            # for type hinting consistency.
            [opt.getopt_short or "" for opt in self._options if opt.shortname]
        )

    @property
    def getopt_long(self) -> List[str]:
        """Long options specification list compatible with GNU getopt.

        E.g. ["help", "depth="] for a command that supports a flag "--help",
        and an argument "--depth <depth>".
        """
        # getopt_long should be defined, the or clause is added
        # for type hinting consistency.
        return [opt.getopt_long or "" for opt in self._options if opt.longname]

    def option(self, name: str) -> Optional[DTShOption]:
        """Retrieve command options by lexical name.

        Args:
            name: An option's lexical name, either in short form (e.g. "-h"),
              or in long form (e.g. "--help").

        Returns:
            The searched for command's option, None if not found.
        """
        if name.startswith("--"):
            longname = name[2:]
            for opt in self._options:
                if longname == opt.longname:
                    return opt
        elif name.startswith("-"):
            shortname = name[1:]
            for opt in self._options:
                if shortname == opt.shortname:
                    return opt
        return None

    def with_option(self, option_t: Type[DTShOptionT]) -> DTShOptionT:
        """Polymorphic access to the command options.

        Args:
            option_t: The option type.
              The command MUST support this option type.

        Returns:
            The command's option downcast to its specialized type.
        """
        opt = None
        if option_t.SHORTNAME:
            opt = self.option(f"-{option_t.SHORTNAME}")
        elif option_t.LONGNAME:
            opt = self.option(f"--{option_t.LONGNAME}")
        if not opt:
            raise KeyError(option_t)
        return cast(DTShOptionT, opt)

    def with_flag(self, flag_t: Type[DTShFlag]) -> bool:
        """Access a flag state.

        Args:
            flag_t: The flag type.
              The command MUST support this flag type.

        Returns:
            True when the command flag is set.
        """
        return self.with_option(flag_t).isset

    def with_arg(self, arg_t: Type[DTShOptionT]) -> DTShOptionT:
        """Polymorphic access to the command arguments.

        Args:
            arg_t: The argument type.
              The command MUST support this argument type.

        Returns:
            The command argument downcast to its specialized type.
        """
        return self.with_option(arg_t)

    def with_param(self, param_t: Type[DTShParamT]) -> DTShParamT:
        """Polymorphic access to the command parameter.

        Args:
            param_t: The parameter type.
              The command MUST expect a parameter.

        Returns:
            The command parameter downcast to its specialized type.
        """
        if self._param:
            return cast(DTShParamT, self._param)
        raise KeyError(param_t)

    def reset(self) -> None:
        """Reset the command's options and parameter."""
        for opt in self._options:
            opt.reset()
        if self._param:
            self._param.reset()

    def parse_argv(self, argv: Sequence[str]) -> None:
        """Parse a command string into options and parameter.

        Args:
            argv: The command string lexical split.

        Raises:
            DTShUsageError: The arguments do not match the command usage.
        """
        # Always reset options and parameter before parsing.
        self.reset()
        try:
            parsed_opts, parsed_param_values = getopt.gnu_getopt(
                list(argv), self.getopt_short, self.getopt_long
            )
        except (getopt.GetoptError, DTShError) as e:
            raise DTShUsageError(self, e.msg) from e

        try:
            for opt_name, opt_arg in parsed_opts:
                opt = self.option(opt_name)
                if opt:
                    opt.parsed(opt_arg)
                else:
                    # Should not happen, since the getopt parser
                    # has just successfully finished.
                    pass
        except DTShError as e:
            raise DTShUsageError(self, f"{opt_name}: {e.msg}") from e

        if self._param:
            try:
                self._param.parsed(parsed_param_values)
            except DTShError as e:
                raise DTShUsageError(self, e.msg) from e

        elif parsed_param_values:
            raise DTShUsageError(
                self, f"unexpected parameter: {' '.join(parsed_param_values)}"
            )

        if self.with_flag(DTShFlagHelp):
            # User's asked for help.
            raise DTShUsageError(self)

    def execute(self, argv: Sequence[str], sh: "DTSh", out: DTShOutput) -> None:
        """Execute the command.

        Args:
            argv: The command arguments parsed from the command string
              (options and parameter values).
            sh: The shell context.
            out: Where the command will write its output.

        Raises:
            DTShUsageError: The arguments does not match the command usage.
            DTShCommandError: The command execution has failed.
        """
        del sh  # Unused by base implementation.
        del out  # Unused by base implementation.
        self.parse_argv(argv)

    def __eq__(self, other: object) -> bool:
        """Commands equal when their names equal."""
        if isinstance(other, DTShCommand):
            return other.name == self.name
        return False

    def __lt__(self, other: object) -> bool:
        """Default order based on command names."""
        if isinstance(other, DTShCommand):
            return self._name < other._name
        raise TypeError(other)

    def __hash__(self) -> int:
        """A command identity is based on its names."""
        return hash(self.name)


class DTSh:
    """Devicetree shell.

    Shell-like interface with a devicetree:

    - hierarchical file-system metaphor: view the device tree from a
      current working branch, support for relative paths and path references
    - built-in commands to walk, search and describe the devicetree
    - parse command lines and execute command strings
    """

    VERSION_STRING = "0.0.99"
    """The devicetree shell version string.

    This version identifies:

    - the Zephyr West command syntax and arguments
    - the devicetree shell command line syntax
    - the defined built-in devicetree shell commands,
      including their options and parameters
    - the devicetree shell configuration file syntax (see dtsh.ini)
      and the bundled default configuration
    - the devicetree shell theme file syntax (see rich/theme.ini)
      and the bundled default theme
    """

    class PathExpansion:
        """Path expansion.

        See path_expansion().
        """

        _prefix: str
        _nodes: Sequence[DTNode]

        def __init__(self, prefix: str, nodes: Sequence[DTNode]) -> None:
            self._prefix = prefix
            self._nodes = nodes

        @property
        def prefix(self) -> str:
            """Expansion's prefix."""
            return self._prefix

        @property
        def nodes(self) -> Sequence[DTNode]:
            """Matched nodes."""
            return self._nodes

        def __repr__(self) -> str:
            return f"{self.prefix}: {self.nodes}"

        def __eq__(self, other: object) -> bool:
            if isinstance(other, DTSh.PathExpansion):
                return (other.prefix == self.prefix) and (
                    other.nodes == self.nodes
                )
            return False

    # Match label references (think &) in devicetree paths.
    _re_labelref: re.Pattern[str] = re.compile(r"^(?P<labelref>&[^/]+)")

    # See commands().
    _commands: Dict[str, DTShCommand]

    # See dt().
    _dt: DTModel

    # See cwd().
    _cwd: DTNode

    def __init__(
        self,
        dt: DTModel,
        builtins: Sequence[DTShCommand],
    ) -> None:
        """Initialize a devicetree shell.

        The current working branch is set to the devicetree root.

        Args:
            dt: The devicetree to interface with.
            builtins: The shell built-in commands.
        """
        self._dt = dt
        self._cwd = self._dt.root
        self._commands = {cmd.name: cmd for cmd in builtins}

    @property
    def dt(self) -> DTModel:
        """The devicetree model this shell interfaces with."""
        return self._dt

    @property
    def cwd(self) -> DTNode:
        """The current working branch."""
        return self._cwd

    @property
    def pwd(self) -> str:
        """Path name of the current working branch."""
        return self._cwd.path

    @property
    def commands(self) -> List[DTShCommand]:
        """The commands supported by this shell."""
        return list(self._commands.values())

    def realpath(self, path: str) -> str:
        """Normalize a devicetree path and resolve DT labels.

        Args:
            path: An absolute or relative path, possibly containing
              path references and DT labels.

        Returns:
            A devicetree path name.

        Raises:
            DTPathNotFoundError: Failed to resolve a devicetree label.
        """
        m = DTSh._re_labelref.match(path)
        if m and m.group("labelref"):
            labelref = m.group("labelref")
            label = labelref[1:]
            try:
                node = self._dt.labeled_nodes[label]
                path = path.replace(m.group("labelref"), node.path, 1)
            except KeyError as e:
                raise DTPathNotFoundError(labelref) from e
        return DTPath.abspath(path, self._cwd.path)

    def pathway(self, node: DTNode, prefix: str) -> str:
        """Get the pathway from an origin to a node.

        The origin node is represented by a prefix: it may be any expression
        that resolves to a devicetree branch.

        The pathway is then the relative path from origin to node,
        where the prefix is substituted for the origin's path.

        Pathways match how common POSIX shell commands like "ls", "find"
        or "tree" would output file and directory paths.

        Args:
            node: The pathway destination node.
            prefix: The origin's prefix. An empty prefix resolves to
              the shell's current working branch, represented by an empty string.

        Returns:
            The pathway from origin to node.

        Raises:
            DTPathNotFoundError: The prefix does not resolve to a node.
        """
        relpath = DTPath.relpath(node.path, self.node_at(prefix).path)

        if relpath == ".":
            path = prefix
        else:
            path = DTPath.join(prefix, relpath)

        return path

    def path_expansion(
        self, path: str, enabled_only: bool = False
    ) -> PathExpansion:
        """Expand a DT path expression that may contain "*" wild-cards.

        Boilerplate code to expand a DT path expression into:
        - the nodes the expression expands to
        - a prefix to use as origin when getting pathways

        If the path expression actually contains wild-cards:
        - the expression expands to the matching nodes (globbing),
          according to enabled_only
        - in pathways:
          - prefix is a valid origin to substitute for the dirname
            in the matched nodes' paths
          - the current working branch is represented by an empty string
            unless the path expression explicitly starts with "."

        Otherwise, if no expansion actually happens:
        - the expression should resolve to a unique node, ignoring enabled_only
        - in pathways:
          - prefix will represent this node's path as a common origin
            for relative pathways
          - "." is substituted for an empty prefix

        See also DTSh.pathway().

        Args:
            path: A DT path expression.
            enabled_only: Whether to filter out disabled nodes.

        Returns:
            The expanded path. May be empty if all nodes have been
            filtered out because they are disabled.

        Raises:
            DTPathNotFoundError: The expanded path is not a node's path,
            or the path expansion didn't match any node.
        """
        prefix: str
        nodes: List[DTNode]

        basename = DTPath.basename(path)
        if "*" in basename:
            dirname = DTPath.dirname(path)
            if dirname == "." and not path.startswith("."):
                prefix = ""
            else:
                prefix = dirname

            pattern = re.escape(basename).replace(r"\*", ".*")
            pattern = f"^{pattern}$"
            re_basename = re.compile(pattern)

            # Path expansion.
            globs = [
                node
                for node in self.node_at(dirname).children
                if re_basename.match(node.name)
            ]
            if not globs:
                # We consider empty expansions as "path not found" errors,
                # like most Un*x shells do.
                raise DTPathNotFoundError(path)

            if enabled_only:
                nodes = [node for node in globs if node.enabled]
            else:
                nodes = globs

        else:
            prefix = path or "."
            nodes = [self.node_at(path)]

        return DTSh.PathExpansion(prefix, nodes)

    def node_at(self, path: str) -> DTNode:
        """Access the devicetree node at path.

        The path is normalized before the node lookup.

        The node MUST exist.

        Args:
            path: An absolute or relative DT path.
              An empty path will answer the current working branch.

        Returns:
            The node at path.

        Raises:
            DTPathNotFoundError: Invalid devicetree path.
        """
        pathname = self.realpath(path)
        try:
            return self._dt[pathname]
        except KeyError as e:
            raise DTPathNotFoundError(pathname) from e

    def cd(self, path: Optional[str] = None) -> None:
        """Change the current working branch.

        Args:
            path: Absolute or relative DT path to the new working branch.
              Defaults to the devicetree root.

        Raises:
            DTPathNotFoundError: The destination branch does not exist.
        """
        self._cwd = self.node_at(path) if path else self.dt.root

    def find(
        self,
        path: str,
        criterion: DTNodeCriterion,
        /,
        order_by: Optional[DTNodeSorter] = None,
        reverse: bool = False,
        enabled_only: bool = False,
    ) -> List[DTNode]:
        """Search branch at path.

        Args:
            path: Absolute or relative path to the devicetree branch to search.
              An empty path will represent the current working branch.
            criterion: The criterion the nodes must match.
            order_by: Sort matched nodes, None will preserve DTS-order.
            reverse: Whether to reverse sort order.
              If set and no order_by is given, means reverse DTS-order.
            enabled_only: Whether to stop at disabled branches.

        Returns:
            The list of matched nodes.

        Raises:
            DTPathNotFoundError: A search branch does not exist.
        """
        root = self.node_at(path)
        nodes = root.find(
            criterion,
            # We don't want children ordering here.
            order_by=None,
            enabled_only=enabled_only,
        )

        # Sort only matched nodes.
        if order_by:
            nodes = order_by.sort(nodes, reverse=reverse)
        elif reverse:
            nodes.reverse()

        return nodes

    def parse_cmdline(
        self, cmdline: str
    ) -> Tuple[DTShCommand, List[str], Optional[str]]:
        """Parse a command line.

        A command line is defined as a command string followed by optional
        output redirection:

            COMMAND_LINE := COMMAND_STRING [REDIR2_STRING]

        Command strings are defined according to the Utility Syntax Guidelines,
        and getopt syntax:

            COMMAND_STRING := COMMAND [OPTION]... [PARAM]...

        where:

        - CMD is a shell command name
        - OPTIONs and PARAMs are not positional

        Parameter values can't start with ">".

        The output redirection string is parsed from the first lex
        that starts with ">" and is not an option value:

            REDIR2_STRING := >|>> PATH

        The redirection operators (">" and ">>") have POSIX-like semantic
        (i.e resp. "create" and "append").

        The extension of the file the command's output is redirected to
        determines its actual format (HTML, SVG, or simple text file).

        See:

        - DTShOption, DTShParameter
        - https://docs.python.org/3.9/library/getopt.html
        - https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap12.html

        Args:
            cmdline: The non empty command line to parse.

        Returns:
            The parsed tuple (command, arguments, redirection),
            where arguments include the parsed options and parameters.

        Raises:
            DTShCommandNotFoundError: The first lex of the command line does
            not represent a valid shell command.
        """
        if not cmdline:
            raise ValueError()

        v_cmdline: List[str] = [
            # If operating in POSIX mode, shlex will preserve the literal value
            # of the next character that follows a non-quoted escape characters:
            # e.g. input string \d is parsed as d: this would require
            # RE patterns that contain character classes to be quoted,
            # e.g find --with-label "led[\d]".
            # If operating in non POSIX mode, escape characters are not
            # recognized (thus \d stays \d), but neither are the
            # quote characters within words, and a parsed command line argument
            # may contain quotes.
            # Operating in non POSIX mode, while striping leading and trailing
            # quotes, seems to work fine for all our use cases.
            lex.strip('"')
            for lex in shlex.split(cmdline, posix=False)
        ]

        cmd_name = v_cmdline[0]
        try:
            cmd = self._commands[cmd_name]
        except KeyError as e:
            raise DTShCommandNotFoundError(cmd_name) from e

        cmd_argc = 0
        expect_val = False
        for lex in v_cmdline[1:]:
            # The lex starts a command output redirection:
            # - it's not a parameter (parameter values can't start with ">")
            # - it's not an option name (would start with "-")
            # - it's not an option value
            if lex.startswith(">") and not expect_val:
                break
            if expect_val:
                # Consume expected option value.
                expect_val = False
            else:
                opt = cmd.option(lex)
                if opt and isinstance(opt, DTShArg):
                    # Current lex is an option that expects a value.
                    expect_val = True
            # Any lex before the redirection statement is a command argument
            # (option or parameter).
            cmd_argc += 1

        cmd_argv = v_cmdline[1 : cmd_argc + 1]

        v_redir2 = v_cmdline[cmd_argc + 1 :]
        if v_redir2:
            redir2 = " ".join(v_redir2)
        else:
            redir2 = None

        return (cmd, cmd_argv, redir2)


class DTShError(Exception):
    """Base for devicetree shell errors."""

    _msg: Optional[str]

    def __init__(self, msg: Optional[str] = None) -> None:
        """A shell error happened.

        Args:
            msg: A message describing the error.
        """
        self._msg = msg

    @property
    def msg(self) -> str:
        """A (may be empty) message describing the error."""
        return self._msg or ""


class DTShCommandError(DTShError):
    """An exceptional condition happened while executing a shell command.

    This may indicate:

    - the command was misused or the user's simply asked for help
      (see DTShUsageError)
    - the command execution has failed

    """

    _cmd: DTShCommand

    def __init__(self, cmd: DTShCommand, msg: Optional[str] = None) -> None:
        """New error.

        Args:
            cmd: The failed command.
            msg: A message describing the error.
        """
        super().__init__(msg)
        self._cmd = cmd

    @property
    def cmd(self) -> DTShCommand:
        """The failed command."""
        return self._cmd


class DTShUsageError(DTShCommandError):
    """An exceptional condition happened while parsing a command string.

    This may indicate:

    - the command string does not match a command's usage
    - the user has asked for the command's usage by setting
      the DTShFlagHelp flag on the command string

    For example:

        try:
            sh.execute(...)
        except DTShUsageError as e:
            if e.cmd.with_flag(DTShFlagHelp):
                # The User's asked for help.
            else:
                # The command string does not match the command's usage.

    """


class DTPathNotFoundError(DTShError):
    """A DT node or branch does not exist."""

    _path: str

    def __init__(self, path: str) -> None:
        """New error.

        Args:
            path: The invalid path name.
        """
        super().__init__(f"node not found: {path}")
        self._path = path

    @property
    def path(self) -> str:
        """The invalid path name."""
        return self._path


class DTShCommandNotFoundError(DTShError):
    """A shell command does not exist."""

    def __init__(self, name: str) -> None:
        """New error.

        Args:
            name: The undefined command name.
        """
        super().__init__(f"{name}: command not found")
        self._name = name

    @property
    def name(self) -> str:
        """The undefined command name."""
        return self._name


class DTShFlagHelp(DTShFlag):
    """Help flag, supported by all devicetree shell commands.

    When set, the command will display it's usage string.
    """

    BRIEF: str = "print command help"
    SHORTNAME: Optional[str] = "h"
    LONGNAME: Optional[str] = "help"
