# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Completion logic and base display callbacks for GNU Readline integration.

Unit tests and examples: tests/test_dtsh_autocomp.py
"""


from typing import cast, List, Set

import os

from dtsh.model import DTPath, DTNode, DTBinding
from dtsh.rl import DTShReadline
from dtsh.io import DTShOutput
from dtsh.config import DTShConfig
from dtsh.shell import (
    DTSh,
    DTShCommand,
    DTShOption,
    DTShArg,
    DTShCommandNotFoundError,
    DTPathNotFoundError,
)


_dtshconf: DTShConfig = DTShConfig.getinstance()


class RlStateDTShCommand(DTShReadline.CompleterState):
    """RL completer state for DTSh commands."""

    def __init__(self, rlstr: str, cmd: DTShCommand) -> None:
        """Initialize completer state.

        Args:
          rlstr: The command name to substitute the RL completion scope with.
          cmd: The corresponding command.
        """
        super().__init__(rlstr, cmd)

    @property
    def cmd(self) -> DTShCommand:
        """The corresponding command."""
        return cast(DTShCommand, self._item)


class RlStateDTShOption(DTShReadline.CompleterState):
    """RL completer state for DTSh command options."""

    def __init__(self, rlstr: str, opt: DTShOption) -> None:
        """Initialize completer state.

        Args:
          rlstr: The option name to substitute the RL completion scope with.
          opt: The corresponding option.
        """
        super().__init__(rlstr, opt)

    @property
    def opt(self) -> DTShOption:
        """The corresponding option."""
        return cast(DTShOption, self._item)


class RlStateDTPath(DTShReadline.CompleterState):
    """RL completer state for Devicetree paths."""

    def __init__(self, rlstr: str, node: DTNode) -> None:
        """Initialize completer state.

        Args:
          rlstr: The absolute or relative Devicetree path to substitute
            the RL completion scope with.
          node: The corresponding Devicetree node.
        """
        super().__init__(rlstr, node)

    @property
    def node(self) -> DTNode:
        """The corresponding Devicetree node."""
        return cast(DTNode, self._item)


class RlStateCompatStr(DTShReadline.CompleterState):
    """RL completer state for compatible strings."""

    _bindings: Set[DTBinding]

    def __init__(self, rlstr: str, bindings: Set[DTBinding]) -> None:
        """Initialize completer state.

        Args:
          rlstr: The compatible string to substitute the RL completion scope with.
        """
        super().__init__(rlstr, None)
        self._bindings = bindings

    @property
    def compatstr(self) -> str:
        """The corresponding compatible string value."""
        return self._rlstr

    @property
    def bindings(self) -> Set[DTBinding]:
        """All bindings for this compatible (e.g. for different buses)."""
        return self._bindings


class RlStateDTVendor(DTShReadline.CompleterState):
    """RL completer state for device or device class vendors."""

    def __init__(self, rlstr: str, vendor: str) -> None:
        """Initialize completer state.

        Args:
          rlstr: The vendor prefix to substitute the RL completion scope with.
          vendor: The corresponding vendor name.
        """
        super().__init__(rlstr, vendor)

    @property
    def prefix(self) -> str:
        """The corresponding vendor prefix."""
        return self._rlstr

    @property
    def vendor(self) -> str:
        """The corresponding vendor name."""
        return cast(str, self._item)


class RlStateDTBus(DTShReadline.CompleterState):
    """RL completer state for Devicetree bus protocols."""

    def __init__(self, rlstr: str) -> None:
        """Initialize completer state.

        Args:
          rlstr: The bus protocol to substitute the RL completion scope with.
        """
        super().__init__(rlstr, None)

    @property
    def proto(self) -> str:
        """The bus protocol."""
        return self._rlstr


class RlStateDTAlias(DTShReadline.CompleterState):
    """RL completer state for aliased nodes."""

    def __init__(self, rlstr: str, node: DTNode) -> None:
        """Initialize completer state.

        Args:
          rlstr: The alias name to substitute the RL completion scope with.
          node: The aliased node.
        """
        super().__init__(rlstr, node)

    @property
    def alias(self) -> str:
        """The corresponding alias name."""
        return self._rlstr

    @property
    def node(self) -> DTNode:
        """The aliased node."""
        return cast(DTNode, self._item)


class RlStateDTChosen(DTShReadline.CompleterState):
    """RL completer state for chosen nodes."""

    def __init__(self, rlstr: str, node: DTNode) -> None:
        """Initialize completer state.

        Args:
          rlstr: The parameter name to substitute the RL completion scope with.
          node: The chosen node.
        """
        super().__init__(rlstr, node)

    @property
    def chosen(self) -> str:
        """The chosen parameter name."""
        return self._rlstr

    @property
    def node(self) -> DTNode:
        """The chosen node."""
        return cast(DTNode, self._item)


class RlStateDTLabel(DTShReadline.CompleterState):
    """RL completer state for labeled nodes."""

    def __init__(self, rlstr: str, node: DTNode) -> None:
        """Initialize completer state.

        Args:
          rlstr: The label candidate, prefixed with "&" when completing
            path parameters, without the leading "&" otherwise.
          node: The labeled node.
        """
        super().__init__(rlstr, node)

    @property
    def label(self) -> str:
        """The label (without the leading "&")."""
        return self._rlstr[1:] if self._rlstr.startswith("&") else self._rlstr

    @property
    def node(self) -> DTNode:
        """The labeled node."""
        return cast(DTNode, self._item)


class RlStateFsEntry(DTShReadline.CompleterState):
    """RL completer state for file-system paths."""

    def __init__(self, rlstr: str, dirent: os.DirEntry[str]) -> None:
        """Initialize completer state.

        Args:
          rlstr: The file-system path to substitute the RL completion scope with.
          dirent: The corresponding file or directory.
        """
        super().__init__(rlstr, dirent)

    @property
    def dirent(self) -> os.DirEntry[str]:
        """The corresponding file or directory."""
        return cast(os.DirEntry[str], self._item)


class RlStateEnum(DTShReadline.CompleterState):
    """RL completer state for enumerated values."""

    def __init__(self, rlstr: str, brief: str) -> None:
        """Initialize completer state.

        Args:
          rlstr: The value string to substitute the RL completion scope with.
          brief: The value semantic.
        """
        super().__init__(rlstr, brief)

    @property
    def value(self) -> str:
        """The enumerated value."""
        return self._rlstr

    @property
    def brief(self) -> str:
        """The corresponding description."""
        return cast(str, self._item)


class DTShAutocomp:
    """Base completion logic and display callbacks for GNU readline integration."""

    # The shell this will auto-complete the command line of.
    _dtsh: DTSh

    @staticmethod
    def complete_dtshcmd(
        cs_txt: str, sh: DTSh
    ) -> List[DTShReadline.CompleterState]:
        """Complete command name.

        Args:
            cs_txt: The command name to complete.
            sh: The context shell.

        Returns:
            The commands that are valid completer states.
        """
        return sorted(
            RlStateDTShCommand(cmd.name, cmd)
            for cmd in sh.commands
            if cmd.name.startswith(cs_txt)
        )

    @staticmethod
    def complete_dtshopt(
        cs_txt: str, cmd: DTShCommand
    ) -> List[DTShReadline.CompleterState]:
        """Complete option name.

        Args:
            cs_txt: The option name to complete, starting with "--" or "-".
            cmd: The command to search for matching options.

        Returns:
            The options that are valid completer states.
        """
        states: List[DTShReadline.CompleterState] = []

        if cs_txt.startswith("--"):
            # Only options with a long name.
            prefix = cs_txt[2:]
            states.extend(
                sorted(
                    [
                        RlStateDTShOption(f"--{opt.longname}", opt)
                        for opt in cmd.options
                        if opt.longname and opt.longname.startswith(prefix)
                    ],
                    key=lambda x: "-"
                    if x.rlstr == "--help"
                    else x.rlstr.lower(),
                )
            )
        elif cs_txt.startswith("-"):
            if cs_txt == "-":
                # All options, those with a short name first.
                states.extend(
                    sorted(
                        [
                            RlStateDTShOption(f"-{opt.shortname}", opt)
                            for opt in cmd.options
                            if opt.shortname
                        ],
                        key=lambda x: "-"
                        if x.rlstr == "-h"
                        else x.rlstr.lower(),
                    )
                )
                # Then options with only a long name.
                states.extend(
                    sorted(
                        [
                            RlStateDTShOption(f"--{opt.longname}", opt)
                            for opt in cmd.options
                            if not opt.shortname
                        ],
                        key=lambda x: x.rlstr.lower(),
                    )
                )
            else:
                # Unique match, assuming the completion scope is "-<shortname>".
                opt = cmd.option(cs_txt)
                if opt:
                    states.append(RlStateDTShOption(f"-{opt.shortname}", opt))

        return states

    @staticmethod
    def complete_dtpath(
        cs_txt: str, sh: DTSh
    ) -> List[DTShReadline.CompleterState]:
        """Complete Devicetree path.

        Args:
            cs_txt: The absolute or relative Devicetree path to complete.
            sh: The context shell.

        Returns:
            The Devicetree paths that are valid completer states.
        """

        if cs_txt.startswith("&"):
            parts = DTPath.split(cs_txt)
            if len(parts) == 1:
                # Auto-complete with labels only for the first path component.
                return DTShAutocomp.complete_dtlabel(cs_txt, sh)

        dirname = DTPath.dirname(cs_txt)
        if (dirname == ".") and not cs_txt.startswith("./"):
            # We'll search the current working branch
            # because we complete a relative DT path.
            # Though, we don't want to include the "." path component
            # into the substitution strings if it's not actually part
            # of the completion scope.
            dirname = ""

        states: List[DTShReadline.CompleterState] = []
        try:
            dirnode = sh.node_at(dirname)
        except DTPathNotFoundError:
            # Won't complete invalid path.
            pass
        else:
            prefix = DTPath.basename(cs_txt)
            states.extend(
                # Intelligently join <dirname> and <nodename>
                # to get a valid substitution string.
                RlStateDTPath(DTPath.join(dirname, child.name), child)
                for child in dirnode.children
                if child.name.startswith(prefix)
            )

        return sorted(states)

    @staticmethod
    def complete_dtcompat(
        cs_txt: str, sh: DTSh
    ) -> List[DTShReadline.CompleterState]:
        """Complete compatible string.

        Args:
            cs_txt: The compatible string to complete.
            sh: The context shell.

        Returns:
            The compatible string values that are valid completer states,
            associated with the relevant bindings.
        """
        states: List[DTShReadline.CompleterState] = []
        for compatible in sorted(
            (
                compat
                for compat in sh.dt.compatible_strings
                if compat.startswith(cs_txt)
            )
        ):
            bindings: Set[DTBinding] = set()
            # 1st, try the natural API, with unknown bus.
            binding = sh.dt.get_compatible_binding(compatible)
            if binding:
                # Exact single match (binding that does not expect
                # a bus of appearance).
                bindings.add(binding)
            else:
                # The compatible string is associated with a bus of appearance:
                # - look for nodes specified by a binding whose compatible string
                #   matches our search
                # - for each node, add its binding to those associated with
                #   the compatible string candidate
                for node in sh.dt.get_compatible_devices(compatible):
                    if node.binding:
                        bindings.add(node.binding)

            states.append(RlStateCompatStr(compatible, bindings))

        return states

    @staticmethod
    def complete_dtvendor(
        cs_txt: str, sh: DTSh
    ) -> List[DTShReadline.CompleterState]:
        """Complete vendor prefix.

        Args:
            cs_txt: The vendor prefix to complete.
            sh: The context shell.

        Returns:
            The vendors that are valid completer states.
        """
        return sorted(
            RlStateDTVendor(vendor.prefix, vendor.name)
            for vendor in sh.dt.vendors
            if vendor.prefix.startswith(cs_txt)
        )

    @staticmethod
    def complete_dtbus(
        cs_txt: str, sh: DTSh
    ) -> List[DTShReadline.CompleterState]:
        """Complete bus protocol.

        Args:
            cs_txt: The bus protocol to complete.
            sh: The context shell.

        Returns:
            The bus protocols that are valid completer states.
        """
        return sorted(
            RlStateDTBus(proto)
            for proto in sh.dt.bus_protocols
            if proto.startswith(cs_txt)
        )

    @staticmethod
    def complete_dtalias(
        cs_txt: str, sh: DTSh
    ) -> List[DTShReadline.CompleterState]:
        """Complete alias name.

        Args:
            cs_txt: The alias name to complete.
            sh: The context shell.

        Returns:
            The aliased nodes that are valid completer states.
        """
        return sorted(
            RlStateDTAlias(alias, node)
            for alias, node in sh.dt.aliased_nodes.items()
            if alias.startswith(cs_txt)
        )

    @classmethod
    def complete_dtchosen(
        cls, cs_txt: str, sh: DTSh
    ) -> List[DTShReadline.CompleterState]:
        """Complete chosen parameter name.

        Args:
            cs_txt: The chosen parameter name to complete.
            sh: The context shell.

        Returns:
            The chosen nodes that are valid completer states.
        """
        return sorted(
            RlStateDTChosen(chosen, node)
            for chosen, node in sh.dt.chosen_nodes.items()
            if chosen.startswith(cs_txt)
        )

    @classmethod
    def complete_dtlabel(
        cls, cs_txt: str, sh: DTSh
    ) -> List[DTShReadline.CompleterState]:
        """Complete devicetree label.

        Args:
            cs_txt: The devicetree label to complete,
              with or without the leading "&".
              If present, it's retained within the completion state.
            sh: The context shell.

        Returns:
            The labeled nodes that are valid completer states.
        """
        # The label pattern is shifted when the completion scope starts with "&".
        i_label = 1 if cs_txt.startswith("&") else 0

        if cs_txt.endswith("/"):
            # We assume we're completing a devicetree path like "&label/":
            # the path to the node with label "label" is the only match.
            label = cs_txt[i_label:-1]
            try:
                node = sh.dt.labeled_nodes[label]
                return [RlStateDTPath(node.path, node)]
            except KeyError:
                return []

        # Complete with matching labels.
        pattern = cs_txt[i_label:]
        states: List[RlStateDTLabel] = [
            # "&" is retained within the completion state
            # when it's present in the completion scope.
            RlStateDTLabel(f"&{label}" if (i_label > 0) else label, node)
            for label, node in sh.dt.labeled_nodes.items()
            if label.startswith(pattern)
        ]

        if len(states) == 1:
            # Exact match: convert completion state to path.
            return [RlStateDTPath(states[0].node.path, states[0].node)]
        return sorted(states)

    @staticmethod
    def complete_fspath(cs_txt: str) -> List[DTShReadline.CompleterState]:
        """Complete file-system path.

        Args:
            cs_txt: The file-system path to complete.

        Returns:
            The file-system paths that are valid completer states.
        """
        if cs_txt.startswith("~"):
            # We'll always expand "~" in both the auto-completion input
            # and the completion states.
            cs_txt = cs_txt.replace("~", os.path.expanduser("~"), 1)

        dirname = os.path.dirname(cs_txt)
        if dirname:
            dirpath = os.path.abspath(dirname)
        else:
            dirpath = os.getcwd()

        if not os.path.isdir(dirpath):
            return []

        basename = os.path.basename(cs_txt)
        fs_entries = [
            entry
            for entry in os.scandir(dirpath)
            if entry.name.startswith(basename)
        ]
        if _dtshconf.pref_fs_hide_dotted:
            # Hide commonly hidden files and directories on POSIX-like systems.
            fs_entries = [
                entry for entry in fs_entries if not entry.name.startswith(".")
            ]

        fs_entries.sort(key=lambda entry: entry.name)

        # Directories 1st.
        states: List[DTShReadline.CompleterState] = [
            # Append "/" to completion matches for directories.
            RlStateFsEntry(
                f"{os.path.join(dirname, entry.name)}{os.path.sep}", entry
            )
            for entry in fs_entries
            if entry.is_dir()
        ]
        # Then files.
        states.extend(
            RlStateFsEntry(os.path.join(dirname, entry.name), entry)
            for entry in fs_entries
            if entry.is_file()
        )

        return states

    def __init__(self, sh: DTSh) -> None:
        """Initialize the auto-completion helper.

        Args:
            sh: The context shell.
        """
        self._dtsh = sh

    def complete(
        self, cs_txt: str, rlbuf: str, cs_begin: int, cs_end: int
    ) -> List[DTShReadline.CompleterState]:
        """Auto-complete a DTSh command line.

        Derived classes may override this method to post-process
        the completions, e.g.:
        - append a space to completion matches, exiting the completion process
          when there remains only a single match
        - pretend there's no completion when there's a single match,
          also to exit the completion process
        - append "/" to matched directories when completing file system entries

        Implements DTShReadline.CompletionCallback.

        Args:
            cs_txt: The input text to complete (aka the RL completion scope
              content).
            rlbuf: The readline input buffer content (aka the current content
              of the command string).
            begin: Beginning of completion scope, index of the first character
              in the completion scope).
            end: End of the completion scope, index of the character immediately
              after the completion scope, such that (end - begin) is the
              length of the completion scope. This is also the input caret
              index wrt the RL buffer.

        Returns:
            The list of candidate completions.
        """
        if (cs_end < len(rlbuf)) and (rlbuf[cs_end] != " "):
            # Some auto-completion providers (e.g. zsh) have an option
            # to auto-complete the command line regardless of the caret position.
            # This sometimes results in a confusing user experience
            # when not completing past the end of a word.
            # DTSh will always answer there's no completion on such situations.
            return []

        # Auto-completion is asking for possible command names
        # when the command line before the completion scope
        # contains only spaces.
        ante_cs_txt = rlbuf[:cs_begin].strip()
        if not ante_cs_txt:
            return DTShAutocomp.complete_dtshcmd(cs_txt, self._dtsh)

        try:
            cmd, _, redir2 = self._dtsh.parse_cmdline(rlbuf)
        except DTShCommandNotFoundError:
            # Won't auto-complete a command line deemed to fail.
            return []

        if redir2:
            # Auto-completion is asking for possible redirection file paths
            # when we're past the last redirection operator.
            if cs_begin > rlbuf.rfind(">"):
                return DTShAutocomp.complete_fspath(cs_txt)

        # At this point, auto-completion may be asking for:
        # - either a command's option name
        # - or a command's argument possible values
        # - or a command's parameter possible values

        if cs_txt.startswith("-"):
            # We assume possible argument and parameter values
            # won't start with "-".
            return DTShAutocomp.complete_dtshopt(cs_txt, cmd)

        # Auto-completion is asking for possible argument values
        # when the completion scope immediately follows a command's
        # argument name.
        ante_opts = ante_cs_txt.split()
        if ante_opts:
            last_opt = cmd.option(ante_opts[-1])
            if isinstance(last_opt, DTShArg):
                return last_opt.autocomp(cs_txt, self._dtsh)

        # Eventually, try to auto-complete with parameter values.
        if cmd.param:
            return cmd.param.autocomp(cs_txt, self._dtsh)

        return []

    def display(
        self,
        out: DTShOutput,
        states: List[DTShReadline.CompleterState],
    ) -> None:
        """Default DTSh callback for displaying the completion candidates.

        The completion model is typically produced by DtshAutocomp.complete().

        Derived classes may override this method
        to provide an alternative (aka rich) completion matches display.

        Implements DTShReadline.DisplayCallback.

        Args:
            out: Where to display these completions.
            completions: The completions to display.
        """
        for state in states:
            if isinstance(state, RlStateDTShCommand):
                cmd = state.cmd
                out.write(f"{cmd.name}  {cmd.brief}")

            elif isinstance(state, RlStateDTShOption):
                opt = state.opt
                out.write(f"{opt.usage}  {opt.brief}")

            elif isinstance(state, RlStateDTPath):
                node = state.node
                out.write(node.name)

            elif isinstance(state, RlStateCompatStr):
                compat = state.compatstr
                out.write(compat)

            elif isinstance(state, RlStateDTVendor):
                out.write(f"{state.prefix}  {state.vendor}")

            elif isinstance(state, RlStateDTBus):
                proto = state.proto
                out.write(proto)

            elif isinstance(state, RlStateDTAlias):
                out.write(state.alias)

            elif isinstance(state, RlStateDTChosen):
                out.write(state.chosen)

            elif isinstance(state, RlStateFsEntry):
                dirent = state.dirent
                if dirent.is_dir():
                    out.write(f"{dirent.name}{os.path.sep}")
                else:
                    out.write(dirent.name)

            elif isinstance(state, RlStateEnum):
                out.write(f"{state.value}  {state.brief}")

            else:
                out.write(state.rlstr)
