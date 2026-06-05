# Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
# SPDX-License-Identifier: Apache-2.0

SHELL := /bin/sh

# Absolute path to this Makefile's directory
SCRIPT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
export ZEPHYR_BASE := $(SCRIPT_DIR)

# Virtualenv activation (matches original script’s ../../.py_dev_venv)
VENV_ACTIVATE ?= $(SCRIPT_DIR)/../../.py_dev_venv/bin/activate
USE_VENV ?= 1

ifeq ($(USE_VENV),1)
WEST := . "$(VENV_ACTIVATE)" && west
else
WEST := west
endif

# Defaults (override from CLI if needed)
BOARD_LIST ?= nucleo_g474re/stm32g474xx
APP_LIST ?= \
  tests/drivers/cordic/cordic_api

PORT ?= /dev/ttyACM0
BAUD ?= 115200
VENDOR ?= st
VERBOSE ?= -v

# Internal: run one app/board pair
define RUN_ONE
@set -e; \
app_abs="$(SCRIPT_DIR)/$(1)"; \
safe_board=$$(printf "%s" "$(2)" | tr '/' '_'); \
build_dir="$$app_abs/build/$$safe_board"; \
printf "\n--> use build directory: %s\n" "$$build_dir"; \
if printf "%s" "$(1)" | grep -q "tests/"; then \
	$(WEST) twister -p $(2) -T "$$app_abs" --vendor $(VENDOR) \
	--device-testing --device-serial-baud $(BAUD) --device-serial "$(PORT)" $(VERBOSE); \
else \
	$(WEST) build -p auto -d "$$build_dir" -b $(2) "$$app_abs"; \
	$(WEST) flash -d "$$build_dir"; \
fi
endef

.PHONY: all
all:
	@set -e; \
	for board in $(BOARD_LIST); do \
		for app in $(APP_LIST); do \
			$(MAKE) --no-print-directory _run APP="$$app" BOARD="$$board"; \
		done; \
	done

.PHONY: _run
_run:
	$(call RUN_ONE,$(APP),$(BOARD))

.PHONY: list
list:
	@echo "Boards:"; for b in $(BOARD_LIST); do echo "  - $$b"; done;
	@echo "Apps:"; for a in $(APP_LIST); do echo "  - $$a"; done

# Only run test (twister) items
.PHONY: twister
twister:
	@set -e; \
	for board in $(BOARD_LIST); do \
		for app in $(APP_LIST); do \
			case "$$app" in *tests/*) \
				$(MAKE) --no-print-directory _run APP="$$app" BOARD="$$board";; \
		esac; \
		done; \
	done

# Only run non-test (west build/flash) items
.PHONY: build_flash
build_flash:
	@set -e; \
	for board in $(BOARD_LIST); do \
		for app in $(APP_LIST); do \
			case "$$app" in *tests/*) ;; \
				$(MAKE) --no-print-directory _run APP="$$app" BOARD="$$board";; \
			esac; \
		done; \
	done

# Optional utilities
.PHONY: check-tools
check-tools:
	@command -v west >/dev/null 2>&1 || { echo "west not found in PATH"; exit 1; }
ifeq ($(USE_VENV),1)
	@[ -f "$(VENV_ACTIVATE)" ] || { echo "Missing venv at $(VENV_ACTIVATE)"; exit 1; }
endif

# Convenience targets for the commented lines in build.sh (optional)
.PHONY: checkpatch compliance
checkpatch:
	@$(SCRIPT_DIR)/scripts/checkpatch.pl --git main..HEAD

compliance:
	@$(SCRIPT_DIR)/scripts/ci/check_compliance.py

clean:
	@git clean -xdf
