#!/bin/bash
# llext-link-wrapper.sh
# Wraps the shared library link step for LLEXT modules.
# 1. Makes read-only sections writable via objcopy (fixes "dangerous relocation
#    in read-only section" errors from the Xtensa linker)
# 2. Links with GCC + -mtext-section-literals + linker script (fixes "l32r
#    literal placed after use" errors from section ordering)
# 3. Strips X flag from .rodata in the output .so (prevents llext_link_helper.py
#    from misclassifying .rodata as executable)
#
# Usage: llext-link-wrapper.sh <cross-compile-prefix> [args...]

CROSS_PREFIX="$1"
shift

SCRIPT_DIR="$(dirname "$0")"
GCC="${CROSS_PREFIX}gcc"
OBJCOPY="${CROSS_PREFIX}objcopy"
READELF="${CROSS_PREFIX}readelf"

# Find output file from -o argument
OUTPUT=""
ARGS=()
skip_next=false
for arg in "$@"; do
  if $skip_next; then
    OUTPUT="$arg"
    ARGS+=("$arg")
    skip_next=false
    continue
  fi
  case "$arg" in
    --target=*|--target\ *)   continue ;; # GCC doesn't need --target
    -mcpu=*)                  continue ;; # GCC doesn't need -mcpu for linking
    -o)                       skip_next=true ;;
  esac

  if [[ "$arg" == *.obj ]] && [[ -f "$arg" ]]; then
    # Make relevant read-only sections writable in-place
    while IFS= read -r section; do
      [[ -n "$section" ]] && \
        "$OBJCOPY" --set-section-flags "$section=alloc,contents,load,data" "$arg" 2>/dev/null || true
    done < <("$READELF" -SW "$arg" 2>/dev/null | \
        grep -E '\.rodata\.|_log_const|\.module[^_]|\.exported_sym' | \
        grep -v '\.rela\.' | grep -v '\.debug' | \
        grep ' A ' | grep -v ' W' | \
        sed 's/.*\] //' | awk '{print $1}')
  fi
  ARGS+=("$arg")
done

# Link with GCC
"$GCC" -mtext-section-literals -Wl,-T,"${SCRIPT_DIR}/llext_shared.ld" "${ARGS[@]}"
rc=$?

# Post-link: strip X flag from .rodata so llext_link_helper.py classifies it correctly
if [[ $rc -eq 0 ]] && [[ -n "$OUTPUT" ]] && [[ -f "$OUTPUT" ]]; then
  "$OBJCOPY" --set-section-flags .rodata=alloc,contents,load,data "$OUTPUT" 2>/dev/null || true
fi

exit $rc
