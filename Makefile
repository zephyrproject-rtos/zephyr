VERSION_MAJOR 	   = 1
VERSION_MINOR 	   = 7
PATCHLEVEL 	   = 99
VERSION_RESERVED   = 0
EXTRAVERSION       =
NAME 		   = Zephyr Kernel

export SOURCE_DIR PROJECT

ifeq ($(MAKECMDGOALS),)
$(error Invoking make from top-level kernel directory is not supported)
endif

# *DOCUMENTATION*
# To see a list of typical targets execute "make help"
# More info can be located in ./README
# Comments in this file are targeted only to the developer, do not
# expect to learn how to build the kernel reading this file.
#

# o Do not use make's built-in rules and variables
#   (this increases performance and avoids hard-to-debug behaviour);
# o Look for make include files relative to root of kernel src
MAKEFLAGS += -rR --include-dir=$(CURDIR)

UNAME := $(shell uname)
ifeq (MSYS, $(findstring MSYS, $(UNAME)))
DISABLE_TRYRUN=y
HOST_OS=MSYS
NATIVE_PWD_OPT=-W
else ifeq (MINGW, $(findstring MINGW, $(UNAME)))
HOST_OS=MINGW
PWD_OPT=-W
NATIVE_PWD_OPT=-W
DISABLE_TRYRUN=y
CPATH ?= $(MIGW_DIR)/include
LIBRARY_PATH ?= $(MINGW_DIR)/lib
export CPATH LIBRARY_PATH
else ifeq (Linux, $(findstring Linux, $(UNAME)))
HOST_OS=Linux
else ifeq (Darwin, $(findstring Darwin, $(UNAME)))
HOST_OS=Darwin
endif
export HOST_OS NATIVE_PWD_OPT

# Avoid funny character set dependencies
unexport LC_ALL
LC_COLLATE=C
LC_NUMERIC=C
export LC_COLLATE LC_NUMERIC

# Avoid interference with shell env settings
unexport GREP_OPTIONS

DQUOTE = "
#This comment line is to fix the highlighting of some editors due the quote effect."
export DQUOTE

# We are using a recursive build, so we need to do a little thinking
# to get the ordering right.
#
# Most importantly: sub-Makefiles should only ever modify files in
# their own directory. If in some directory we have a dependency on
# a file in another dir (which doesn't happen often, but it's often
# unavoidable when linking the built-in.o targets which finally
# turn into the kernel binary), we will call a sub make in that other
# dir, and after that we are sure that everything which is in that
# other dir is now up to date.
#
# The only cases where we need to modify files which have global
# effects are thus separated out and done before the recursive
# descending is started. They are now explicitly listed as the
# prepare rule.

# Beautify output
# ---------------------------------------------------------------------------
#
# Normally, we echo the whole command before executing it. By making
# that echo $($(quiet)$(cmd)), we now have the possibility to set
# $(quiet) to choose other forms of output instead, e.g.
#
#         quiet_cmd_cc_o_c = Compiling $(RELDIR)/$@
#         cmd_cc_o_c       = $(CC) $(c_flags) -c -o $@ $<
#
# If $(quiet) is empty, the whole command will be printed.
# If it is set to "quiet_", only the short version will be printed.
# If it is set to "silent_", nothing will be printed at all, since
# the variable $(silent_cmd_cc_o_c) doesn't exist.
#
# A simple variant is to prefix commands with $(Q) - that's useful
# for commands that shall be hidden in non-verbose mode.
#
#	$(Q)ln $@ :<
#
# If KBUILD_VERBOSE equals 0 then the above command will be hidden.
# If KBUILD_VERBOSE equals 1 then the above command is displayed.
#
# To put more focus on warnings, be less verbose as default
# Use 'make V=1' to see the full commands

ifeq ("$(origin V)", "command line")
  KBUILD_VERBOSE = $(V)
endif
ifndef KBUILD_VERBOSE
  KBUILD_VERBOSE = 0
endif

ifeq ($(KBUILD_VERBOSE),1)
  quiet =
  Q =
else
  quiet=quiet_
  Q = @
endif

# If the user is running make -s (silent mode), suppress echoing of
# commands

ifneq ($(filter 4.%,$(MAKE_VERSION)),)	# make-4
ifneq ($(filter %s ,$(firstword x$(MAKEFLAGS))),)
  quiet=silent_
endif
else					# make-3.8x
ifneq ($(filter s% -s%,$(MAKEFLAGS)),)
  quiet=silent_
endif
endif

export quiet Q KBUILD_VERBOSE

# kbuild supports saving output files in a separate directory.
# To locate output files in a separate directory two syntaxes are supported.
# In both cases the working directory must be the root of the kernel src.
# 1) O=
# Use "make O=dir/to/store/output/files/"
#
# 2) Set KBUILD_OUTPUT
# Set the environment variable KBUILD_OUTPUT to point to the directory
# where the output files shall be placed.
# export KBUILD_OUTPUT=dir/to/store/output/files/
# make
#
# The O= assignment takes precedence over the KBUILD_OUTPUT environment
# variable.

# KBUILD_SRC is set on invocation of make in OBJ directory
# KBUILD_SRC is not intended to be used by the regular user (for now)
ifeq ($(KBUILD_SRC),)

# OK, Make called in directory where kernel src resides
# Do we want to locate output files in a separate directory?
ifeq ("$(origin O)", "command line")
  KBUILD_OUTPUT := $(O)
endif

# That's our default target when none is given on the command line
PHONY := _all
_all:

# Cancel implicit rules on top Makefile
$(CURDIR)/Makefile Makefile: ;

ifneq ($(KBUILD_OUTPUT),)
# Invoke a second make in the output directory, passing relevant variables
# check that the output directory actually exists
saved-output := $(KBUILD_OUTPUT)
KBUILD_OUTPUT := $(shell mkdir -p $(KBUILD_OUTPUT) && cd $(KBUILD_OUTPUT) \
								&& pwd $(PWD_OPT))
$(if $(KBUILD_OUTPUT),, \
     $(error failed to create output directory "$(saved-output)"))

PHONY += $(MAKECMDGOALS) sub-make

$(filter-out _all sub-make $(CURDIR)/Makefile, $(MAKECMDGOALS)) _all: sub-make
	@:

sub-make: FORCE
	$(Q)$(MAKE) -C $(KBUILD_OUTPUT) KBUILD_SRC=$(CURDIR) \
	-f $(CURDIR)/Makefile $(filter-out _all sub-make,$(MAKECMDGOALS))

# Leave processing to above invocation of make
skip-makefile := 1
endif # ifneq ($(KBUILD_OUTPUT),)
endif # ifeq ($(KBUILD_SRC),)

# We process the rest of the Makefile if this is the final invocation of make
ifeq ($(skip-makefile),)

# Do not print "Entering directory ...",
# but we want to display it when entering to the output directory
# so that IDEs/editors are able to understand relative filenames.
MAKEFLAGS += --no-print-directory

# Call a source code checker (by default, "sparse") as part of the
# C compilation.
#
# Use 'make C=1' to enable checking of only re-compiled files.
# Use 'make C=2' to enable checking of *all* source files, regardless
# of whether they are re-compiled or not.
#
ifeq ("$(origin C)", "command line")
  KBUILD_CHECKSRC = $(C)
endif
ifndef KBUILD_CHECKSRC
  KBUILD_CHECKSRC = 0
endif

PHONY += all
_all: all

ifeq ($(KBUILD_SRC),)
        # building in the source tree
        srctree := .
else
        ifeq ($(KBUILD_SRC)/,$(dir $(CURDIR)))
                # building in a subdirectory of the source tree
                srctree := ..
        else
                srctree := $(KBUILD_SRC)
        endif
endif
objtree		:= .
src		:= $(srctree)
obj		:= $(objtree)

VPATH		:= $(SOURCE_DIR_PARENT) $(srctree)

export srctree objtree VPATH


# SUBARCH tells the usermode build what the underlying arch is.  That is set
# first, and if a usermode build is happening, the "ARCH=um" on the command
# line overrides the setting of ARCH below.  If a native build is happening,
# then ARCH is assigned, getting whatever value it gets normally, and
# SUBARCH is subsequently ignored.

SUBARCH := $(shell uname -m | sed -e s/i.86/x86/ -e s/x86_64/x86/ \
				  -e s/sun4u/sparc64/ \
				  -e s/arm.*/arm/ -e s/sa110/arm/ \
				  -e s/s390x/s390/ -e s/parisc64/parisc/ \
				  -e s/ppc.*/powerpc/ -e s/mips.*/mips/ \
				  -e s/sh[234].*/sh/ -e s/aarch64.*/arm64/ )

# Cross compiling and selecting different set of gcc/bin-utils
# ---------------------------------------------------------------------------
#
# When performing cross compilation for other architectures ARCH shall be set
# to the target architecture. (See arch/* for the possibilities).
# ARCH can be set during invocation of make:
# make ARCH=x86
# Another way is to have ARCH set in the environment.
# The default ARCH is the host where make is executed.

# CROSS_COMPILE specify the prefix used for all executables used
# during compilation. Only gcc and related bin-utils executables
# are prefixed with $(CROSS_COMPILE).
# CROSS_COMPILE can be set on the command line
# make CROSS_COMPILE=i586-pc-elf
# Alternatively CROSS_COMPILE can be set in the environment.
# A third alternative is to store a setting in .config so that plain
# "make" in the configured kernel build directory always uses that.
# Default value for CROSS_COMPILE is not to prefix executables
# Note: Some architectures assign CROSS_COMPILE in their arch/*/Makefile
ARCH		?= $(SUBARCH)
CROSS_COMPILE	?= $(CONFIG_CROSS_COMPILE:"%"=%)

KCONFIG_CONFIG	?= .config
export KCONFIG_CONFIG

# SHELL used by kbuild
CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)

HOSTCC       ?= gcc
HOSTCXX      ?= g++
HOSTCFLAGS   = -Wall -Wmissing-prototypes -Wstrict-prototypes -O2 -fomit-frame-pointer -std=gnu89
HOSTCXXFLAGS = -O2

ifeq ($(shell $(HOSTCC) -v 2>&1 | grep -c "clang version"), 1)
HOSTCFLAGS  += -Wno-unused-value -Wno-unused-parameter \
		-Wno-missing-field-initializers -fno-delete-null-pointer-checks
endif

# Decide whether to build built-in, modular, or both.
# Normally, just do built-in.

KBUILD_BUILTIN := 1

export KBUILD_BUILTIN
export KBUILD_CHECKSRC KBUILD_SRC

# Look for make include files relative to root of kernel src
MAKEFLAGS += --include-dir=$(srctree)

# We need some generic definitions (do not try to remake the file).
$(srctree)/scripts/Kbuild.include: ;
include $(srctree)/scripts/Kbuild.include
ifeq ($(USE_CCACHE),1)
CCACHE := ccache
endif

# Make variables (CC, etc...)
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
ifeq ($(USE_CCACHE),1)
CC		= $(CCACHE) $(CROSS_COMPILE)gcc
CXX		= $(CCACHE) $(CROSS_COMPILE)g++
else
CC		?= $(CROSS_COMPILE)gcc
CXX		?= $(CROSS_COMPILE)g++
endif
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
GDB		= $(CROSS_COMPILE)gdb
READELF		= $(CROSS_COMPILE)readelf
AWK		= awk
ifeq ($(PREBUILT_HOST_TOOLS),)
GENOFFSET_H	= scripts/gen_offset_header/gen_offset_header
FIXDEP		= scripts/basic/fixdep
else
GENOFFSET_H	= $(PREBUILT_HOST_TOOLS)/gen_offset_header
ifneq ($(filter host-tools, $(MAKECMDGOALS)),)
FIXDEP		= scripts/basic/fixdep
else
FIXDEP		= $(PREBUILT_HOST_TOOLS)/fixdep
endif
endif
PERL		= perl
PYTHON		= python
CHECK		= sparse

CHECKFLAGS     := -Wbitwise -Wno-return-void $(CF)
CFLAGS_GCOV	= -fprofile-arcs -ftest-coverage

ifeq ($(COMPILER),clang)
ifneq ($(CROSS_COMPILE),)
CLANG_TARGET    := -target $(notdir $(CROSS_COMPILE:%-=%))
GCC_TOOLCHAIN   := $(dir $(CROSS_COMPILE))
endif
ifneq ($(GCC_TOOLCHAIN),)
CLANG_GCC_TC    := -gcc-toolchain $(GCC_TOOLCHAIN)
endif
ifneq ($(IA),1)
CLANG_IA_FLAG   = -no-integrated-as
endif
CLANG_FLAGS     := $(CLANG_TARGET) $(CLANG_GCC_TC) $(CLANG_IA_FLAG)
endif

# Use USERINCLUDE when you must reference the UAPI directories only.
USERINCLUDE    := -include $(CURDIR)/include/generated/autoconf.h

SOC_NAME = $(subst $(DQUOTE),,$(CONFIG_SOC))
SOC_SERIES = $(subst $(DQUOTE),,$(CONFIG_SOC_SERIES))
SOC_FAMILY = $(subst $(DQUOTE),,$(CONFIG_SOC_FAMILY))

override ARCH = $(subst $(DQUOTE),,$(CONFIG_ARCH))
BOARD_NAME = $(subst $(DQUOTE),,$(CONFIG_BOARD))
KERNEL_NAME = $(subst $(DQUOTE),,$(CONFIG_KERNEL_BIN_NAME))
KERNEL_ELF_NAME = $(KERNEL_NAME).elf
KERNEL_BIN_NAME = $(KERNEL_NAME).bin
KERNEL_HEX_NAME = $(KERNEL_NAME).hex
KERNEL_STAT_NAME = $(KERNEL_NAME).stat
PREBUILT_KERNEL = $(KERNEL_NAME)_prebuilt.elf

export SOC_FAMILY SOC_SERIES SOC_PATH SOC_NAME BOARD_NAME
export ARCH KERNEL_NAME KERNEL_ELF_NAME KERNEL_BIN_NAME KERNEL_HEX_NAME
# Use ZEPHYRINCLUDE when you must reference the include/ directory.
# Needed to be compatible with the O= option
ZEPHYRINCLUDE    = \
		-I$(srctree)/kernel/include \
		-I$(srctree)/arch/$(ARCH)/include \
		-I$(srctree)/arch/$(ARCH)/soc/$(SOC_PATH) \
		-I$(srctree)/boards/$(ARCH)/$(BOARD_NAME) \
		$(if $(KBUILD_SRC), -I$(srctree)/include) \
		-I$(srctree)/include \
		-I$(CURDIR)/include/generated \
		$(USERINCLUDE) \
		$(STDINCLUDE)

KBUILD_CPPFLAGS := -DKERNEL -D__ZEPHYR__=1

KBUILD_CFLAGS   := -c -g -std=c99 \
		-Wall \
		-Wformat \
		-Wformat-security \
		-D_FORTIFY_SOURCE=2 \
		-Wno-format-zero-length \
		-Wno-main -ffreestanding

KBUILD_CXXFLAGS   := -c -g -std=c++11 \
		-fno-reorder-functions \
		-fno-asynchronous-unwind-tables \
		-fcheck-new \
		-fno-defer-pop -Wall \
		-Wno-unused-but-set-variable \
		-Wno-format-zero-length \
		-Wno-main -ffreestanding \
		-ffunction-sections -fdata-sections \
		-fno-rtti -fno-exceptions

KBUILD_AFLAGS   := -c -g -xassembler-with-cpp -D_ASMLANGUAGE

LDFLAGS += $(call ld-option,-nostartfiles)
LDFLAGS += $(call ld-option,-nodefaultlibs)
LDFLAGS += $(call ld-option,-nostdlib)
LDFLAGS += $(call ld-option,-static)

KERNELVERSION = $(VERSION_MAJOR)$(if $(VERSION_MINOR),.$(VERSION_MINOR)$(if $(PATCHLEVEL),.$(PATCHLEVEL)))$(EXTRAVERSION)

exports += VERSION_MAJOR VERSION_MINOR PATCHLEVEL VERSION_RESERVED EXTRAVERSION
exports += KERNELRELEASE KERNELVERSION
exports += ARCH CONFIG_SHELL HOSTCC HOSTCFLAGS CROSS_COMPILE AS LD CC CXX
exports += CPP AR NM STRIP OBJCOPY OBJDUMP GDB
exports += MAKE AWK INSTALLKERNEL PERL PYTHON GENOFFSET_H
exports += HOSTCXX HOSTCXXFLAGS CHECK CHECKFLAGS

exports += KBUILD_CPPFLAGS NOSTDINC_FLAGS ZEPHYRINCLUDE OBJCOPYFLAGS LDFLAGS
exports += KBUILD_CFLAGS KBUILD_CXXFLAGS CFLAGS_GCOV KBUILD_AFLAGS AFLAGS_KERNEL
exports += KBUILD_ARFLAGS FIXDEP

# Push the exports to sub-processes
export $(exports)

# Make the exports available to third-party build systems
PHONY += outputexports
outputexports: Makefile.export
Makefile.export: include/config/auto.conf FORCE
	$(call filechk,Makefile.export)

define filechk_Makefile.export
	(echo "# file is auto-generated, do not modify !"; \
	echo "BOARD=$(BOARD)"; \
	echo; \
	$(foreach e,$(exports),echo $(e)=$($(e));) echo; \
	echo "include $(O)/include/config/auto.conf";)
endef

# Files to ignore in find ... statements

export RCS_FIND_IGNORE := \( -name SCCS -o -name BitKeeper -o -name .svn -o    \
			  -name CVS -o -name .pc -o -name .hg -o -name .git \) \
			  -prune -o
export RCS_TAR_IGNORE := --exclude SCCS --exclude BitKeeper --exclude .svn \
			 --exclude CVS --exclude .pc --exclude .hg --exclude .git

# ===========================================================================
# Rules shared between *config targets and build targets

# Basic helpers built in scripts/
PHONY += scripts_basic
ifeq ($(PREBUILT_HOST_TOOLS),)
scripts_basic:
	$(Q)$(MAKE) $(build)=scripts/basic
	$(Q)$(MAKE) $(build)=scripts/gen_offset_header
else
scripts_basic:
endif

# To avoid any implicit rule to kick in, define an empty command.
scripts/basic/%: scripts_basic ;

PHONY += outputmakefile
# outputmakefile generates a Makefile in the output directory, if using a
# separate output directory. This allows convenient use of make in the
# output directory.
outputmakefile:
ifneq ($(KBUILD_SRC),)
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/mkmakefile \
	    $(srctree) $(objtree) $(VERSION_MAJOR) $(VERSION_MINOR)
endif

# To make sure we do not include .config for any of the *config targets
# catch them early, and hand them over to scripts/kconfig/Makefile
# It is allowed to specify more targets when calling make, including
# mixing *config targets and build targets.
# For example 'make oldconfig all'.
# Detect when mixed targets is specified, and make a second invocation
# of make so .config is not included in this case either (for *config).

version_h := include/generated/version.h

no-dot-config-targets := pristine distclean clean mrproper help kconfig-help host-tools \
			 cscope gtags TAGS tags help% %docs check% \
			 $(version_h) headers_% kernelversion %src-pkg

config-targets := 0
mixed-targets  := 0
dot-config     := 1

ifneq ($(filter $(no-dot-config-targets), $(MAKECMDGOALS)),)
	ifeq ($(filter-out $(no-dot-config-targets), $(MAKECMDGOALS)),)
		dot-config := 0
	endif
endif

ifneq ($(filter config %config,$(MAKECMDGOALS)),)
	config-targets := 1
	ifneq ($(filter-out config %config,$(MAKECMDGOALS)),)
		mixed-targets := 1
	endif
endif

ifeq ($(mixed-targets),1)
# ===========================================================================
# We're called with mixed targets (*config and build targets).
# Handle them one by one.

PHONY += $(MAKECMDGOALS) __build_one_by_one

$(filter-out __build_one_by_one, $(MAKECMDGOALS)): __build_one_by_one
	@:

__build_one_by_one:
	$(Q)set -e; \
	for i in $(MAKECMDGOALS); do \
		$(MAKE) -f $(srctree)/Makefile $$i; \
	done

else
ifeq ($(config-targets),1)
# ===========================================================================
# *config targets only - make sure prerequisites are updated, and descend
# in scripts/kconfig to make the *config target

# Read arch specific Makefile to set KBUILD_DEFCONFIG as needed.
# KBUILD_DEFCONFIG may point out an alternative default configuration
# used for 'make defconfig'
include $(srctree)/arch/$(subst $(DQUOTE),,$(CONFIG_ARCH))/Makefile
export KBUILD_DEFCONFIG KBUILD_KCONFIG

config: scripts_basic outputmakefile FORCE
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

%config: scripts_basic outputmakefile FORCE
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

else
# ===========================================================================
# Build targets only - this includes zephyr, arch specific targets, clean
# targets and others. In general all targets except *config targets.

# Additional helpers built in scripts/
# Carefully list dependencies so we do not try to build scripts twice
# in parallel
PHONY += scripts
scripts: scripts_basic include/config/auto.conf include/config/tristate.conf
	$(Q)$(MAKE) $(build)=$(@)

# arch/ must be last here so that .gnu.linkonce magic for interrupts/exceptions
# works as expected

ifneq ($(strip $(MAKEFILE_APP_DIR)),)
MAKEFILE_APP := $(realpath $(MAKEFILE_APP_DIR)/Makefile.app)
endif

ifneq ($(strip $(PROJECT)),)
ifneq ($(strip $(MAKEFILE_APP)), )
include $(MAKEFILE_APP)
else
-include $(PROJECT)/Makefile.app
endif
ifneq ($(strip $(KBUILD_ZEPHYR_APP)),)
export KBUILD_ZEPHYR_APP
endif
app-y := $(SOURCE_DIR)
endif


ifeq ($(dot-config),1)
# Read in config
-include include/config/auto.conf

# Read in dependencies to all Kconfig* files, make sure to run
# oldconfig if changes are detected.
-include include/config/auto.conf.cmd

# Read in DTS derived configuration, if it exists
#
# We check to see if the ARCH is correctly sourced before doing the -include
# The reason for this is due to implicit rules kicking in to create this file.
# If this occurs before the above auto.conf is sourced correctly, the build
# will iterate over the dts conf file 2-3 times before settling down to the
# correct output.
ifneq ($(ARCH),)
-include include/generated/generated_dts_board.conf
endif

# To avoid any implicit rule to kick in, define an empty command
$(KCONFIG_CONFIG) include/config/auto.conf.cmd: ;

# If .config is newer than include/config/auto.conf, someone tinkered
# with it and forgot to run make oldconfig.
# if auto.conf.cmd is missing then we are probably in a cleaned tree so
# we execute the config step to be sure to catch updated Kconfig files
include/config/%.conf: $(KCONFIG_CONFIG) include/config/auto.conf.cmd
	$(Q)$(MAKE) -f $(srctree)/Makefile silentoldconfig

else
# Dummy target needed, because used as prerequisite
include/config/auto.conf: ;
endif # $(dot-config)

# kernel objects are built as a static library
libs-y := kernel/
core-y := lib/ misc/ boards/ ext/ subsys/ tests/ arch/
drivers-y := drivers/

ARCH = $(subst $(DQUOTE),,$(CONFIG_ARCH))
export ARCH

ifeq ($(CONFIG_DEBUG),y)
KBUILD_CFLAGS_OPTIMIZE := -Og
else
KBUILD_CFLAGS_OPTIMIZE := -Os
endif

ifdef ZEPHYR_GCC_VARIANT
include $(srctree)/scripts/Makefile.toolchain.$(ZEPHYR_GCC_VARIANT)
else
ifneq ($(MAKECMDGOALS),htmldocs)
$(if $(CROSS_COMPILE),, \
     $(error ZEPHYR_GCC_VARIANT is not set. ))
endif
endif

# Let Makefile.toolchain adjust optimization level
KBUILD_CFLAGS += $(KBUILD_CFLAGS_OPTIMIZE)

-include $(srctree)/ext/Makefile
-include $(srctree)/lib/Makefile

ifneq ($(CC),)
ifeq ($(shell $(CC) -v 2>&1 | grep -Ec "clang version|icx version"), 1)
COMPILER := clang
else
COMPILER := gcc
endif
export COMPILER
endif

# The all: target is the default when no target is given on the
# command line.
# This allow a user to issue only 'make' to build a kernel including modules
# Defaults to zephyr, but the arch makefile usually adds further targets
all: zephyr

ifdef CONFIG_READABLE_ASM
# Disable optimizations that make assembler listings hard to read.
# reorder blocks reorders the control in the function
# ipa clone creates specialized cloned functions
# partial inlining inlines only parts of functions
KBUILD_CFLAGS += $(call cc-option,-fno-reorder-blocks,) \
                 $(call cc-option,-fno-ipa-cp-clone,) \
                 $(call cc-option,-fno-partial-inlining)
endif

# Some GCC variants don't support these
KBUILD_CFLAGS += $(call cc-option,-fno-asynchronous-unwind-tables,)

ifeq ($(CONFIG_STACK_CANARIES),y)
KBUILD_CFLAGS += $(call cc-option,-fstack-protector-all,)
else
KBUILD_CFLAGS += $(call cc-option,-fno-stack-protector,)
endif

ifeq ($(CONFIG_OVERRIDE_FRAME_POINTER_DEFAULT),y)
ifeq ($(CONFIG_OMIT_FRAME_POINTER),y)
fp_arg := $(call cc-option,-fomit-frame-pointer,)
else
fp_arg := $(call cc-option,-fno-omit-frame-pointer,)
endif
endif
KBUILD_CFLAGS += $(fp_arg)
KBUILD_CXXFLAGS += $(fp_arg)

KBUILD_CFLAGS += $(subst $(DQUOTE),,$(CONFIG_COMPILER_OPT))

export LDFLAG_LINKERCMD

ifeq ($(SOC_SERIES),)
SOC_PATH = $(SOC_NAME)
else
SOC_PATH = $(SOC_FAMILY)/$(SOC_SERIES)
endif
include arch/$(ARCH)/Makefile

KBUILD_CFLAGS += $(CFLAGS)
KBUILD_CXXFLAGS += $(CXXFLAGS)
KBUILD_AFLAGS += $(CFLAGS)


ifeq ($(COMPILER),clang)
KBUILD_CPPFLAGS += $(call cc-option,-Wno-unknown-warning-option,)
KBUILD_CFLAGS += $(call cc-disable-warning, unused-variable)
KBUILD_CFLAGS += $(call cc-disable-warning, format-invalid-specifier)
KBUILD_CFLAGS += $(call cc-disable-warning, gnu)
# Quiet clang warning: comparison of unsigned expression < 0 is always false
KBUILD_CFLAGS += $(call cc-disable-warning, tautological-compare)
else

# This warning generated too much noise in a regular build.
# Use make W=1 to enable this warning (see scripts/Makefile.build)
KBUILD_CFLAGS += $(call cc-disable-warning, unused-but-set-variable)
KBUILD_CFLAGS += $(call cc-option,-fno-reorder-functions)
ifneq (${ZEPHYR_GCC_VARIANT},xcc)
KBUILD_CFLAGS += $(call cc-option,-fno-defer-pop)
endif
endif

# We trigger additional mismatches with less inlining
ifdef CONFIG_DEBUG_SECTION_MISMATCH
KBUILD_CFLAGS += $(call cc-option, -fno-inline-functions-called-once)
endif

# arch Makefile may override CC so keep this after arch Makefile is included
NOSTDINC_FLAGS += -nostdinc -isystem $(shell $(CC) -print-file-name=include)
NOSTDINC_FLAGS += -isystem $(shell $(CC) -print-file-name=include-fixed)
CHECKFLAGS     += $(NOSTDINC_FLAGS)

# disable pointer signed / unsigned warnings in gcc 4.0
KBUILD_CFLAGS += $(call cc-disable-warning, pointer-sign)

# disable invalid "can't wrap" optimizations for signed / pointers
KBUILD_CFLAGS	+= $(call cc-option,-fno-strict-overflow)

# generate an extra file that specifies the maximum amount of stack used,
# on a per-function basis.
ifdef CONFIG_STACK_USAGE
KBUILD_CFLAGS	+= $(call cc-option,-fstack-usage)
endif


# disallow errors like 'EXPORT_GPL(foo);' with missing header
KBUILD_CFLAGS   += $(call cc-option,-Werror=implicit-int)

# Prohibit date/time macros, which would make the build non-deterministic
# KBUILD_CFLAGS   += $(call cc-option,-Werror=date-time)

# use the deterministic mode of AR if available
KBUILD_ARFLAGS := $(call ar-option,D)

include $(srctree)/scripts/Makefile.extrawarn

# Add user supplied CPPFLAGS, AFLAGS and CFLAGS as the last assignments
KBUILD_CPPFLAGS += $(KCPPFLAGS)
KBUILD_AFLAGS += $(KAFLAGS)
KBUILD_CFLAGS += $(KCFLAGS)

# Use --build-id when available.

LINKFLAGPREFIX ?= -Wl,
LDFLAGS_zephyr += $(LDFLAGS)
LDFLAGS_zephyr += $(call cc-ldoption,$(LINKFLAGPREFIX)-X)
LDFLAGS_zephyr += $(call cc-ldoption,$(LINKFLAGPREFIX)-N)
LDFLAGS_zephyr += $(call cc-ldoption,$(LINKFLAGPREFIX)--gc-sections)
LDFLAGS_zephyr += $(call cc-ldoption,$(LINKFLAGPREFIX)--build-id=none)

LD_TOOLCHAIN ?= -D__GCC_LINKER_CMD__

ifdef CONFIG_HAVE_CUSTOM_LINKER_SCRIPT
KBUILD_LDS := $(subst $(DQUOTE),,$(PROJECT_BASE)/$(CONFIG_CUSTOM_LINKER_SCRIPT))
ifeq ($(wildcard $(KBUILD_LDS)),)
KBUILD_LDS := $(subst $(DQUOTE),,$(CONFIG_CUSTOM_LINKER_SCRIPT))
endif
else
# Try a board specific linker file
KBUILD_LDS := $(srctree)/boards/$(ARCH)/$(BOARD_NAME)/linker.ld

# If not available, try an SoC specific linker file
ifeq ($(wildcard $(KBUILD_LDS)),)
KBUILD_LDS         := $(srctree)/arch/$(ARCH)/soc/$(SOC_PATH)/linker.ld
endif
endif

export LD_TOOLCHAIN KBUILD_LDS

ifdef MAKEFILE_TOOLCHAIN_DO_PASS2
# KBUILD_CFLAGS known at this point.
# We can now determine which multilib libraries to use
include $(srctree)/scripts/Makefile.toolchain.$(ZEPHYR_GCC_VARIANT)
endif

QEMU		= $(addsuffix /,$(QEMU_BIN_PATH))$(QEMU_$(ARCH))

# The all: target is the default when no target is given on the
# command line.
# This allow a user to issue only 'make' to build a kernel including modules
# Defaults to zephyr, but the arch makefile usually adds further targets
all: $(KERNEL_BIN_NAME) $(KERNEL_STAT_NAME)

# Default kernel image to build when no specific target is given.
# KBUILD_IMAGE may be overruled on the command line or
# set in the environment
# Also any assignments in arch/$(ARCH)/Makefile take precedence over
# this default value
export KBUILD_IMAGE ?= zephyr

zephyr-dirs	:= $(patsubst %/,%,$(filter %/, $(core-y) $(drivers-y) \
		     $(libs-y)))

# Workaround for some make notdir implementations that require
# the paramenter not to end in "/".
zephyr-app-dir-root-name := $(patsubst %/,%, $(SOURCE_DIR))

zephyr-app-dir-root := $(abspath $(patsubst %, %/.., $(SOURCE_DIR)))

zephyr-alldirs	:= $(sort $(zephyr-dirs) $(SOURCE_DIR) $(patsubst %/,%,$(filter %/, \
		     $(core-) $(drivers-) $(libs-) $(app-))))

core-y		:= $(patsubst %/, %/built-in.o, $(core-y))
app-y		:= $(patsubst %, %/built-in.o, $(notdir $(zephyr-app-dir-root-name)))
drivers-y	:= $(patsubst %/, %/built-in.o, $(drivers-y))
libs-y1		:= $(patsubst %/, %/lib.a, $(libs-y))
libs-y2		:= $(patsubst %/, %/built-in.o, $(libs-y))
libs-y		:= $(libs-y1) $(libs-y2)

# core-y must be last here. several arches use .gnu.linkonce magic
# to register interrupt or exception handlers, and defaults under
# arch/ (part of core-y) must be linked after drivers or libs.
export KBUILD_ZEPHYR_MAIN := $(drivers-y) $(libs-y) $(core-y)
export LDFLAGS_zephyr

zephyr-deps := $(KBUILD_LDS) $(KBUILD_ZEPHYR_MAIN) $(app-y)

ALL_LIBS += $(TOOLCHAIN_LIBS)
export ALL_LIBS

LINK_LIBS := $(foreach l,$(ALL_LIBS), -l$(l))

quiet_cmd_ar_target = AR      $@
# Do not put lib.a into libzephyr.a. lib.a files are to be linked separately to
# the final image
cmd_ar_target = rm -f $@; $(AR) rcT$(KBUILD_ARFLAGS) $@ \
	$(filter-out %/lib.a, $(KBUILD_ZEPHYR_MAIN))
libzephyr.a: $(zephyr-deps)
	$(call cmd,ar_target)

quiet_cmd_create-lnk = LINK    $@
      cmd_create-lnk =								\
(										\
	echo $(LDFLAGS_zephyr); 						\
	echo "$(LINKFLAGPREFIX)-Map=$(KERNEL_NAME).map"; 			\
	echo "-L $(objtree)/include/generated";					\
	echo "-u _OffsetAbsSyms -u _ConfigAbsSyms"; 				\
	echo "-e __start"; 						 	\
	echo "$(LINKFLAGPREFIX)--start-group";					\
	echo "$(LINKFLAGPREFIX)--whole-archive";				\
	echo "$(KBUILD_ZEPHYR_APP)";						\
	echo "$(app-y)";							\
	echo "libzephyr.a";							\
	echo "$(LINKFLAGPREFIX)--no-whole-archive";         			\
	echo "$(filter %/lib.a, $(KBUILD_ZEPHYR_MAIN))";			\
	echo "$(objtree)/arch/$(ARCH)/core/offsets/offsets.o"; 			\
	echo "$(LINKFLAGPREFIX)--end-group"; 					\
	echo "$(LIB_INCLUDE_DIR) $(LINK_LIBS)";					\
) > $@

$(KERNEL_NAME).lnk: $(zephyr-deps)
	$(call cmd,create-lnk)

linker.cmd: $(zephyr-deps)
	$(Q)$(CC) -x assembler-with-cpp -nostdinc -undef -E -P \
		$(LDFLAG_LINKERCMD) $(LD_TOOLCHAIN) \
		-I$(srctree)/include -I$(SOURCE_DIR) \
		-I$(objtree)/include/generated \
		$(EXTRA_LINKER_CMD_OPT) $(KBUILD_LDS) -o $@


$(PREBUILT_KERNEL): $(zephyr-deps) libzephyr.a $(KBUILD_ZEPHYR_APP) $(app-y) \
		linker.cmd $(KERNEL_NAME).lnk
	$(Q)$(CC) -T linker.cmd @$(KERNEL_NAME).lnk -o $@

ASSERT_WARNING_STR := \
    "\n      ------------------------------------------------------------" \
    "\n      --- WARNING:  __ASSERT() statements are globally ENABLED ---" \
    "\n      --- The kernel will run more slowly and uses more memory ---" \
    "\n      ------------------------------------------------------------\n\n" \

WARN_ABOUT_ASSERT := $(if $(CONFIG_ASSERT),echo -e -n $(ASSERT_WARNING_STR),true)


DEPRECATION_WARNING_STR := \
    "\n      WARNING:  The board '$(BOARD)' is deprecated and will be" \
    "\n      removed in version $(CONFIG_BOARD_DEPRECATED)\n\n"

WARN_ABOUT_DEPRECATION := $(if $(CONFIG_BOARD_DEPRECATED),echo -e \
				-n $(DEPRECATION_WARNING_STR),true)

ifeq ($(ARCH),x86)
include $(srctree)/arch/x86/Makefile.idt
endif

ifeq ($(CONFIG_GEN_ISR_TABLES),y)
include $(srctree)/arch/common/Makefile.gen_isr_tables
endif

ifneq ($(GENERATED_KERNEL_OBJECT_FILES),)

# Identical rule to linker.cmd, but we also define preprocessor LINKER_PASS2.
# For arches that place special metadata in $(PREBUILT_KERNEL) not intended
# for the final binary, it can be #ifndef'd around this.
linker-pass2.cmd: $(zephyr-deps)
	$(Q)$(CC) -x assembler-with-cpp -nostdinc -undef -E -P \
		-DLINKER_PASS2 \
		$(LDFLAG_LINKERCMD) $(LD_TOOLCHAIN) \
		-I$(srctree)/include -I$(SOURCE_DIR) \
		-I$(objtree)/include/generated \
		$(EXTRA_LINKER_CMD_OPT) $(KBUILD_LDS) -o $@

$(KERNEL_ELF_NAME): $(GENERATED_KERNEL_OBJECT_FILES) linker-pass2.cmd
	$(Q)$(CC) -T linker-pass2.cmd $(GENERATED_KERNEL_OBJECT_FILES) \
		@$(KERNEL_NAME).lnk -o $@
	$(Q)$(srctree)/scripts/check_link_map.py $(KERNEL_NAME).map
	@$(WARN_ABOUT_ASSERT)
	@$(WARN_ABOUT_DEPRECATION)

else # GENERATED_KERNEL_OBJECT_FILES

# Otherwise, nothing to do, prebuilt kernel is the real one
$(KERNEL_ELF_NAME): $(PREBUILT_KERNEL)
	$(Q)cp $(PREBUILT_KERNEL) $(KERNEL_ELF_NAME)
	$(Q)$(srctree)/scripts/check_link_map.py $(KERNEL_NAME).map
	@$(WARN_ABOUT_ASSERT)
	@$(WARN_ABOUT_DEPRECATION)
endif


quiet_cmd_gen_bin = BIN     $@
      cmd_gen_bin =									\
(											\
        $(OBJDUMP) -S $< > $(KERNEL_NAME).lst;						\
        $(OBJCOPY) -S -O binary -R .note -R .comment -R COMMON -R .eh_frame $< $@;	\
        $(STRIP) -s -o $(KERNEL_NAME).strip $<;						\
)

$(KERNEL_BIN_NAME): $(KERNEL_ELF_NAME)
	$(call cmd,gen_bin)

quiet_cmd_gen_hex = HEX     $@
      cmd_gen_hex =                                                         \
(                                                                           \
   $(OBJCOPY) -S -O ihex -R .note -R .comment -R COMMON -R .eh_frame $< $@; \
)

$(KERNEL_HEX_NAME): $(KERNEL_ELF_NAME)
	$(call cmd,gen_hex)

$(KERNEL_STAT_NAME): $(KERNEL_BIN_NAME) $(KERNEL_ELF_NAME)
	@$(READELF) -e $(KERNEL_ELF_NAME) > $@

ram_report: $(KERNEL_STAT_NAME)
	@$(srctree)/scripts/size_report -r -o $(O)
rom_report: $(KERNEL_STAT_NAME)
	@$(srctree)/scripts/size_report -F -o $(O)

zephyr: $(zephyr-deps) $(KERNEL_BIN_NAME)

ifeq ($(CONFIG_HAS_DTS),y)
define filechk_generated_dts_board.h
	(echo "/* WARNING. THIS FILE IS AUTO-GENERATED. DO NOT MODIFY! */"; \
		if test -e $(ZEPHYR_BASE)/dts/$(ARCH)/$(BOARD_NAME).fixup; then \
			$(ZEPHYR_BASE)/scripts/extract_dts_includes.py \
				-d dts/$(ARCH)/$(BOARD_NAME).dts_compiled \
				-y $(ZEPHYR_BASE)/dts/$(ARCH)/yaml \
				-f $(ZEPHYR_BASE)/dts/$(ARCH)/$(BOARD_NAME).fixup; \
		else \
			$(ZEPHYR_BASE)/scripts/extract_dts_includes.py \
				-d dts/$(ARCH)/$(BOARD_NAME).dts_compiled \
				-y $(ZEPHYR_BASE)/dts/$(ARCH)/yaml; \
		fi; \
		)
endef
define filechk_generated_dts_board.conf
	(echo "# WARNING. THIS FILE IS AUTO-GENERATED. DO NOT MODIFY!"; \
		$(ZEPHYR_BASE)/scripts/extract_dts_includes.py \
		-d dts/$(ARCH)/$(BOARD_NAME).dts_compiled \
		-y $(ZEPHYR_BASE)/dts/$(ARCH)/yaml -k; \
		)
endef
else
define filechk_generated_dts_board.h
	(echo "/* WARNING. THIS FILE IS AUTO-GENERATED. DO NOT MODIFY! */";)
endef
define filechk_generated_dts_board.conf
	(echo "# WARNING. THIS FILE IS AUTO-GENERATED. DO NOT MODIFY!";)
endef
endif

include/generated/generated_dts_board.h: include/config/auto.conf FORCE
ifeq ($(CONFIG_HAS_DTS),y)
	$(Q)$(MAKE) $(build)=dts/$(ARCH)
endif
	$(call filechk,generated_dts_board.h)

include/generated/generated_dts_board.conf: include/config/auto.conf FORCE
ifeq ($(CONFIG_HAS_DTS),y)
	$(Q)$(MAKE) $(build)=dts/$(ARCH)
endif
	$(call filechk,generated_dts_board.conf)

dts: include/generated/generated_dts_board.h

# The actual objects are generated when descending,
# make sure no implicit rule kicks in
$(sort $(zephyr-deps)): $(zephyr-dirs) zephyr-app-dir ;

# Handle descending into subdirectories listed in $(zephyr-dirs)
# Preset locale variables to speed up the build process. Limit locale
# tweaks to this spot to avoid wrong language settings when running
# make menuconfig etc.
# Error messages still appears in the original language

PHONY += $(zephyr-dirs)
$(zephyr-dirs): prepare scripts
	$(Q)$(MAKE) $(build)=$@

PHONY += zephyr-app-dir
zephyr-app-dir: prepare scripts
	$(Q)$(MAKE) $(build)=$(notdir $(zephyr-app-dir-root-name)) \
		srctree=$(zephyr-app-dir-root)

# Things we need to do before we recursively start building the kernel
# or the modules are listed in "prepare".
# A multi level approach is used. prepareN is processed before prepareN-1.
# archprepare is used in arch Makefiles and when processed asm symlink,
# version.h and scripts_basic is processed / created.

# Listed in dependency order
PHONY += prepare prepare1 prepare2 prepare3

# prepare3 is used to check if we are building in a separate output directory,
# and if so do:
# 1) Check that make has not been executed in the kernel src $(srctree)
prepare3:
ifneq ($(KBUILD_SRC),)
	@$(kecho) '  Using $(srctree) as source for kernel'
	$(Q)if [ -f $(srctree)/.config ]; then \
		echo >&2 "  $(srctree) is not clean, please run 'make mrproper'"; \
		echo >&2 "  in the '$(srctree)' directory.";\
		/bin/false; \
	fi;
endif

# prepare2 creates a makefile if using a separate output directory
prepare2: prepare3 outputmakefile

prepare1: prepare2 $(version_h) \
                   include/config/auto.conf

archprepare_common = $(strip \
		prepare1 scripts_basic \
		)


archprepare = $(strip \
		$(archprepare_common) \
		)

# All the preparing..
prepare: $(archprepare) dts FORCE
	$(Q)$(MAKE) $(build)=.

# Generate some files
# ---------------------------------------------------------------------------

# KERNELRELEASE can change from a few different places, meaning version.h
# needs to be updated, so this check is forced on all builds

VERSION_MAJOR_HEX=$(shell printf '%02x\n' ${VERSION_MAJOR})
VERSION_MINOR_HEX=$(shell printf '%02x\n' ${VERSION_MINOR})
PATCHLEVEL_HEX=$(shell printf '%02x\n' ${PATCHLEVEL})
VERSION_RESERVED_HEX=00
KERNEL_VERSION_HEX=0x$(VERSION_MAJOR_HEX)$(VERSION_MINOR_HEX)$(PATCHLEVEL_HEX)

define filechk_version.h
       (echo "#ifndef _KERNEL_VERSION_H_"; \
       echo "#define _KERNEL_VERSION_H_"; \
       echo ;\
       (echo \#define ZEPHYR_VERSION_CODE $(shell                         \
       expr $(VERSION_MAJOR) \* 65536 + 0$(VERSION_MINOR) \* 256 + 0$(PATCHLEVEL)); \
       echo '#define ZEPHYR_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))';); \
       echo ;\
       echo "#define KERNELVERSION \\"; \
       echo "$(KERNEL_VERSION_HEX)$(VERSION_RESERVED_HEX)"; \
       echo "#define KERNEL_VERSION_NUMBER     $(KERNEL_VERSION_HEX)"; \
       echo "#define KERNEL_VERSION_MAJOR      $(VERSION_MAJOR)"; \
       echo "#define KERNEL_VERSION_MINOR      $(VERSION_MINOR)"; \
       echo "#define KERNEL_PATCHLEVEL         $(PATCHLEVEL)"; \
       echo "#define KERNEL_VERSION_STRING     \"$(KERNELVERSION)\""; \
       echo; \
       echo "#endif /* _KERNEL_VERSION_H_ */";)
endef

$(version_h): $(srctree)/Makefile FORCE
	$(call filechk,version.h)

PHONY += headerdep
headerdep:
	$(Q)find $(srctree)/include/ -name '*.h' | xargs --max-args 1 \
	$(srctree)/scripts/headerdep.pl -I$(srctree)/include

# ---------------------------------------------------------------------------

PHONY += depend dep
depend dep:
	@echo '*** Warning: make $@ is unnecessary now.'

###
# Cleaning is done on three levels.
# make clean     Delete most generated files
# make mrproper  Delete the current configuration, and all generated files
# make distclean Remove editor backup files, patch leftover files and the like

# Directories & files removed with 'make clean'
CLEAN_DIRS  += $(MODVERDIR)

CLEAN_FILES += 	include/generated/generated_dts_board.conf \
		include/generated/generated_dts_board.h \
		.old_version .tmp_System.map .tmp_version \
		.tmp_* System.map *.lnk *.map *.elf *.lst \
		*.bin *.hex *.stat *.strip staticIdt.o linker.cmd \
		linker-pass2.cmd

# Directories & files removed with 'make mrproper'
MRPROPER_DIRS  += bin include/config usr/include include/generated          \
		  arch/*/include/generated .tmp_objdiff
MRPROPER_FILES += .config .config.old .version $(version_h) \
		  Module.symvers tags TAGS cscope* GPATH GTAGS GRTAGS GSYMS \
		  signing_key.priv signing_key.x509 x509.genkey		\
		  extra_certificates signing_key.x509.keyid		\
		  signing_key.x509.signer

# clean - Delete most
#
clean: rm-dirs  := $(CLEAN_DIRS)
clean: rm-files := $(CLEAN_FILES)
clean-dirs      := $(addprefix _clean_, . $(zephyr-alldirs) )

PHONY += clean-dirs-target clean archclean zephyrclean
clean-dirs-target:
	$(Q)$(MAKE) $(clean)=$(patsubst _clean_%,%,$(clean-dirs))

clean: archclean

# mrproper - Delete all generated files, including .config
#
mrproper: rm-dirs  := $(wildcard $(MRPROPER_DIRS))
mrproper: rm-files := $(wildcard $(MRPROPER_FILES))
mrproper-dirs      := $(addprefix _mrproper_,scripts)

PHONY += $(mrproper-dirs) mrproper archmrproper
$(mrproper-dirs):
	$(Q)$(MAKE) $(clean)=$(patsubst _mrproper_%,%,$@)

mrproper: clean archmrproper $(mrproper-dirs)
	$(call cmd,rmdirs)
	$(call cmd,rmfiles)

# distclean
#
PHONY += distclean

distclean: mrproper
	@find $(srctree) $(RCS_FIND_IGNORE) \
		\( -name '*.orig' -o -name '*.rej' -o -name '*~' \
		-o -name '*.bak' -o -name '#*#' -o -name '.*.orig' \
		-o -name '.*.rej' -o -name '*%'  -o -name 'core' \) \
		-type f -print | xargs rm -f

# Brief documentation of the typical targets used
# ---------------------------------------------------------------------------

boards := $(wildcard $(srctree)/boards/*/*/*_defconfig)
boards := $(sort $(notdir $(boards)))

kconfig-help:
	@echo  'Configuration targets:'
	@echo  ''
	@$(MAKE) -f $(srctree)/scripts/kconfig/Makefile help
	@echo  ''

help:
	@echo  'Cleaning targets:'
	@echo  '  clean		  - Remove most generated files but keep configuration and backup files'
	@echo  '  mrproper	  - Remove all generated files + config + various backup files'
	@echo  '  distclean	  - mrproper + remove editor backup and patch files'
	@echo  '  pristine	  - Remove the output directory with all generated files'
	@echo  ''
	@echo  'Configuration targets:'
	@echo  ''
	@echo  '  run <make kconfig-help>'
	@echo  ''
	@echo  'Other generic targets:'
	@echo  '  all		  - Build all targets marked with [*]'
	@echo  '* zephyr	  - Build a zephyr application'
	@echo  '  run		  - Build a zephyr application and run it if board supports emulation'
	@echo  '  qemu		  - Build a zephyr application and run it in qemu [deprecated]'
	@echo  '  qemugdb         - Same as 'qemu' but start a GDB server on port 1234 [deprecated]'
	@echo  '  flash		  - Build and flash an application'
	@echo  '  debug		  - Build and debug an application using GDB'
	@echo  '  debugserver	  - Build and start a GDB server (port 1234 for Qemu targets)'
	@echo  '  ram_report	  - Build and create RAM usage report'
	@echo  '  rom_report	  - Build and create ROM usage report'
	@echo  ''
	@echo  'Supported Boards:'
	@echo  ''
	@echo  '  To build an image for one of the supported boards below, run:'
	@echo  ''
	@echo  '  make BOARD=<BOARD NAME>'
	@echo  '  in the application directory.'
	@echo  ''
	@echo  '  To flash the image (if supported), run:'
	@echo  ''
	@echo  '  make BOARD=<BOARD NAME> flash'
	@echo  ''
	@$(if $(boards), \
		$(foreach b, $(boards), \
		printf "  make BOARD=%-24s - Build for %s\\n" $(subst _defconfig,,$(b)) $(subst _defconfig,,$(b));) \
		echo '')

	@echo  ''
	@echo  'Build flags:'
	@echo  ''
	@echo  '  make V=0|1 [targets] 0 => quiet build (default), 1 => verbose build'
	@echo  '  make V=2   [targets] 2 => give reason for rebuild of target'
	@echo  '  make O=dir [targets] Locate all output files in "dir", including .config'
	@echo  '  make C=1   [targets] Check all c source with $$CHECK (sparse by default)'
	@echo  '  make C=2   [targets] Force check of all c source with $$CHECK'
	@echo  '  make RECORDMCOUNT_WARN=1 [targets] Warn about ignored mcount sections'
	@echo  '  make W=n   [targets] Enable extra gcc checks, n=1,2,3 where'
	@echo  '		1: warnings which may be relevant and do not occur too often'
	@echo  '		2: warnings which occur quite often but may still be relevant'
	@echo  '		3: more obscure warnings, can most likely be ignored'
	@echo  '		Multiple levels can be combined with W=12 or W=123'
	@echo  ''
	@echo  'Execute "make" or "make all" to build all targets marked with [*] '


help-board-dirs := $(addprefix help-,$(board-dirs))

help-boards: $(help-board-dirs)

boards-per-dir = $(sort $(notdir $(wildcard $(srctree)/boards/*/$*/*_defconfig)))

$(help-board-dirs): help-%:
	@echo  'Architecture specific targets ($(ARCH) $*):'
	@$(if $(boards-per-dir), \
		$(foreach b, $(boards-per-dir), \
		printf "  %-24s - Build for %s\\n" $*/$(b) $(subst _defconfig,,$(b));) \
		echo '')



host-tools:
	$(Q)$(MAKE) $(build)=scripts/basic
	$(Q)$(MAKE) $(build)=scripts/kconfig standalone
	$(Q)$(MAKE) $(build)=scripts/gen_idt
	$(Q)$(MAKE) $(build)=scripts/gen_offset_header
	@mkdir -p ${ZEPHYR_BASE}/bin
	@cp scripts/basic/fixdep scripts/gen_idt/gen_idt scripts/kconfig/conf \
		scripts/gen_offset_header/gen_offset_header ${ZEPHYR_BASE}/bin


# Documentation targets
# ---------------------------------------------------------------------------
%docs: FORCE
	$(Q)$(MAKE) -C doc htmldocs

clean: clean-dirs-target
	$(call cmd,rmdirs)
	$(call cmd,rmfiles)
	@find $(if $(KBUILD_EXTMOD), $(KBUILD_EXTMOD), .) $(RCS_FIND_IGNORE) \
		\( -name '*.[oas]' -o -name '.*.cmd' \
		-o -name '*.dwo' -o -name '.*.d' -o -name '.*.tmp'  \
		-o -name '.tmp_*.o.*' -o -name '*.gcno' \) -type f \
		-print | xargs rm -f

# Generate tags for editors
# ---------------------------------------------------------------------------
quiet_cmd_tags = GEN     $@
      cmd_tags = $(CONFIG_SHELL) $(srctree)/scripts/tags.sh $@

tags TAGS cscope gtags: FORCE
	$(call cmd,tags)

endif #ifeq ($(config-targets),1)
endif #ifeq ($(mixed-targets),1)

PHONY += checkstack kernelversion image_name

CHECKSTACK_ARCH := $(ARCH)

checkstack:
	$(OBJDUMP) -d $(O)/$(KERNEL_ELF_NAME) | \
	$(PERL) $(src)/scripts/checkstack.pl $(CHECKSTACK_ARCH)

kernelversion:
	@echo $(KERNELVERSION)

image_name:
	@echo $(KBUILD_IMAGE)

# Clear a bunch of variables before executing the submake
tools/: FORCE
	$(Q)mkdir -p $(objtree)/tools
	$(Q)$(MAKE) LDFLAGS= MAKEFLAGS="$(filter --j% -j,$(MAKEFLAGS))" O=$(objtree) subdir=tools -C $(src)/tools/

tools/%: FORCE
	$(Q)mkdir -p $(objtree)/tools
	$(Q)$(MAKE) LDFLAGS= MAKEFLAGS="$(filter --j% -j,$(MAKEFLAGS))" O=$(objtree) subdir=tools -C $(src)/tools/ $*

-include $(srctree)/boards/$(ARCH)/$(BOARD_NAME)/Makefile.board

ifneq ($(EMU_PLATFORM),)
-include $(srctree)/scripts/Makefile.$(EMU_PLATFORM)
else
run:
	@echo ===================================================
	@echo Emulation/Simulation not supported with this board.
	@echo ===================================================
endif

ifneq ($(FLASH_SCRIPT),)
flash: zephyr
	@echo "Flashing $(BOARD_NAME)"
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/support/$(FLASH_SCRIPT) flash
else
flash: FORCE
	@echo Flashing not supported with this board.
	@echo Please check the documentation for alternate instructions.
endif

ifneq ($(DEBUG_SCRIPT),)
debug: zephyr
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/support/$(DEBUG_SCRIPT) debug

else
debug: FORCE
	@echo Debugging not supported with this board.
	@echo Please check the documentation for alternate instructions.
endif

# Single targets
# ---------------------------------------------------------------------------
# Single targets are compatible with:
# - build with mixed source and output
# - build with separate output dir 'make O=...'
#
#  target-dir => where to store outputfile
#  build-dir  => directory in kernel source tree to use

        build-dir  = $(patsubst %/,%,$(dir $@))
        target-dir = $(dir $@)

%.s: %.c prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.i: %.c prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.o: %.c prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.o: %.cpp prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.o: %.cxx prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.lst: %.c prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.s: %.S prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.o: %.S prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)
%.symtypes: %.c prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir) $(target-dir)$(notdir $@)

# Modules
/: prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir)

%/: prepare scripts FORCE
	$(Q)$(MAKE) $(build)=$(build-dir)

# FIXME Should go into a make.lib or something
# ===========================================================================

quiet_cmd_rmdirs = $(if $(wildcard $(rm-dirs)),CLEAN   $(wildcard $(rm-dirs)))
      cmd_rmdirs = rm -rf $(rm-dirs)

quiet_cmd_rmfiles = $(if $(wildcard $(rm-files)),CLEAN   $(wildcard $(rm-files)))
      cmd_rmfiles = rm -f $(rm-files)

# read all saved command lines

targets := $(wildcard $(sort $(targets)))
cmd_files := $(wildcard .*.cmd $(foreach f,$(targets),$(dir $(f)).$(notdir $(f)).cmd))

ifneq ($(cmd_files),)
  $(cmd_files): ;	# Do not try to update included dependency files
  include $(cmd_files)
endif

endif	# skip-makefile

PHONY += FORCE
FORCE:

# Declare the contents of the .PHONY variable as phony.  We keep that
# information in a variable so we can use it in if_changed and friends.
.PHONY: $(PHONY)
