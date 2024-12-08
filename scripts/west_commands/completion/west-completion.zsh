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
  'status["git status" for one or more projects]'
  'forall[run a command in one or more local projects]'
  'config[get or set config file values]'
  'topdir[print the top level directory of the workspace]'
  'help[get help for west or a command]'
  )

  local -a zephyr_ext_cmds=(
  'completion[display shell completion scripts]'
  'boards[display information about supported boards]'
  'build[compile a Zephyr application]'
  'sign[sign a Zephyr binary for bootloader chain-loading]'
  'flash[flash and run a binary on a board]'
  'debug[flash and interactively debug a Zephyr application]'
  'debugserver[connect to board and launch a debug server]'
  'attach[interactively debug a board]'
  'zephyr-export[export Zephyr installation as a CMake config package]'
  'spdx[create SPDX bill of materials]'
  'blobs[work with binary blobs]'
  'sdk[manage SDKs]'
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
  _west_boards=( $(__west_x boards --format='{name}|{qualifiers}') )
  for i in {1..${#_west_boards[@]}}; do
    local name="${_west_boards[$i]%%|*}"
    local transformed_board="${_west_boards[$i]//|//}"
    _west_boards[$i]="${transformed_board//,/ ${name}/}"
  done
  _west_boards=(${(@s/ /)_west_boards})

  _describe 'boards' _west_boards
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
  '--skip-rebuild[do not refresh cmake dependencies first]'
  '--domain[execute build tool (make or ninja) for a given domain]:domain:'
  )

  local -a all_opts=(${_west_runner_opts} ${opts})
  _arguments -S $all_opts
}

_west_debug() {
  _west_flash
}

_west_debugserver() {
  _west_flash
}

_west_attach() {
  _west_flash
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

# don't run the completion function when being source-ed or eval-ed
if [ "$funcstack[1]" = "_west" ]; then
    _west
fi
