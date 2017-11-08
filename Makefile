#
# Top level makefile for things not covered by cmake
#

ifeq ($(VERBOSE),1)
  Q =
else
  Q = @
endif

MAKEFLAGS += --no-print-directory
export Q

# Documentation targets
# ---------------------------------------------------------------------------
htmldocs:
	$(Q)$(MAKE) -C doc htmldocs
