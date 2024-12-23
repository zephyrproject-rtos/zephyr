# Copyright (c) 2024 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Misc utils.

The herein CMake cache reader implementation is adapted
from the zcmake.py module in zephyr/scripts/west_commands.
"""

from collections import defaultdict
from typing import (
    Any,
    Optional,
    Union,
    List,
    Sequence,
    Dict,
    Mapping,
    Iterator,
    Tuple,
)

from pathlib import Path

import os
import re
import subprocess
import sys
import yaml

# NOTE: not sure we still need to support the legacy "!include",
# not seen since at least Zephyr 3.6.
# Are there out-of-tree bindings relying on this ?
from devicetree.edtlib import _BindingLoader


class YAMLInclude:
    """YAML 'include:' entry."""

    _name: str
    _allow_list: Optional[List[str]]
    _block_list: Optional[List[str]]

    def __init__(
        self,
        name: str,
        allow_list: Optional[List[str]] = None,
        block_list: Optional[List[str]] = None,
    ) -> None:
        self._name = name
        self._allow_list = allow_list
        self._block_list = block_list

    @property
    def name(self) -> str:
        """Base file name."""
        return self._name

    def is_filtered_out(self, prop: str) -> bool:
        """Whether this YAML 'include:' masks a property."""
        if self._allow_list:
            return prop not in self._allow_list
        if self._block_list:
            return prop in self._block_list
        return False


class YAMLFile:
    """Cheap wrapper around a YAML file.

    Rationale: simple API to access a YAML file's content
    and its "include:"ed element, if any.

    Each DTS binding will instantiate such a YAML file,
    and there will be a lot of binding instances:
    - this API is lazy-initialized: the file is opened and red when
      the content() property is first accessed, and the content is parsed
      into YAML when properties raw() or includes() are accessed
    - since client code won't expect errors when simply accessing properties,
      this API won't fail: property will have empty values,
      and lasterr() will represent the last error (OS or YAML), if any
    """

    # Absolute file path.
    _path: Path

    # Lazy-initialized file content.
    _content: Optional[str]

    # Lazy-initialized YAML model.
    _raw: Optional[Dict[str, Any]]

    # Lazy-initialized YAML "include:",
    # sorted by child-binding depth of inclusion (i.e. top-level,
    # child-binding, grandchild-binding, etc).
    _depth2included: Optional[Dict[int, List[YAMLInclude]]]

    # If set, we've failed to load the YAML file at some point:
    # - OSError: all kinds of file system errors
    # - YAMLError: invalid YAML content
    _lasterr: Optional[Union[OSError, yaml.YAMLError]]

    def __init__(self, path: Union[str, Path]) -> None:
        """Lazy-initialize wrapper.

        Args:
            path: Absolute path to the YAML file.
              Invalid path or file will produce an empty content.
        """
        if isinstance(path, str):
            self._path = Path(path)
        else:
            self._path = path
        # Lazy-initialized.
        self._lasterr = None
        self._content = None
        self._raw = None
        self._depth2included = None

    @property
    def path(self) -> Path:
        """Absolute file path."""
        return self._path

    @property
    def content(self) -> str:
        """Text content."""
        self._init_content()
        return self._content or ""

    @property
    def raw(self) -> Dict[str, Any]:
        """YAML model.

        If empty, see lasterr().
        """
        self._init_model()
        return self._raw or {}

    @property
    def includes(self) -> Sequence[str]:
        """Names of all included YAML files."""
        self._init_includes()
        if not self._depth2included:
            return []
        yaml_includes: List[YAMLInclude] = [
            yaml_inc
            for included in self._depth2included.values()
            for yaml_inc in included
        ]
        return [yaml_inc.name for yaml_inc in yaml_includes]

    @property
    def lasterr(self) -> Optional[Union[OSError, yaml.YAMLError]]:
        """Last error that happened while loading this YAML file.

        Possible values:

        - None: no error
        - OSError: IO errors
        - YAMLError: invalid YAML content
        """
        return self._lasterr

    def includes_at_depth(self, cb_depth: int) -> List[YAMLInclude]:
        """Get the YAML files included at a given child-binding depth.

        Args:
            cb_depth: Child-binding depth (0 means top-level "include:",
                1 means child-binding, etc).

        Returns:
            A list of YAML file names.
        """
        self._init_includes()
        return self._depth2included[cb_depth] if self._depth2included else []

    def _init_content(self) -> None:
        # Actually try top open the YAML file and read its content.
        # Set lasterr accordingly.

        if self._content is not None:
            return
        # Only one attempt to initialize content.
        self._content = ""

        try:
            with open(self._path, mode="r", encoding="utf-8") as f:
                self._content = f.read().strip()
        except OSError as e:
            self._lasterr = e

    def _init_model(self) -> None:
        # Actually try to parse the file's content into YAML.
        # Set lasterr accordingly.

        if self._raw is not None:
            return
        # Only one attempt to initialize YAML model.
        self._raw = {}

        # Depends on file's content.
        self._init_content()
        if not self._content:
            return

        try:
            self._raw = yaml.load(self._content, Loader=_BindingLoader)
        except yaml.YAMLError as e:
            self._lasterr = e

    def _init_includes(self) -> None:
        # Search YAML model for include directives.

        if self._depth2included is not None:
            return
        # Only one attempt to initialize includes.
        self._depth2included = defaultdict(list)

        # Depends on YAML model.
        self._init_model()
        if not self._raw:
            return

        yaml_inc = self._raw.get("include")
        if yaml_inc:
            self._add_yaml_include(yaml_inc, 0)

        cb_depth: int = 0
        child_binding = self._raw.get("child-binding")
        while isinstance(child_binding, dict):
            cb_depth += 1
            yaml_inc = child_binding.get("include")
            if yaml_inc:
                self._add_yaml_include(yaml_inc, cb_depth)
            child_binding = child_binding.get("child-binding")

    def _add_yaml_include(self, yaml_inc: Any, cb_depth: int) -> None:
        # yaml_inc: YAML "include:" entry, string or list of intermixed
        # strings and maps. Maps may set property filters.
        if self._depth2included is None:
            # Should not happen: called to early?
            return

        if isinstance(yaml_inc, str):
            # The "include:" entry is the file name as string.
            self._depth2included[cb_depth].append(YAMLInclude(yaml_inc))
        elif isinstance(yaml_inc, list):
            # List of intermixed strings and maps.
            for inc in yaml_inc:
                if isinstance(inc, str):
                    self._depth2included[cb_depth].append(YAMLInclude(inc))
                elif isinstance(inc, dict):
                    basename = inc.get("name")
                    if isinstance(basename, str):
                        allowlist: List[str] = [
                            str(p) for p in inc.get("property-allowlist", [])
                        ]
                        blocklist: List[str] = [
                            str(p) for p in inc.get("property-blocklist", [])
                        ]
                        self._depth2included[cb_depth].append(
                            YAMLInclude(basename, allowlist, blocklist)
                        )


class PropertyLineage:
    """Property specifications lineage."""

    fyaml_last: Optional[YAMLFile] = None
    fyaml_spec: Optional[YAMLFile] = None

    def __init__(self) -> None:
        self.reset()

    def reset(self) -> None:
        """Commodity for clearing found lineage."""
        self.fyaml_last = None
        self.fyaml_spec = None

    def is_complete(self) -> bool:
        """Lineage complete."""
        return (self.fyaml_last is not None) and (self.fyaml_spec is not None)

    def __eq__(self, other: Any) -> bool:
        if not isinstance(other, PropertyLineage):
            return False
        return (self.fyaml_last == other.fyaml_last) and (
            self.fyaml_spec == other.fyaml_spec
        )


class YAMLFilesystem:
    """Find YAML files within a fixed search path (set of directories).

    Rationale:

    - retrieve YAML files with their base name
    - provide the name2path semantic expected for edtlib.Binding
      objects initialization
    """

    _name2path: Dict[str, str]

    def __init__(self, yaml_dirs: Sequence[str]) -> None:
        """Initialize the YAML file system.

        Args:
            yaml_dirs: The YAML search path as a set of absolute or relative
              directory paths.
        """
        self._name2path = {}
        for yaml_dir in [os.path.abspath(path) for path in yaml_dirs]:
            for root, _, basenames in os.walk(yaml_dir):
                for name in basenames:
                    if name.endswith((".yaml", ".yml")):
                        self._name2path[name] = os.path.join(root, name)

    @property
    def name2path(self) -> Mapping[str, str]:
        """Mapping from YAML base names to absolute file paths.

        This mapping contains all YAML files within this file system.
        """
        return self._name2path

    def find_path(self, name: str) -> Optional[str]:
        """Find a YAML file by name.

        Args:
            name: The base name of a YAML file.

        Returns:
            The absolute path to the requested YAML file,
            or None if not found.
        """
        return self._name2path.get(name)

    def find_file(self, name: str) -> Optional[YAMLFile]:
        """Find a YAML file by name.

        Args:
            name: The base name of a YAML file.

        Returns:
            A wrapper to the requested YAML file,
            or None if not found.
        """
        path = self.find_path(name)
        return YAMLFile(path) if path else None

    def find_property(
        self, name: str, fyaml: YAMLFile, cb_depth: int
    ) -> Optional[YAMLFile]:
        """Find where the property was last modified.

        Starting from the given "top-level" YAML file,
        down to the recursively included files,
        stop at the first binding file that adds some definition
        (e.g. "required: true") to a property with the given name.
        This file is where we assume the property was last modified.

        This is a workaround for issue #5.

        Note that this API has no notion of what a binding is,
        other than that the YAML document may contain (nested)
        "child-binding" and "properties:" keys.

        Args:
            name: The property name.
            fyaml: Start from this YAML file.
            cb_depth: Child-binding depth at which we'll search
                for the property.

        Returns:
            The YAML file where the property was last modified,
            or None if not found.
        """
        if self._fyaml_get_property(name, fyaml, cb_depth):
            return fyaml

        for depth in range(cb_depth + 1):
            includes = fyaml.includes_at_depth(depth)
            for yaml_inc in includes:
                if not yaml_inc.is_filtered_out(name):
                    fyaml_inc = self.find_file(yaml_inc.name)
                    if fyaml_inc:
                        fyaml_found = self.find_property(
                            name, fyaml_inc, cb_depth - depth
                        )
                        if fyaml_found:
                            return fyaml_found
        return None

    def backtrack_property(
        self,
        lineage: PropertyLineage,
        name: str,
        fyaml: YAMLFile,
        cb_depth: int,
    ) -> None:
        """Backtrack a property lineage.

        Similar to find_property() but won't stop at the first YAML file
        where we find the property, and also search for a YAML file where
        the property has a "description:".
        This file is where we assume the property is first specified.

        Args:
            lineage: Empty property lineage, will hold search results on return.
            name: The property name.
            fyaml: Start from this YAML file.
            cb_depth: Child-binding depth at which we'll search
                for the property.
        """
        raw: Optional[Dict[Any, Any]] = self._fyaml_get_property(
            name, fyaml, cb_depth
        )
        if raw:
            if not lineage.fyaml_last:
                lineage.fyaml_last = fyaml
            if "description" in raw:
                lineage.fyaml_spec = fyaml
            if lineage.is_complete():
                return

        for depth in range(cb_depth + 1):
            includes = fyaml.includes_at_depth(depth)
            for yaml_inc in includes:
                if not yaml_inc.is_filtered_out(name):
                    fyaml_inc = self.find_file(yaml_inc.name)
                    if fyaml_inc:
                        self.backtrack_property(
                            lineage, name, fyaml_inc, cb_depth - depth
                        )

    def _fyaml_get_property(
        self, name: str, fyaml: YAMLFile, cb_depth: int
    ) -> Optional[Dict[Any, Any]]:
        raw: Dict[Any, Any] = fyaml.raw
        for _ in range(cb_depth):
            raw = raw.get("child-binding", {})
        prop = raw.get("properties", {}).get(name)
        return prop if isinstance(prop, dict) else None


class CMakeCache:
    """CMake cache reader.

    Adapted from zcmake.CMakeCache.
    """

    _entries: Dict[str, "CMakeCacheEntry"]

    @classmethod
    def open(cls, path: str) -> Optional["CMakeCache"]:
        """Open a CMake cache file for reading.

        Args:
            path: Path to the CMake cache file (CMakeCache.txt) to open.

        Returns:
            The CMakeCache content.
        """
        try:
            return CMakeCache(path)
        except OSError as e:
            print(f"CMakeCache file error: {e}", file=sys.stderr)
        except ValueError as e:
            print(f"CMakeCache content error: {e}", file=sys.stderr)
        return None

    def __init__(self, path: str) -> None:
        """Open a CMake cache file.

        Args:
            path: Path to the CMake cache file (CMakeCache.txt) to open.

        Raises:
            OSError: CMakeCache file error.
            ValueError: CMakeCache content error.
        """
        with open(path, "r", encoding="utf-8") as cache:
            entries = [
                CMakeCacheEntry.from_line(line, line_no)
                for line_no, line in enumerate(cache)
            ]
        self._entries = {entry.name: entry for entry in entries if entry}

    def get(self, name: str) -> Optional["CMakeCacheEntry.ValueType"]:
        """Access a cache entry by name.

        Arg:
            name: Cache entry name.

        Returns:
            The raw cache entry value, or None if not set.
        """
        if name in self._entries:
            return self._entries[name].value
        return None

    def getbool(self, name: str) -> bool:
        """Access a cache entry as boolean.

        Arg:
            name: Cache entry name.

        Returns:
            True if the entry is of type boolean and set ("ON", "YES", etc),
            false otherwise.
        """
        val = self.get(name)
        # May not be Pythonic, but is consistent with type hinting.
        return val is True

    def getstr(self, name: str) -> Optional[str]:
        """Access a cache entry as string.

        Arg:
            name: Cache entry name.

        Returns:
            A string value if the entry exists and is actually of type string,
            None otherwise.
        """
        val = self.get(name)
        if val and isinstance(val, str):
            return val
        return None

    def getstrs(self, name: str) -> List[str]:
        """Access a cache entry as a list of strings.

        Arg:
            name: Cache entry name.

        Returns:
            A list of string values if the entry exists and is either of type
            string or of type list of strings, an empty list otherwise.
        """
        val = self.get(name)
        if val and isinstance(val, str):
            return [val]
        if isinstance(val, list):
            # Assuming list of string.
            return val
        return []

    def __contains__(self, name: str) -> bool:
        """Map protocol."""
        return name in self._entries

    def __getitem__(self, name: str) -> "CMakeCacheEntry.ValueType":
        """Map protocol."""
        return self._entries[name].value

    def __iter__(self) -> Iterator[str]:
        """Iterate on the CMake cache entries."""
        return iter(self._entries.keys())

    def __len__(self) -> int:
        """Number of entries in this CMake cache."""
        return len(self._entries)


class CMakeCacheEntry:
    """CMake cache entry.

    This class understands the type system in a CMakeCache.txt, and
    converts the following cache types to Python types:

        Cache Type    Python type
        ----------    -------------------------------------------
        FILEPATH      str
        PATH          str
        STRING        str OR list of str (if ';' is in the value)
        BOOL          bool
        INTERNAL      str OR list of str (if ';' is in the value)
        STATIC        str OR list of str (if ';' is in the value)
        UNINITIALIZED str OR list of str (if ';' is in the value)
        ----------    -------------------------------------------

    Adapted from zcmake.CMakeCacheEntry.
    """

    # Regular expression for a cache entry.
    #
    # CMake variable names can include escape characters, allowing a
    # wider set of names than is easy to match with a regular
    # expression. To be permissive here, use a non-greedy match up to
    # the first colon (':'). This breaks if the variable name has a
    # colon inside, but it's good enough.
    CACHE_ENTRY = re.compile(
        r"""(?P<name>.*?)
         :(?P<type>FILEPATH|PATH|STRING|BOOL|INTERNAL|STATIC|UNINITIALIZED)
         =(?P<value>.*)
        """,
        re.X,
    )

    ValueType = Union[str, List[str], bool]

    _name: str
    _value: "CMakeCacheEntry.ValueType"

    @classmethod
    def from_line(cls, line: str, line_no: int) -> Optional["CMakeCacheEntry"]:
        """Create an entry from a cache line.

        Args:
            line: The cache line content.
            line_no: The cache file line number (used for error reporting).

        Returns:
            A cache entry or `None` if the line was a comment, empty,
            or malformed.

        Raises:
            ValueError: Failed conversion to bool.
        """
        # Comments can only occur at the beginning of a line.
        # (The value of an entry could contain a comment character).
        if line.startswith("//") or line.startswith("#"):
            return None

        # Whitespace-only lines do not contain cache entries.
        if not line.strip():
            return None

        m = cls.CACHE_ENTRY.match(line)
        if not m:
            return None

        name, type_, value = (m.group(g) for g in ("name", "type", "value"))
        if type_ == "BOOL":
            try:
                value = cls._to_bool(value)
            except ValueError as exc:
                args = exc.args + (f"on line {line_no}: {line}",)
                raise ValueError(args) from exc
        elif type_ in {"STRING", "INTERNAL", "STATIC", "UNINITIALIZED"}:
            # If the value is a CMake list (i.e. is a string which
            # contains a ';'), convert to a Python list.
            if ";" in value:
                value = value.split(";")

        return CMakeCacheEntry(name, value)

    @classmethod
    def _to_bool(cls, val: str) -> bool:
        # Convert a CMake BOOL string into a Python bool.
        #
        #   "True if the constant is 1, ON, YES, TRUE, Y, or a
        #   non-zero number. False if the constant is 0, OFF, NO,
        #   FALSE, N, IGNORE, NOTFOUND, the empty string, or ends in
        #   the suffix -NOTFOUND. Named boolean constants are
        #   case-insensitive. If the argument is not one of these
        #   constants, it is treated as a variable."
        #
        # https://cmake.org/cmake/help/v3.0/command/if.html
        val = val.upper()
        if val in ("ON", "YES", "TRUE", "Y"):
            return True
        if val in ("OFF", "NO", "FALSE", "N", "IGNORE", "NOTFOUND", ""):
            return False
        if val.endswith("-NOTFOUND"):
            return False
        try:
            v = int(val)
            return v != 0
        except ValueError as e:
            raise ValueError(f"not a bool: {val}") from e

    def __init__(self, name: str, value: "CMakeCacheEntry.ValueType") -> None:
        """Initialize a new cache entry.

        Args:
            name: Entry name.
            value: Entry value.
        """
        self._name = name
        self._value = value

    @property
    def name(self) -> str:
        """Cache entry name."""
        return self._name

    @property
    def value(self) -> "CMakeCacheEntry.ValueType":
        """Cache entry raw value."""
        return self._value

    def __repr__(self) -> str:
        return f"{self._name}: {self._value}"


class GitUtil:
    """Git helper.

    Execute Git commands as sub-processes and get results.
    """

    # Working directory for Git commands
    _cwd: str

    # True when:
    # - the git command is found
    # - we're inside a Git working tree
    _enabled: bool

    def __init__(self, cwd: str) -> None:
        """
        Args:
           cwd: Working directory for Git commands.
        """
        self._cwd = cwd
        self._enabled = self._git_check_enabled()

    @property
    def is_available(self) -> bool:
        """True when we should be able to execute Git commands."""
        return self._enabled

    def head_get_tag(self) -> Optional[str]:
        """Get the tag of the repository HEAD, if any."""
        (ret, output) = self._git_exec(["describe", "--exact-match", "--tags"])
        if ret == 0:
            return output
        return None

    def head_get_short(self) -> Optional[str]:
        """Get the short commit hash of the repository HEAD."""
        (ret, output) = self._git_exec(["rev-parse", "--short", "HEAD"])
        if ret == 0:
            return output
        return None

    def _git_check_enabled(self) -> bool:
        (ret, _) = self._git_exec(["rev-parse", "--is-inside-work-tree"])
        return ret == 0

    # Execute a Git command and answer result as a tuple (return value, output).
    def _git_exec(self, args: Sequence[str]) -> Tuple[int, str]:
        git_cmd: Sequence[str] = [
            "git.exe" if os.name == "nt" else "git",
            *args,
        ]
        try:
            with subprocess.Popen(
                git_cmd,
                cwd=self._cwd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
            ) as proc:
                ret = proc.wait()
                # We know proc.stdout is set.
                output = proc.stdout.read().decode("utf-8").strip()  # type: ignore
                return (ret, output)
        except OSError as e:
            return (e.errno, e.strerror)


class DTShToolchain:
    """Facade to CMake cache variables that represent the toolchain
    used at build-time.
    """

    CMAKE_TOOLCHAIN_VARIANT = "ZEPHYR_TOOLCHAIN_VARIANT"
    CMAKE_ZEPHYR_SDK_PATH = "ZEPHYR_SDK_INSTALL_DIR"
    CMAKE_TOOLCHAIN_PATH_FMT = "{VARIANT}_TOOLCHAIN_PATH"

    ZEPHYR_SDK = "zephyr"
    GNU_ARM_EMB = "gnuarmemb"

    @staticmethod
    def get_instance(cmake_cache: CMakeCache) -> Optional["DTShToolchain"]:
        """Commodity for instantiating a board.

        Args:
            cmake_cache: The CMake cache to search.

        Returns:
            A board instance, or None if instantiation failed.
        """
        try:
            return DTShToolchain(cmake_cache)
        except KeyError as e:
            print(f"dubious CMake cache: {e}", file=sys.stderr)
        return None

    _variant: str
    _name: Optional[str] = None
    _path: Optional[Path] = None
    _release: Optional[str] = None

    def __init__(self, cmake_cache: CMakeCache) -> None:
        """Initialize toolchain metadata with CMake cache content.

        Args:
            cmake_cache: The CMake cache to search.
                MUST define at least ZEPHYR_TOOLCHAIN_VARIANT.
        """
        self._variant = self._init_variant(cmake_cache)
        if self._variant == DTShToolchain.ZEPHYR_SDK:
            self._init_toolchain_zephyr(cmake_cache)
        elif self._variant == DTShToolchain.GNU_ARM_EMB:
            self._init_toolchain_gnuarm(cmake_cache)
        else:
            self._init_toolchain_any(cmake_cache)

    def _init_variant(self, cmake_cache: CMakeCache) -> str:
        variant = cmake_cache.getstr(DTShToolchain.CMAKE_TOOLCHAIN_VARIANT)
        if variant:
            return variant
        raise KeyError(DTShToolchain.CMAKE_TOOLCHAIN_VARIANT)

    @property
    def variant(self) -> str:
        """Toolchain variant (ZEPHYR_TOOLCHAIN_VARIANT)."""
        return self._variant

    @property
    def path(self) -> Optional[Path]:
        """Toolchain path.
        Either ZEPHYR_SDK_INSTALL_DIR or ${VARIANT}_TOOLCHAIN_PATH.
        """
        return self._path

    @property
    def name(self) -> Optional[str]:
        """A toolchain name variant (zephyr, gnuarmemb only)."""
        return self._name

    @property
    def release(self) -> Optional[str]:
        """The toolchain release (zephyr, gnuarmemb only)."""
        return self._release

    def _init_toolchain_zephyr(self, cmake_cache: CMakeCache) -> None:
        self._name = "Zephyr SDK"
        self._path = self._get_zephyr_sdk_dir(cmake_cache)
        self._release = self._get_zephyr_sdk_release()

    def _init_toolchain_gnuarm(self, cmake_cache: CMakeCache) -> None:
        self._name = "Arm GNU Toolchain"
        self._path = self._get_toolchain_path(cmake_cache)
        self._release = self._get_gnuarm_release()

    def _init_toolchain_any(self, cmake_cache: CMakeCache) -> None:
        self._path = self._get_toolchain_path(cmake_cache)

    # Path to build toolchain.
    # Either retrieved from the CMake cache or the shell
    # environment (${TOOLCHAIN_VARIANT}_TOOLCHAIN_PATH).
    def _get_toolchain_path(self, cmake_cache: CMakeCache) -> Optional[Path]:
        var = DTShToolchain.CMAKE_TOOLCHAIN_PATH_FMT.format(
            VARIANT=self._variant.upper()
        )
        path: Optional[str] = cmake_cache.getstr(var)
        return Path(path) if path else None

    # Path to Zephyr SDK.
    # Either retrieved from the CMake cache or the shell
    # environment (ZEPHYR_SDK_INSTALL_DIR).
    def _get_zephyr_sdk_dir(self, cmake_cache: CMakeCache) -> Optional[Path]:
        zephyr_sdk_dir = cmake_cache.getstr(
            "ZEPHYR_SDK_INSTALL_DIR"
        ) or os.getenv("ZEPHYR_SDK_INSTALL_DIR")
        return Path(zephyr_sdk_dir) if zephyr_sdk_dir else None

    # Zephyr SDK version.
    # Retrieved from SDV version file.
    def _get_zephyr_sdk_release(self) -> Optional[str]:
        if not self._path:
            return None
        version_path: Path = self._path / "sdk_version"
        release: Optional[str] = None
        try:
            with open(version_path, mode="r", encoding="utf-8") as f:
                release = f.readline().strip()
        except OSError as e:
            print(f"dubious Zephyr SDK: {e}")
        return release

    def _get_gnuarm_release(self) -> Optional[str]:
        if not self._path:
            return None
        gcc_exe = (
            "arm-none-eabi-gcc.exe" if os.name == "nt" else "arm-none-eabi-gcc"
        )
        gcc_path = self._path / "bin" / gcc_exe
        gcc_cmd: Sequence[str] = (str(gcc_path), "--version")

        release: Optional[str] = None
        try:
            with subprocess.Popen(
                gcc_cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
            ) as proc:
                ret = proc.wait()
                if ret == 0:
                    output: List[str] = (
                        # We know proc.stdout is set.
                        proc.stdout.read()  # type: ignore[union-attr]
                        .decode("utf-8")
                        .splitlines()
                    )
                    if output:
                        release = output[0]
        except OSError as e:
            print(f"dubious Arm GNU: {e}")

        return release
