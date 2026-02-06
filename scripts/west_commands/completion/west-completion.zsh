#compdef west

# Copyright (c) 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Ensure this works also when being source-ed

compdef _west west
typeset -A -g _opt_args

_west_cmds() {
  local -a builtin_cmds=(
  'init[create a west workspace]'
  'update[update projects described in west manifest]'
  'list[print information about projects]'
  'manifest[manage the west manifest]'
  'diff["git diff" for one or more projects]'
  'compare[manifest-rev vs project/revision]'
  'status["git status" for one or more projects]'
  'grep["run grep or a grep-like tool in one or more local projects]'
  'forall[run a command in one or more local projects]'
  'help[get help for west or a command]'
  'config[get or set config file values]'
  'topdir[print the top level directory of the workspace]'
  )

  local -a zephyr_ext_cmds=(
  'completion[display shell completion scripts]'
  'boards[display information about supported boards]'
  'shields[display information about supported shields]'
  'build[compile a Zephyr application]'
  'sign[sign a Zephyr binary for bootloader chain-loading]'
  'flash[flash and run a binary on a board]'
  'debug[flash and interactively debug a Zephyr application]'
  'debugserver[connect to board and launch a debug server]'
  'attach[interactively debug a board]'
  'rtt[open an rtt shell]'
  'reset[reset the board to reboot]'
  'zephyr-export[export Zephyr installation as a CMake config package]'
  'spdx[create SPDX bill of materials]'
  'blobs[work with binary blobs]'
  'bindesc[work with Binary Descriptors]'
  'robot[run RobotFramework test suites]'
  'simulate[simulate a Zephyr application]'
  'sdk[manage SDKs]'
  'packages[manage packages for Zephyr]'
  'patch[manage patches for Zephyr modules]'
  'gtags[create a GNU global tags file for the current workspace]'
  'twister[run Twister test runner]'
  )

  local -a all_cmds=(${builtin_cmds} ${zephyr_ext_cmds})

  if [[ -v WEST_COMP_CHECK_WORKSPACE ]]; then
    west topdir &>/dev/null
    if [ $? -eq 0 ]; then
      _values "west command" $all_cmds
    else
      _values "west command" $builtin_cmds
    fi
  else
      _values "west command" $all_cmds
  fi
}

# Completion script for Zephyr's meta-tool, west
_west() {


  # Global options for all commands
  local -a global_opts=(
  # (: * -) as exclusion list means exclude everything else
  '(: * -)'{-h,--help}'[show help]'
  # An exclusion list with the very option means only allow once
  {-v,--verbose}'[enable verbosity]'
  '(: * -)'{-V,--version}'[print version]'
  '(-z --zephyr-base)'{-z,--zephyr-base}'[zephyr base folder]:zephyr base folder:_directories'
  )

  typeset -A opt_args
  local curcontext="$curcontext" context state state_descr line
  local -a orig_words

  orig_words=( ${words[@]} )

  _arguments -S -C \
    $global_opts \
          "1: :->cmds" \
          "*::arg:->args" \

  case "$state" in
  cmds)
    _west_cmds
    ;;

  args)
    _opt_args=( ${(@kv)opt_args} )
    _call_function ret _west_$line[1]
    ;;
  esac
}

__west_x()
{
  west 2>/dev/null "$@"
}

_get_west_projs() {
  local extra_args
  [[ -v _opt_args[-z] ]] && extra_args="-z $_opt_args[-z]"
  [[ -v _opt_args[--zephyr-base] ]] && extra_args="-z $_opt_args[--zephyr-base]"

  _west_projs=($(__west_x $extra_args list --format={name}))
  _describe 'projs' _west_projs
}

_get_west_boards() {
  local boards=( $(__west_x boards --format='{name}|{qualifiers}') )
  local -a transformed_boards

  for board_line in "${boards[@]}"; do
    local name="${board_line%%|*}"
    local transformed_board="${board_line//|//}"
    transformed_boards+=("${transformed_board//,/ ${name}/}")
  done

  _describe 'boards' transformed_boards
}

_get_west_shields() {
  _west_shields=( $(__west_x shields --format='{name}') )
  for i in {1..${#_west_shields[@]}}; do
    local name="${_west_shields[$i]%%|*}"
    local transformed_shield="${_west_shields[$i]//|//}"
    _west_shields[$i]="${transformed_shield//,/ ${name}/}"
  done
  _west_shields=(${(@s/ /)_west_shields})

  _describe 'shields' _west_shields
}
__west_tilde() {
    local cur=${words[CURRENT]}

    [[ $cur == "~"* && $cur != */* ]] || return 1

    compset -P '~'
    _users
}



_filedir()
{
    if [[ "$1" == "-d" ]]; then
        _files -/
    else
        _files
    fi
}


__comp_west_runner_cmd() {
  local -a opts=(
  # Boolean flags
  '(-H --context)'{-H,--context}'[print runner-specific options]'
  '--rebuild[reinvoke cmake]'
  '--no-rebuild[do not reinvoke cmake]'

  # Directory options
  '--board-dir[board directory]:board dir:_directories'
  '(-d --build-dir)'{-d,--build-dir}'[build directory]:build dir:_directories'
  '--openocd-search[openocd search path]:path:_directories'

  # File options
  '(-f --file)'{-f,--file}'[path to binary]:binary:_files'
  '(-t --file-type)'{-t,--file-type}'[type of binary]:type:(hex bin elf)'
  '--elf-file[path to zephyr.elf]:file:_files'
  '--hex-file[path to zephyr.hex]:file:_files'
  '--bin-file[path to zephyr.bin]:file:_files'
  '--gdb[path to GDB]:gdb:_files'
  '--openocd[path to openocd]:openocd:_files'

  # Other options
  '(-r --runner)'{-r,--runner}'[override runner]:runner:'
  '--domain[build domain]:domain:'
  '(-i --dev-id)'{-i,--dev-id}'[device id]:id:'
  )

  _arguments -S -C $opts
}

_west_init() {
  local -a opts=(
  '(-l --local)'{--mr,--manifest-rev}'[manifest revision]:manifest rev:'
  {--mf,--manifest-file}'[manifest file]:manifest file:'
  '(-l --local)'{-m,--manifest}'[manifest URL]:manifest URL:_directories'
  '(-m --manifest --mr --manifest-rev)'{-l,--local}'[use local directory as manifest repository]:local manifest repository directory:_directories'
  )

  _arguments -S $opts \
          "1:workspace directory:"
}

_west_update() {
  local -a opts=(
  '--stats[print performance stats]'
  '--name-cache[name-based cache]:name cache folder:_directories'
  '--path-cache[path-based cache]:path cache folder:_directories'
  {-f,--fetch}'[fetch strategy]:fetch strategy:(always smart)'
  {-o,--fetch-opt}'[fetch options]:fetch options:'
  {-n,--narrow}'[narrow fetch]'
  {-k,--keep-descendants}'[keep manifest-rev descendants checked out]'
  {-r,--rebase}'[rebase checked out branch onto the new manifest-rev]'
  )

  _arguments -S $opts \
      "1:west proj:_get_west_projs"
}

_west_list() {
  local -a opts=(
  {-a,--all}'[include inactive projects]'
  '--manifest-path-from-yaml[print performance stats]'
  {-f,--format}'[format string]:format string:'
  )

  _arguments -S $opts \
      "1:west proj:_get_west_projs"
}

_west_manifest() {
  local -a opts=(
  '--resolve[resolve into single manifest]'
  '--freeze[resolve into single manifest, with SHAs]'
  '--validate[silently validate manifest]'
  '--path[print the path to the top level manifest file]'
  {-o,--out}'[output file]:output file:_files'
  )

  _arguments -S $opts
}

_west_diff() {
  local -a opts=(
  {-a,--all}'[include inactive projects]'
  )

  _arguments -S $opts \
      "1:west proj:_get_west_projs"
}

_west_compare() {
  _west_diff
}

_west_status() {
  local -a opts=(
  {-a,--all}'[include inactive projects]'
  )

  _arguments -S $opts \
      "1:west proj:_get_west_projs"
}

_west_forall() {
  local -a opts=(
  '-c[command to execute]:command:'
  {-a,--all}'[include inactive projects]'
  )

  _arguments -S $opts \
      "1:west proj:_get_west_projs"
}

_west_config() {
  local -a opts=(
  {-l,--list}'[list all options and values]'
  {-d,--delete}'[delete an option in one config file]'
  {-D,--delete-all}"[delete an option everywhere it\'s set]"
  + '(mutex)'
  '--system[system-wide file]'
  '--global[global user-wide file]'
  "--local[this workspace\'s file]"
  )

  _arguments -S $opts
}

_west_help() {
  _west_cmds
}

_west_completion() {

  _arguments -S "1:shell:(bash zsh fish)"
}

_west_boards() {
  local -a opts=(
  {-f,--format}'[format string]:format string:'
  {-n,--name}'[name regex]:regex:'
  '*--arch-root[Add an arch root]:arch root:_directories'
  '*--board-root[Add a board root]:board root:_directories'
  '*--soc-root[Add a soc root]:soc root:_directories'
  )

  _arguments -S $opts
}

_west_shields() {
  local -a opts=(
  {-f,--format}'[format string]:format string:'
  {-n,--name}'[name regex]:regex:'
  '*--board-root[Add a board root]:board root:_directories'
  )

  _arguments -S $opts
}

_west_build() {
  local -a opts=(
  '(-b --board)'{-b,--board}'[board to build for]:board:_get_west_boards'
  '(-d --build-dir)'{-d,--build-dir}'[build directory to create or use]:build dir:_directories'
  '(-f --force)'{-f,--force}'[ignore errors and continue]'
  '--sysbuild[create multi-domain build system]'
  '--no-sysbuild[do not create multi-domain build system]'
  '(-c --cmake)'{-c,--cmake}'[force a cmake run]'
  '--cmake-only[just run cmake]'
  '--domain[execute build tool (make or ninja) for a given domain]:domain:'
  '(-t --target)'{-t,--target}'[run build system target]:target:'
  '(-T --test-item)'{-T,--test-item}'[Build based on test data in .yml]:test item:'
  {-o,--build-opt}'[options to pass to build tool (make or ninja)]:tool opt:'
  '(-n --just-print --dry-run --recon)'{-n,--just-print,--dry-run,--recon}"[just print build commands, don't run them]"
  '(-p --pristine)'{-p,--pristine}'[pristine build setting]:pristine:(auto always never)'
  '(-S --snippet)'{-S,--snippet}'[apply a snippet]:snippet:'
  '--shield[shield to build for]:shield:_get_west_shields'
  )
  _arguments -S $opts \
      "1:source_dir:_directories"
}

_west_sign() {
  local -a opts=(
  '(-d --build-dir)'{-d,--build-dir}'[build directory to create or use]:build dir:_directories'
  '(-q --quiet)'{-q,--quiet}'[suppress non-error output]'
  '(-f --force)'{-f,--force}'[ignore errors and continue]'
  '(-t --tool)'{-t,--tool}'[image signing tool name]:tool:(imgtool rimage)'
  '(-p --tool-path)'{-p,--tool-path}'[path to the tool]:tool path:_directories'
  '(-D --tool-data)'{-D,--tool-data}'[path to tool data]:tool data path:_directories'
  '(--no-bin)--bin[produce a signed bin file]'
  '(--bin)--no-bin[do not produce a signed bin file]'
  '(-B --sbin)'{-B,--sbin}'[signed .bin filename]:bin filename:_files'
  '(--no-hex)--hex[produce a signed hex file]'
  '(--hex)--no-hex[do not produce a signed hex file]'
  '(-H --shex)'{-H,--shex}'[signed .hex filename]:hex filename:_files'
  )

  _arguments -S $opts
}

typeset -a -g _west_runner_opts=(
  '(-H --context)'{-H,--context}'[print runner-specific options]'
  '--board-dir[board directory]:board dir:_directories'
  '(-f --file)'{-f,--file}'[path to binary]:path to binary:_files'
  '(-t --file-type)'{-t,--file-type}'[type of binary]:type of binary:(hex bin elf)'
  '--elf-file[path to zephyr.elf]:path to zephyr.elf:_files'
  '--hex-file[path to zephyr.hex]:path to zephyr.hex:_files'
  '--bin-file[path to zephyr.bin]:path to zephyr.bin:_files'
  '--gdb[path to GDB]:path to GDB:_files'
  '--openocd[path to openocd]:path to openocd:_files'
  '--openocd-search[path to add to openocd search path]:openocd search:_directories'
)

_west_flash() {
  local -a opts=(
  '(-d --build-dir)'{-d,--build-dir}'[build directory to create or use]:build dir:_directories'
  '(-r --runner)'{-r,--runner}'[override default runner from build-dir]:runner:'
  '--no-rebuild[manually specify to reinvoke cmake or not]'
  '--rebuild[manually specify to reinvoke cmake or not]'
  '--domain[execute build tool (make or ninja) for a given domain]:domain:'
  )

  local -a all_opts=(${_west_runner_opts} ${opts})
  _arguments -S -C $all_opts
}

_west_debug() {
  __comp_west_runner_cmd
}

_west_debugserver() {
  __comp_west_runner_cmd
}

_west_attach() {
  __comp_west_runner_cmd
}

_west_spdx() {
  local -a opts=(
  '(-i --init)'{-i,--init}'[initialize CMake file-based API]'
  '(-d --build-dir)'{-d,--build-dir}'[build directory to create or use]:build dir:_directories'
  '(-n --namespace-prefix)'{-n,--namespace-prefix}'[namespace prefix]:namespace prefix:'
  '(-s --spdx-dir)'{-s,--spdx-dir}'[SPDX output directory]:spdx output dir:_directories'
  '--analyze-includes[also analyze included header files]'
  '--include-sdk[also generate SPDX document for SDK]'
  )
  _arguments -S $opts
}

_west_blobs() {
  local -a blob_cmds=(
  'list[list binary blobs]'
  'fetch[fetch binary blobs]'
  'clean[clean working tree of binary blobs]'
  )

  local line state

  _arguments -S -C \
          "1: :->cmds" \
          "*::arg:->args" \

  case "$state" in
  cmds)
    _values "west blob cmds" $blob_cmds
    ;;

  args)
    _opt_args=( ${(@kv)opt_args} )
    _call_function ret _west_blob_$line[1]
    ;;
  esac
}

_west_blob_list () {
  local -a opts=(
  {-f,--format}'[format string]:format string:'
  )

  _arguments -S $opts \
      "1:west proj:_get_west_projs"
}

_west_blob_fetch () {
  _arguments -S "1:west proj:_get_west_projs"
}

_west_blob_clean () {
  _arguments -S "1:west proj:_get_west_projs"
}

_west_sdk() {
  local -a opts=(
  '(-d --install-dir)'{-d,--install-dir}'[installation directory]:dir:_directories'
  '(-b --install-base)'{-b,--install-base}'[installation base]:dir:_directories'
  '(-i --interactive)'{-i,--interactive}'[interactive installation]'
  '(-T --no-toolchains)'{-T,--no-toolchains}'[skip toolchains]'
  '(-H --no-hosttools)'{-H,--no-hosttools}'[skip hosttools]'
  '--version[SDK version]:version:'
  '--toolchains[toolchains]:toolchains:'
  '--personal-access-token[Github personal access token (PAT)]:token:'
  '--api-url[Github API URL]:url:'
  )

  _arguments -S $opts \
      "1:subcommand:(list install)"
}


_west_twister() {
  local -a opts=(
  # Boolean flags
  '(-b --build-only)'{-b,--build-only}'[only build tests, do not run]'
  '(-c --clobber-output)'{-c,--clobber-output}'[clobber output directory]'
  '(-l --all)'{-l,--all}'[run all tests]'
  '(-n --no-clean)'{-n,--no-clean}'[do not clean before building]'
  '(-v --verbose)'{-v,--verbose}'[verbose output]'
  '(-y --dry-run)'{-y,--dry-run}'[dry run, do not execute]'
  '(-f --only-failed)'{-f,--only-failed}'[run only failed tests]'
  '--cmake-only[only run cmake]'
  '--no-update[do not update test results]'
  '--inline-logs[inline test logs in output]'

  '--aggressive-no-clean[do not clean before building]'
  '(-D --all-deltas)'{-D,--all-deltas}'[show all deltas]'
  '--allow-installed-plugin[allow installed plugins]'
  '(-C --coverage)'{-C,--coverage}'[generate coverage report]'
  '--create-rom-ram-report[create ROM/RAM report]'
  '--detailed-skipped-report[detailed skipped report]'
  '--detailed-test-id[detailed test ID]'
  '--device-flash-with-test[flash device with test]'
  '--device-testing[enable device testing]'
  '--disable-suite-name-check[disable suite name check]'
  '--disable-unrecognized-section-test[disable unrecognized section test]'
  '(-W --disable-warnings-as-errors)'{-W,--disable-warnings-as-errors}'[do not treat warnings as errors]'
  '--emulation-only[only use emulation]'
  '--enable-asan[enable ASAN]'
  '--enable-coverage[enable coverage]'
  '--enable-lsan[enable LSAN]'
  '--enable-size-report[enable size report]'
  '(-S --enable-slow)'{-S,--enable-slow}'[enable slow tests]'
  '--enable-slow-only[only run slow tests]'
  '--enable-ubsan[enable UBSAN]'
  '--enable-valgrind[enable Valgrind]'
  '--flash-before[flash before running]'
  '--footprint-from-buildlog[footprint from buildlog]'
  '--force-color[force color output]'
  '(-K --force-platform)'{-K,--force-platform}'[force platform]:platform:_get_west_boards'
  '--force-toolchain[force toolchain]'
  '--ignore-platform-key[ignore platform key]'
  '(-G --integration)'{-G,--integration}'[integration mode]'
  '(-m --last-metrics)'{-m,--last-metrics}'[last metrics]'
  '--list-tags[list tags]'
  '--list-tests[list tests]'
  '(-k --make)'{-k,--make}'[use make]'
  '(-N --ninja)'{-N,--ninja}'[use ninja]'
  '--no-detailed-test-id[no detailed test ID]'
  '--overflow-as-errors[treat overflow as errors]'
  '--persistent-hardware-map[persistent hardware map]'
  '--platform-reports[platform reports]'
  '--prep-artifacts-for-testing[prepare artifacts for testing]'
  '--quarantine-verify[quarantine verify]'
  '--retry-build-errors[retry build errors]'
  '--short-build-path[short build path]'
  '--show-footprint[show footprint]'
  '--shuffle-tests[shuffle tests]'
  '--test-only[test only]'
  '--test-tree[test tree]'
  '--timestamps[timestamps]'

  # Directory options
  '(-O --outdir)'{-O,--outdir}'[output directory]:dir:_directories'
  '(-A --board-root)'{-A,--board-root}'[board root directory]:dir:_directories'
  '(--testsuite-root)'{--testsuite-root}'[testsuite root]:dir:_directories'
  '(-o --report-dir)'{-o,--report-dir}'[report directory]:dir:_directories'
  '--alt-config-root[alt config root]:dir:_directories'
  '--coverage-basedir[coverage basedir]:dir:_directories'

  # File options
  '--compare-report[compare report]:file:_files'
  '--device-serial[device serial]:file:_files'
  '--device-serial-pty[device serial PTY]:file:_files'
  '--gcov-tool[gcov tool]:file:_files'
  '--generate-hardware-map[generate hardware map]:file:_files'
  '--hardware-map[hardware map]:file:_files'
  '(-F --load-tests)'{-F,--load-tests}'[load tests]:file:_files'
  '--log-file[log file]:file:_files'
  '--package-artifacts[package artifacts]:file:_files'
  '--pre-script[pre script]:file:_files'
  '--quarantine-list[quarantine list]:file:_files'
  '(-E --save-tests)'{-E,--save-tests}'[save tests]:file:_files'
  '(-z --size)'{-z,--size}'[size tool]:file:_files'
  '--test-config[test config]:file:_files'

  # Options with values
  '(-p --platform)'{-p,--platform}'[target platform]:platform:_get_west_boards'
  '(-j --jobs)'{-j,--jobs}'[parallel jobs]:jobs:'
  '(-t --tag)'{-t,--tag}'[filter by tag]:tag:'
  '(-s --scenario --test)'{-s,--scenario,--test}'[test scenario]:scenario:'
  '(-a --arch)'{-a,--arch}'[architecture]:arch:'
  '--filter[filter tests]:filter:(buildable runnable)'
  '--timeout-multiplier[timeout multiplier]:multiplier:'
  '--coverage-platform[coverage platform]:platform:_get_west_boards'
  '--coverage-tool[coverage tool]:tool:(gcovr lcov)'
  '(-P --exclude-platform)'{-P,--exclude-platform}'[exclude platform]:platform:_get_west_boards'
  '--log-level[log level]:level:(CRITICAL DEBUG ERROR INFO NOTSET WARNING)'
  '(-M --runtime-artifact-cleanup)'{-M,--runtime-artifact-cleanup}'[runtime artifact cleanup]:cleanup:(all pass)'
  '--coverage-formats[coverage formats]:formats:'
  '--device-flash-timeout[device flash timeout]:timeout:'
  '--device-serial-baud[device serial baud]:baud:'
  '(-e --exclude-tag)'{-e,--exclude-tag}'[exclude tag]:tag:'
  '(-x --extra-args)'{-x,--extra-args}'[extra args]:args:'
  '(-X --fixture)'{-X,--fixture}'[fixture]:fixture:'
  '(-H --footprint-threshold)'{-H,--footprint-threshold}'[footprint threshold]:threshold:'
  '--level[level]:level:'
  '--pytest-args[pytest args]:args:'
  '--report-name[report name]:name:'
  '--report-suffix[report suffix]:suffix:'
  '--retry-failed[retry failed]:retry:'
  '--retry-interval[retry interval]:interval:'
  '--seed[seed]:seed:'
  '--shuffle-tests-seed[shuffle tests seed]:seed:'
  '--sub-test[sub test]:test:'
  '(-B --subset)'{-B,--subset}'[subset]:subset:'
  '--vendor[vendor]:vendor:'
  '--west-flash[west flash command]:command:'
  '--west-runner[west runner]:runner:'
  )

  _arguments -S -C $opts
}



_west_simulate() {
  __comp_west_runner_cmd
  }

_west_reset() {
  __comp_west_runner_cmd
}

_west_rtt() {
  local -a opts=(
  '(-d --build-dir)'{-d,--build-dir}'[build directory]:build dir:_directories'
  '(-s --serial-pty)'{-s,--serial-pty}'[serial PTY]:pty:_files'
  )

  _arguments -S $opts
}

_west_zephyr-export() {
  _arguments -S
}

_west_bindesc() {
  local -a subcommands=(
  'dump:Dump all binary descriptors in the image'
  'extract:Extract the binary descriptor blob to a file'
  'search:Search for a specific descriptor'
  'custom_search:Search for a custom descriptor'
  'list:List all known descriptors'
  'get_offset:Get the offset of the descriptors'
  )

  local context state line
  typeset -A opt_args

  _arguments -S -C \
    '1:subcommand:->subcmds' \
    '*::arg:->args'

  case $state in
  subcmds)
    _describe 'subcommands' subcommands
    ;;
  args)
    curcontext="${curcontext%:*:*}:west-bindesc-${line[1]}:"
    _call_function ret _west_bindesc_${line[1]}
    ;;
  esac
}

_west_bindesc_dump() {
  _arguments -S \
    '*:image:_files'
}

_west_bindesc_extract() {
  _arguments -S \
    '*:image:_files'
}

_west_bindesc_search() {
  _arguments -S \
    '*:image:_files'
}

_west_bindesc_custom_search() {
  _arguments -S \
    '*:image:_files'
}

_west_bindesc_list() {
  _arguments -S \
    '*:image:_files'
}

_west_bindesc_get_offset() {
  _arguments -S \
    '*:image:_files'
}

# don't run the completion function when being source-ed or eval-ed
if [ "$funcstack[1]" = "_west" ]; then
    _west
fi