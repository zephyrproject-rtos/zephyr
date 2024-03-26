# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# Copyright (c) 2018 Open Source Foundries Limited.
#
# SPDX-License-Identifier: Apache-2.0

"""Devicetree source definition.

A devicetree is fully defined by:

- a DTS file in Devicetree source format (DTSpec 6)
- all of the YAML binding files the DTS recursively depends on
- the Devicetree Specification

This module may rely on cached CMake variables to locate
the binding files the DTS file was actually generated with.
The herein CMake cache reader implementation is adapted from
the zcmake.py module in zephyr/scripts/west_commands.

This module eventually introduces a YAML file system API
that should cover the devicetree shell needs:

- access the DT binding files with their base name
- recursively access YAML-included bindings
- the name2path semantic expected for edtlib.Binding objects initialization

Unit tests and examples: tests/test_dtsh_dts.py
"""


from typing import (
    cast,
    Optional,
    Union,
    List,
    Sequence,
    Dict,
    Iterator,
    Mapping,
)

import os
import re
import sys

import yaml

# Custom PyYAML loader with support for the legacy '!include syntax.
from devicetree.edtlib import _BindingLoader as YAMLBindingLoader


class DTS:
    """Devicetree source definition.

    A devicetree source is defined by:

    - a DTS file in Devicetree source format (DTSpec 6)
    - all of the YAML binding files the DTS recursively depends on;
      the list of directories to search for the YAML files is
      termed "bindings search path"

    When not explicitly set, a default bindings search path
    can be retrieved, or worked out, based on:

    - cached CMake variables: this is the preferred method,
      assuming the cache file was produced by the same build as the DTS file
    - environment variables: less reliable than the CMake cache,
      since their *current* values are not bound to the build that
      produced the DTS file
    - assumptions on files layout, e.g. the CMake cache file is expected
      to be named CMakeCache.txt, and located in the grand-parent directory
      of where the DTS file itself is located (as shown bellow)

    The CMake cache file is searched for according to the typical
    Zephyr build layout:

        build/
        ├── CMakeCache.txt
        └── zephyr/
            └── zephyr.dts

    """

    # See path().
    _dts_path: str

    # See bindings_search_path().
    _binding_dirs: List[str]

    # See zephyr_base().
    _zephyr_base: Optional[str]

    # See vendors_file().
    _vendors_file: Optional[str]

    _cmake: Optional["CMakeCache"]
    _yamlfs: "YAMLFilesystem"

    def __init__(
        self,
        dts_path: str,
        binding_dirs: Optional[Sequence[str]] = None,
        vendors_file: Optional[str] = None,
    ) -> None:
        """Define a devicetree source.

        Args:
            dts_path: Path to the devicetree source file.
            binding_dirs: The list of directories to search for the
              YAML binding files the DTS depends on.
              If not set, the default bindings search path is assumed.
            vendors_file: Path to a file in vendor-prefixes.txt format.
        """
        self._dts_path = os.path.abspath(dts_path)
        self._cmake = self._init_cmake_cache()
        self._zephyr_base = self._init_zephyr_base()
        self._vendors_file = self._init_vendors_file(vendors_file)
        self._binding_dirs = self._init_binding_dirs(binding_dirs)
        self._yamlfs = YAMLFilesystem(self._binding_dirs)

    @property
    def path(self) -> str:
        """Path to the DTS file."""
        return self._dts_path

    @property
    def vendors_file(self) -> Optional[str]:
        "Path to the vendors file." ""
        return self._vendors_file

    @property
    def bindings_search_path(self) -> Sequence[str]:
        """Directories where the YAML binding files are located.

        Either:

        - set explicitly when defining the DT source
        - or retrieved from the CMake cached variable CACHED_DTS_ROOT_BINDINGS
        - or worked out (*best effort*) according to "Where bindings are located"

        In the later case, depending on defined environment variables
        or cached CMake variables, the bindings search path should contain
        the dts/bindings sub-directories of:

        - the zephyr repository
        - the application source directory
        - any custom board directory
        - shield directories

        This search path is not expunged from non existing directories:
        it's intended to represent the *considered* directories.

        Refer to zephyr/cmake/modules/dts.cmake for the DTS_ROOT
        and DTS_ROOT_BINDINGS values set at build-time:

            list(APPEND
              DTS_ROOT
              ${APPLICATION_SOURCE_DIR}
              ${BOARD_DIR}
              ${SHIELD_DIRS}
              ${ZEPHYR_BASE}
              )

            foreach(dts_root ${DTS_ROOT})
              set(bindings_path ${dts_root}/dts/bindings)
              if(EXISTS ${bindings_path})
                list(APPEND
                  DTS_ROOT_BINDINGS
                  ${bindings_path}
                  )
              endif()

        """
        return self._binding_dirs

    @property
    def yamlfs(self) -> "YAMLFilesystem":
        """YAML bindings search support."""
        return self._yamlfs

    @property
    def app_binary_dir(self) -> str:
        """Application binary directory (aka build directory).

        Derived from the DTS file path.
        """
        return os.path.dirname(os.path.dirname(self._dts_path))

    @property
    def app_source_dir(self) -> Optional[str]:
        """Application source directory (aka project directory).

        Either retrieved from the CMake cache (APPLICATION_SOURCE_DIR),
        or derived from the application binary directory.
        """
        app_src_dir = None
        if self._cmake:
            app_src_dir = self._cmake.getstr("APPLICATION_SOURCE_DIR")
        if not app_src_dir:
            app_src_dir = os.path.dirname(self.app_binary_dir)
        return app_src_dir

    @property
    def board_dir(self) -> Optional[str]:
        """Board directory.

        Retrieved from the CMake cache (BOARD_DIR).
        """
        return self._cmake.getstr("BOARD_DIR") if self._cmake else None

    @property
    def board(self) -> Optional[str]:
        """Board name.

        Retrieved from the CMake cache (BOARD, CACHED_BOARD).
        """
        board = None
        if self._cmake:
            board = self._cmake.getstr("BOARD")
            if not board:
                board = self._cmake.getstr("CACHED_BOARD")
        return board

    @property
    def board_file(self) -> Optional[str]:
        """Board DTS file.

        Shortcut to "${BOARD_DIR}/${BOARD}.dts".
        """
        if self.board_dir and self.board:
            return os.path.join(self.board_dir, f"{self.board}.dts")
        return None

    @property
    def shield_dirs(self) -> Sequence[str]:
        """Shield directories.

        Retrieved from the CMake cache (SHIELD_DIRS, CACHED_SHIELD_DIRS).
        """
        shield_dirs = []
        if self._cmake:
            shield_dirs = self._cmake.getstrs("SHIELD_DIRS")
            if not shield_dirs:
                shield_dirs = self._cmake.getstrs("CACHED_SHIELD_DIRS")
        return shield_dirs

    @property
    def fw_name(self) -> Optional[str]:
        """Application name.

        Retrieved from the CMake cache (CMAKE_PROJECT_NAME).
        """
        if self._cmake:
            return self._cmake.getstr("CMAKE_PROJECT_NAME")
        return None

    @property
    def fw_version(self) -> Optional[str]:
        """Application version.

        Retrieved from the CMake cache (CMAKE_PROJECT_VERSION).
        """
        if self._cmake:
            return self._cmake.getstr("CMAKE_PROJECT_VERSION")
        return None

    @property
    def zephyr_base(self) -> Optional[str]:
        """Path to Zephyr repository.

        Either:

        - retrieved from the CMake cache (ZEPHYR_BASE)
        - or retrieved from the shell environment (ZEPHYR_BASE)
        - or set according the expected file layout ZEPHYR_BASE/scripts/dts/dtsh
        - or unvailable
        """
        return self._zephyr_base

    @property
    def zephyr_sdk_dir(self) -> Optional[str]:
        """Path to Zephyr SDK.

        Either retrieved from the CMake cache or the shell
        environment (ZEPHYR_SDK_INSTALL_DIR).
        """
        zephyr_sdk_dir = None
        if self._cmake:
            zephyr_sdk_dir = self._cmake.getstr("ZEPHYR_SDK_INSTALL_DIR")
        if not zephyr_sdk_dir:
            zephyr_sdk_dir = os.environ.get("ZEPHYR_SDK_INSTALL_DIR")
        return zephyr_sdk_dir

    @property
    def toolchain_variant(self) -> Optional[str]:
        """Zephyr build toolchain variant.

        Either retrieved from the CMake cache or the shell
        environment (ZEPHYR_TOOLCHAIN_VARIANT).
        """
        toolchain_variant = None
        if self._cmake:
            toolchain_variant = self._cmake.getstr("ZEPHYR_TOOLCHAIN_VARIANT")
        if not toolchain_variant:
            toolchain_variant = os.environ.get("ZEPHYR_TOOLCHAIN_VARIANT")
        return toolchain_variant

    @property
    def toolchain_dir(self) -> Optional[str]:
        """Path to build toolchain.

        Depends on toolchain variant:

        - "zephyr": path to the Zephyr SDK
        - other variants: build system variable {TOOLCHAIN}_TOOLCHAIN_PATH
          (CMake cache or environment)
        """
        toolchain_dir = None
        if self.toolchain_variant:
            if self.toolchain_variant == "zephyr":
                toolchain_dir = self.zephyr_sdk_dir
            else:
                var = f"{self.toolchain_variant.upper()}_TOOLCHAIN_PATH"
                if self._cmake:
                    toolchain_dir = self._cmake.getstr(var)
                if not toolchain_dir:
                    toolchain_dir = os.environ.get(var)
        return toolchain_dir

    def _init_cmake_cache(self) -> Optional["CMakeCache"]:
        # Is the CMake cache available at ../CMakeCache.txt from the DTS file ?
        path = os.path.join(self.app_binary_dir, "CMakeCache.txt")
        if os.path.isfile(path):
            return CMakeCache.open(path)
        return None

    def _init_zephyr_base(self) -> Optional[str]:
        if self._cmake:
            zephyr_base = self._cmake.getstr("ZEPHYR_BASE")
        else:
            zephyr_base = os.environ.get("ZEPHYR_BASE")
        if not zephyr_base:
            # dtsh/src/dtsh/__file__
            dtsh_base = os.path.dirname(
                os.path.dirname(os.path.dirname(__file__))
            )
            # ZEPHYR_BASE/scripts/dts/dtsh
            zephyr_base = os.path.dirname(
                os.path.dirname(os.path.dirname(dtsh_base))
            )
        return zephyr_base

    def _init_vendors_file(self, vendors_file: Optional[str]) -> Optional[str]:
        if (not vendors_file) and self._zephyr_base:
            vendors_file = os.path.join(
                self._zephyr_base, "dts", "bindings", "vendor-prefixes.txt"
            )
        return vendors_file

    def _init_binding_dirs(
        self, binding_dirs: Optional[Sequence[str]]
    ) -> List[str]:
        if binding_dirs:
            binding_dirs = [os.path.abspath(path) for path in binding_dirs]
        else:
            if self._cmake:
                binding_dirs = self._cmake.getstrs("CACHED_DTS_ROOT_BINDINGS")
            if not binding_dirs:
                # Fallback to "Where bindings are located".
                # NOTE: Possible missing DTS roots ?
                #
                # - any directories manually included in the
                #   DTS_ROOT CMake variable
                # - any module that defines a dts_root in its Build settings
                #
                # IIUC, this might be a non-issue, since either the CMake cache:
                # - is available, and we should be able to get all bindings
                #   from the CACHED_DTS_ROOT_BINDINGS value
                # - is unavailable, and we won't access any build settings
                dts_roots: List[Optional[str]] = [
                    self.app_source_dir,
                    self.board_dir,
                    *self.shield_dirs,
                    self.zephyr_base,
                ]
                binding_dirs = [
                    os.path.join(dtsroot, "dts", "bindings")
                    for dtsroot in dts_roots
                    if dtsroot
                ]
        # cast() is required to avoid type hinting error since
        # binding_dirs is first typed as an optional Sequence.
        return cast(List[str], binding_dirs)


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

    def find_file(self, name: str) -> Optional["YAMLFile"]:
        """Find a YAML file by name.

        Args:
            name: The base name of a YAML file.

        Returns:
            A wrapper to the requested YAML file,
            or None if not found.
        """
        path = self.find_path((name))
        return YAMLFile(path) if path else None


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


class YAMLFile:
    """Cheap wrapper around a YAML file.

    This API is:

    - lazy-initialized: properties are initialized when accessed
    - fail-safe: errors are logged once to stderr, and empty values are returned
    """

    # Absolute file path.
    _path: str

    # Lazy-initialized file content.
    _content: Optional[str]

    # Lazy-initialized YAML model.
    _raw: Optional[Dict[str, object]]

    # Lazy-initialized YAML "include: ".
    _includes: Optional[List[str]]

    def __init__(self, path: str) -> None:
        """Lazy-initialize wrapper.

        Only the YAML file path is set upon initialization.

        Then accessing:

        - the *includes* will require initializing the YAML model
        - the YAML model will require loading the YAML file content

        Args:
            path: Absolute path to a YAML file.
        """
        self._path = path
        # Lazy-initialized.
        self._content = None
        self._raw = None
        self._includes = None

    @property
    def path(self) -> str:
        """Absolute file path."""
        return self._path

    @property
    def content(self) -> str:
        """YAML file content."""
        # Will Initialize an empty content if fails to load the YAML file.
        self._init_content()
        return self._content  # type: ignore

    @property
    def raw(self) -> Dict[str, object]:
        """YAML model."""
        # Will Initialize an empty model if the YAML file content unavailable.
        self._init_model()
        return self._raw  # type: ignore

    @property
    def includes(self) -> Sequence[str]:
        """Names of included YAML files."""
        # Will Initialize an empty list if the YAML model is unavailable.
        self._init_includes()
        return self._includes  # type: ignore

    def _init_content(self) -> None:
        if self._content is not None:
            return
        # Only one attempt to initialize content.
        self._content = ""

        try:
            with open(self._path, mode="r", encoding="utf-8") as f:
                self._content = f.read().strip()
        except OSError as e:
            print(f"YAML: {e}", file=sys.stderr)

    def _init_model(self) -> None:
        if self._raw is not None:
            return
        # Only one attempt to initialize model.
        self._raw = {}

        # Depends on YAML content.
        self._init_content()
        if not self._content:
            return

        try:
            self._raw = yaml.load(self._content, Loader=YAMLBindingLoader)
        except yaml.YAMLError as e:
            print(f"YAML: {self._path}: {e}", file=sys.stderr)

    def _init_includes(self) -> None:
        if self._includes is not None:
            return
        # Only one attempt to initialize includes.
        self._includes = []

        # Depends on YAML model.
        self._init_model()
        if not self._raw:
            return

        # See edtlib.Binding._merge_includes()
        yaml_inc = self._raw.get("include")
        if isinstance(yaml_inc, str):
            self._includes.append(yaml_inc)
        elif isinstance(yaml_inc, list):
            for inc in yaml_inc:
                if isinstance(inc, str):
                    self._includes.append(inc)
                elif isinstance(inc, dict):
                    basename = inc.get("name")
                    if basename:
                        self._includes.append(basename)
