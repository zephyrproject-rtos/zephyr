VERSION_MAJOR 	   = 0
VERSION_MINOR 	   = 7
PATCHLEVEL 	   = 0
VERSION_RESERVED   = 0
EXTRAVERSION       = -rc2
NAME 		   = Zephyr Kernel

export SOURCE_DIR PROJECT MDEF_FILE KLIBC_DIR

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

# Avoid funny character set dependencies
unexport LC_ALL
LC_COLLATE=C
LC_NUMERIC=C
export LC_COLLATE LC_NUMERIC

# Avoid interference with shell env settings
unexport GREP_OPTIONS

DQUOTE = "
#This comment line is to fix the highlighting of some editors due the quote effect."

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
								&& /bin/pwd)
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

VPATH		:= $(srctree)

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

# Architecture as present in compile.h
UTS_MACHINE 	:= $(subst $(DQUOTE),,$(CONFIG_ARCH))
SRCARCH 	= $(subst $(DQUOTE),,$(CONFIG_ARCH))


# Where to locate arch specific headers
hdr-arch  := $(SRCARCH)

KCONFIG_CONFIG	?= .config
export KCONFIG_CONFIG

# SHELL used by kbuild
CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)

HOSTCC       = gcc
HOSTCXX      = g++
HOSTCFLAGS   = -Wall -Wmissing-prototypes -Wstrict-prototypes -O2 -fomit-frame-pointer -std=gnu89
HOSTCXXFLAGS = -O2

ifeq ($(shell $(HOSTCC) -v 2>&1 | grep -c "clang version"), 1)
HOSTCFLAGS  += -Wno-unused-value -Wno-unused-parameter \
		-Wno-missing-field-initializers -fno-delete-null-pointer-checks
endif

# Decide whether to build built-in, modular, or both.
# Normally, just do built-in.

KBUILD_MODULES :=
KBUILD_BUILTIN := 1


export KBUILD_MODULES KBUILD_BUILTIN
export KBUILD_CHECKSRC KBUILD_SRC

ifneq ($(CC),)
ifeq ($(shell $(CC) -v 2>&1 | grep -c "clang version"), 1)
COMPILER := clang
else
COMPILER := gcc
endif
export COMPILER
endif

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
else
CC		= $(CROSS_COMPILE)gcc
endif
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
AWK		= awk
GENKSYMS	= scripts/genksyms/genksyms
GENIDT		= scripts/gen_idt/gen_idt
GENOFFSET_H	= scripts/gen_offset_header/gen_offset_header
PERL		= perl
PYTHON		= python
CHECK		= sparse

CHECKFLAGS     := -Wbitwise -Wno-return-void $(CF)
CFLAGS_MODULE   =
AFLAGS_MODULE   =
LDFLAGS_MODULE  =
CFLAGS_KERNEL	=
AFLAGS_KERNEL	=
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
PROJECTINCLUDE := $(strip -I$(srctree)/include/microkernel \
		-I$(CURDIR)/misc/generated/sysgen) \
		$(USERINCLUDE)

# Use ZEPHYRINCLUDE when you must reference the include/ directory.
# Needed to be compatible with the O= option
ZEPHYRINCLUDE    = \
		-I$(srctree)/arch/$(SRCARCH)/include \
		$(if $(KBUILD_SRC), -I$(srctree)/include) \
		-I$(srctree)/include \
		-I$(CURDIR)/include/generated \
		-I$(CURDIR)/misc/generated/sysgen \
		$(USERINCLUDE) \
		$(STDINCLUDE)
ZEPHYRINCLUDE    +=  -I$(srctree)/arch/$(subst $(DQUOTE),,$(CONFIG_ARCH))/include

KBUILD_CPPFLAGS := -DKERNEL

KBUILD_CFLAGS   := -c -g -std=c99 \
		-fno-asynchronous-unwind-tables \
		-fno-omit-frame-pointer \
		-Wall \
		-Wno-format-zero-length \
		-Wno-main -ffreestanding

KBUILD_AFLAGS_KERNEL :=
KBUILD_CFLAGS_KERNEL :=
KBUILD_AFLAGS   := -c -g -xassembler-with-cpp

LDFLAGS += $(call ld-option,-nostartfiles)
LDFLAGS += $(call ld-option,-nodefaultlibs)
LDFLAGS += $(call ld-option,-nostdlib)
LDFLAGS += $(call ld-option,-static)
LDLIBS_TOOLCHAIN ?= -lgcc

KERNELVERSION = $(VERSION_MAJOR)$(if $(VERSION_MINOR),.$(VERSION_MINOR)$(if $(PATCHLEVEL),.$(PATCHLEVEL)))$(EXTRAVERSION)

export VERSION_MAJOR VERSION_MINOR PATCHLEVEL VERSION_RESERVED EXTRAVERSION
export KERNELRELEASE KERNELVERSION
export ARCH SRCARCH CONFIG_SHELL HOSTCC HOSTCFLAGS CROSS_COMPILE AS LD CC
export CPP AR NM STRIP OBJCOPY OBJDUMP
export MAKE AWK GENKSYMS INSTALLKERNEL PERL PYTHON UTS_MACHINE GENIDT GENOFFSET_H
export HOSTCXX HOSTCXXFLAGS LDFLAGS_MODULE CHECK CHECKFLAGS

export KBUILD_CPPFLAGS NOSTDINC_FLAGS ZEPHYRINCLUDE OBJCOPYFLAGS LDFLAGS
export KBUILD_CFLAGS CFLAGS_KERNEL CFLAGS_MODULE CFLAGS_GCOV
export KBUILD_AFLAGS AFLAGS_KERNEL AFLAGS_MODULE
export KBUILD_AFLAGS_MODULE KBUILD_CFLAGS_MODULE KBUILD_LDFLAGS_MODULE
export KBUILD_AFLAGS_KERNEL KBUILD_CFLAGS_KERNEL
export KBUILD_ARFLAGS PROJECTINCLUDE LDLIBS_TOOLCHAIN


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
scripts_basic:
	$(Q)$(MAKE) $(build)=scripts/basic
	$(Q)$(MAKE) $(build)=scripts/gen_idt
	$(Q)$(MAKE) $(build)=scripts/gen_offset_header
	$(Q)rm -f .tmp_quiet_recordmcount

# To avoid any implicit rule to kick in, define an empty command.
scripts/basic/%: scripts_basic ;

PHONY += outputmakefile
# outputmakefile generates a Makefile in the output directory, if using a
# separate output directory. This allows convenient use of make in the
# output directory.
outputmakefile:
ifneq ($(KBUILD_SRC),)
	$(Q)ln -fsn $(srctree) source
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/mkmakefile \
	    $(srctree) $(objtree) $(VERSION_MAJOR) $(VERSION_MINOR)
endif

# Support for using generic headers in asm-generic
PHONY += asm-generic
asm-generic:
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.asm-generic \
	            src=asm obj=arch/$(SRCARCH)/include/generated/asm
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.asm-generic \
	            src=uapi/asm obj=arch/$(SRCARCH)/include/generated/uapi/asm

# To make sure we do not include .config for any of the *config targets
# catch them early, and hand them over to scripts/kconfig/Makefile
# It is allowed to specify more targets when calling make, including
# mixing *config targets and build targets.
# For example 'make oldconfig all'.
# Detect when mixed targets is specified, and make a second invocation
# of make so .config is not included in this case either (for *config).

version_h := include/generated/version.h

no-dot-config-targets := pristine distclean clean mrproper help \
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


core-y := lib/ arch/ kernel/ misc/ net/
drivers-y := drivers/

ifneq ($(strip $(PROJECT)),)
-include $(PROJECT)/Makefile.app
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

ifdef CONFIG_TINYCRYPT
# Objects we will link into the kernel / subdirs we need to visit
KCRYPTO_DIR := lib/crypto/tinycrypt
libs-y += $(KCRYPTO_DIR)/
 ZEPHYRINCLUDE += -I$(srctree)/lib/crypto/tinycrypt/include
endif

ARCH = $(subst $(DQUOTE),,$(CONFIG_ARCH))
export ARCH
ifdef ZEPHYR_GCC_VARIANT
include $(srctree)/scripts/Makefile.toolchain.$(ZEPHYR_GCC_VARIANT)
else
$(if $(CROSS_COMPILE),, \
     $(error ZEPHYR_GCC_VARIANT is not set. ))
endif

ifdef CONFIG_MINIMAL_LIBC
ZEPHYRINCLUDE += -I$(srctree)/lib/libc/minimal/include
endif

ifdef CONFIG_NEWLIB_LIBC
ZEPHYRINCLUDE += $(TOOLCHAIN_CFLAGS)
ALL_LIBS += c m
endif

QEMU_BIN_PATH	?= /usr/bin
QEMU		= $(QEMU_BIN_PATH)/$(QEMU_$(SRCARCH))

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

# Handle stack protector mode.
#
# Since kbuild can potentially perform two passes (first with the old
# .config values and then with updated .config values), we cannot error out
# if a desired compiler option is unsupported. If we were to error, kbuild
# could never get to the second pass and actually notice that we changed
# the option to something that was supported.
#
# Additionally, we don't want to fallback and/or silently change which compiler
# flags will be used, since that leads to producing kernels with different
# security feature characteristics depending on the compiler used. ("But I
# selected CC_STACKPROTECTOR_STRONG! Why did it build with _REGULAR?!")
#
# The middle ground is to warn here so that the failed option is obvious, but
# to let the build fail with bad compiler flags so that we can't produce a
# kernel when there is a CONFIG and compiler mismatch.
#
ifdef CONFIG_CC_STACKPROTECTOR_REGULAR
  stackp-flag := -fstack-protector
  ifeq ($(call cc-option, $(stackp-flag)),)
    $(warning Cannot use CONFIG_CC_STACKPROTECTOR_REGULAR: \
             -fstack-protector not supported by compiler)
  endif
else
ifdef CONFIG_CC_STACKPROTECTOR_STRONG
  stackp-flag := -fstack-protector-strong
  ifeq ($(call cc-option, $(stackp-flag)),)
    $(warning Cannot use CONFIG_CC_STACKPROTECTOR_STRONG: \
	      -fstack-protector-strong not supported by compiler)
  endif
endif
endif
KBUILD_CFLAGS += $(stackp-flag)

ifeq ($(CONFIG_DEBUG),y)
KBUILD_CFLAGS  += -O0
else
KBUILD_CFLAGS  += -Os
endif

KBUILD_CFLAGS += $(subst $(DQUOTE),,$(CONFIG_COMPILER_OPT))

export LDFLAG_LINKERCMD OUTPUT_FORMAT OUTPUT_ARCH

include arch/$(SRCARCH)/Makefile

KBUILD_CFLAGS += $(CFLAGS)
KBUILD_AFLAGS += $(CFLAGS)


ifeq ($(COMPILER),clang)
KBUILD_CPPFLAGS += $(call cc-option,-Qunused-arguments,)
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
KBUILD_CFLAGS += $(call cc-option,-fno-defer-pop)
endif

# We trigger additional mismatches with less inlining
ifdef CONFIG_DEBUG_SECTION_MISMATCH
KBUILD_CFLAGS += $(call cc-option, -fno-inline-functions-called-once)
endif

# arch Makefile may override CC so keep this after arch Makefile is included
NOSTDINC_FLAGS += -nostdinc -isystem $(shell $(CC) -print-file-name=include)
CHECKFLAGS     += $(NOSTDINC_FLAGS)

# disable pointer signed / unsigned warnings in gcc 4.0
KBUILD_CFLAGS += $(call cc-disable-warning, pointer-sign)

# disable invalid "can't wrap" optimizations for signed / pointers
KBUILD_CFLAGS	+= $(call cc-option,-fno-strict-overflow)

# generate an extra file that specifies the maximum amount of stack used,
# on a per-function basis.
KBUILD_CFLAGS	+= $(call cc-option,-fstack-usage)


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

LDFLAGS_zephyr += $(call ld-option,-nostartfiles)
LDFLAGS_zephyr += $(call ld-option,-nodefaultlibs)
LDFLAGS_zephyr += $(call ld-option,-nostdlib)
LDFLAGS_zephyr += $(call ld-option,-static)
LDFLAGS_zephyr += $(call ld-option,-X)
LDFLAGS_zephyr += $(call ld-option,-N)
LDFLAGS_zephyr += $(call ld-option,--gc-sections)
LDFLAGS_zephyr += $(call ld-option,--build-id=none)

LD_TOOLCHAIN ?= -D__GCC_LINKER_CMD__

KERNEL_NAME=$(subst $(DQUOTE),,$(CONFIG_KERNEL_BIN_NAME))

export LD_TOOLCHAIN KERNEL_NAME

# Default kernel image to build when no specific target is given.
# KBUILD_IMAGE may be overruled on the command line or
# set in the environment
# Also any assignments in arch/$(ARCH)/Makefile take precedence over
# this default value
export KBUILD_IMAGE ?= zephyr

#
# INSTALL_PATH specifies where to place the updated kernel and system map
# images. Default is /boot, but you can set it to other values
export	INSTALL_PATH ?= /boot

#
# INSTALL_DTBS_PATH specifies a prefix for relocations required by build roots.
# Like INSTALL_MOD_PATH, it isn't defined in the Makefile, but can be passed as
# an argument if needed. Otherwise it defaults to the kernel install path
#
export INSTALL_DTBS_PATH ?= $(INSTALL_PATH)/dtbs/$(KERNELRELEASE)

core-y		+=

zephyr-dirs	:= $(patsubst %/,%,$(filter %/, $(core-y) $(drivers-y) \
		     $(libs-y) $(app-y)))

zephyr-alldirs	:= $(sort $(zephyr-dirs) $(patsubst %/,%,$(filter %/, \
		     $(init-) $(core-) $(drivers-) $(libs-) $(app-))))

init-y		:= $(patsubst %/, %/built-in.o, $(init-y))
core-y		:= $(patsubst %/, %/built-in.o, $(core-y))
app-y		:= $(patsubst %/, %/built-in.o, $(app-y))
drivers-y	:= $(patsubst %/, %/built-in.o, $(drivers-y))
libs-y1		:= $(patsubst %/, %/lib.a, $(libs-y))
libs-y2		:= $(patsubst %/, %/built-in.o, $(libs-y))
libs-y		:= $(libs-y1) $(libs-y2)

PLATFORM_NAME = $(subst $(DQUOTE),,$(CONFIG_PLATFORM))
SOC_NAME = $(subst $(DQUOTE),,$(CONFIG_SOC))
ARCH_NAME = $(subst $(DQUOTE),,$(CONFIG_ARCH))
export PLATFORM_NAME SOC_NAME ARCH_NAME

# Externally visible symbols (used by link-zephyr.sh)
export KBUILD_ZEPHYR_MAIN := $(drivers-y) $(core-y) $(libs-y) $(app-y)
ifdef CONFIG_HAVE_CUSTOM_LINKER_SCRIPT
export KBUILD_LDS         := $(subst $(DQUOTE),,$(CONFIG_CUSTOM_LINKER_SCRIPT))
else
export KBUILD_LDS         := $(srctree)/arch/$(SRCARCH)/platforms/$(PLATFORM_NAME)/linker.cmd
endif
export LDFLAGS_zephyr
# used by scripts/pacmage/Makefile
export KBUILD_ALLDIRS := $(sort $(filter-out arch/%,$(zephyr-alldirs)) arch include samples scripts)

zephyr-deps := $(KBUILD_LDS) $(KBUILD_ZEPHYR_MAIN)

ALL_LIBS += $(TOOLCHAIN_LIBS)
export ALL_LIBS

# Final link of zephyr
      cmd_link-zephyr = $(CONFIG_SHELL) $< $(LD) $(LDFLAGS) $(LDFLAGS_zephyr) $(LIB_INCLUDE_DIR) $(ALL_LIBS)
quiet_cmd_link-zephyr = LINK    $@

# Include targets which we want to
# execute if the rest of the kernel build went well.
zephyr: scripts/link-zephyr.sh $(zephyr-deps) $(KBUILD_ZEPHYR_APP) FORCE
	@touch zephyr
ifdef CONFIG_HEADERS_CHECK
	$(Q)$(MAKE) -f $(srctree)/Makefile headers_check
endif

ifneq ($(strip $(PROJECT)),)
	+$(call if_changed,link-zephyr)
endif

# The actual objects are generated when descending,
# make sure no implicit rule kicks in
$(sort $(zephyr-deps)): $(zephyr-dirs) ;

# Handle descending into subdirectories listed in $(zephyr-dirs)
# Preset locale variables to speed up the build process. Limit locale
# tweaks to this spot to avoid wrong language settings when running
# make menuconfig etc.
# Error messages still appears in the original language

PHONY += $(zephyr-dirs)
$(zephyr-dirs): prepare scripts
	$(Q)$(MAKE) $(build)=$@

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
	$(cmd_crmodverdir)

archprepare_common = $(strip \
		prepare1 scripts_basic \
		)


archprepare = $(strip \
		$(archprepare_common) \
		)

# All the preparing..
prepare: $(archprepare)  FORCE
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

CLEAN_FILES += 	misc/generated/sysgen/kernel_main.c \
		misc/generated/sysgen/sysgen.h \
		misc/generated/sysgen/prj.mdef

# Directories & files removed with 'make mrproper'
MRPROPER_DIRS  += include/config usr/include include/generated          \
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

PHONY += $(clean-dirs) clean archclean zephyrclean
$(clean-dirs):
	$(Q)$(MAKE) $(clean)=$(patsubst _clean_%,%,$@)

zephyrclean:
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/link-zephyr.sh clean

clean: archclean zephyrclean

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

boards := $(wildcard $(srctree)/configs/*_defconfig) $(wildcard $(srctree)/arch/*/platforms/*/configs/*_defconfig)
boards := $(sort $(notdir $(boards)))
board-dirs := $(dir $(wildcard $(srctree)/configs/*/*_defconfig))
board-dirs := $(sort $(notdir $(board-dirs:/=)))

help:
	@echo  'Cleaning targets:'
	@echo  '  clean		  - Remove most generated files but keep configuration and backup files'
	@echo  '  mrproper	  - Remove all generated files + config + various backup files'
	@echo  '  distclean	  - mrproper + remove editor backup and patch files'
	@echo  '  pristine	  - Remove the output directory with all generated files'
	@echo  ''
	@echo  'Configuration targets:'
	@$(MAKE) -f $(srctree)/scripts/kconfig/Makefile help
	@echo  ''
	@echo  'Other generic targets:'
	@echo  '  all		  - Build all targets marked with [*]'
	@echo  '* zephyr	  - Build the bare kernel'
	@echo  '  qemu		  - Build the bare kernel and runs the emulation with qemu'
	@echo  ''
	@echo  'Supported platforms:'
	@echo  ''
	@$(if $(boards), \
		$(foreach b, $(boards), \
		printf "  %-24s - Build for %s\\n" $(b) $(subst _defconfig,,$(b));) \
		echo '')
	@$(if $(board-dirs), \
		$(foreach b, $(board-dirs), \
		printf "  %-16s - Show %s-specific targets\\n" help-$(b) $(b);) \
		printf "  %-16s - Show all of the above\\n" help-boards; \
		echo '')

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

boards-per-dir = $(sort $(notdir $(wildcard $(srctree)/arch/$(SRCARCH)/configs/$*/*_defconfig)))

$(help-board-dirs): help-%:
	@echo  'Architecture specific targets ($(SRCARCH) $*):'
	@$(if $(boards-per-dir), \
		$(foreach b, $(boards-per-dir), \
		printf "  %-24s - Build for %s\\n" $*/$(b) $(subst _defconfig,,$(b));) \
		echo '')


# Documentation targets
# ---------------------------------------------------------------------------
%docs: FORCE
	$(Q)$(MAKE) -C doc htmldocs

clean: $(clean-dirs)
	$(call cmd,rmdirs)
	$(call cmd,rmfiles)
	@find $(if $(KBUILD_EXTMOD), $(KBUILD_EXTMOD), .) $(RCS_FIND_IGNORE) \
		\( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' \
		-o -name '*.ko.*' \
		-o -name '*.dwo'  \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \
		-o -name '*.symtypes' \
		-o -name modules.builtin -o -name '.tmp_*.o.*' \
		-o -name '*.gcno' \) -type f -print | xargs rm -f

# Generate tags for editors
# ---------------------------------------------------------------------------
quiet_cmd_tags = GEN     $@
      cmd_tags = $(CONFIG_SHELL) $(srctree)/scripts/tags.sh $@

tags TAGS cscope gtags: FORCE
	$(call cmd,tags)

endif #ifeq ($(config-targets),1)
endif #ifeq ($(mixed-targets),1)

PHONY += checkstack kernelversion image_name

# UML needs a little special treatment here.  It wants to use the host
# toolchain, so needs $(SUBARCH) passed to checkstack.pl.  Everyone
# else wants $(ARCH), including people doing cross-builds, which means
# that $(SUBARCH) doesn't work here.
ifeq ($(ARCH), um)
CHECKSTACK_ARCH := $(SUBARCH)
else
CHECKSTACK_ARCH := $(ARCH)
endif
checkstack:
	$(OBJDUMP) -d zephyr $$(find . -name '*.ko') | \
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

QEMU_FLAGS = $(QEMU_FLAGS_$(SRCARCH)) -pidfile qemu.pid

ifneq ($(QEMU_PIPE),)
    # Send console output to a pipe, used for running automated sanity tests
    QEMU_FLAGS += -serial pipe:$(QEMU_PIPE)
else
    QEMU_FLAGS += -serial mon:stdio
endif

qemu: zephyr
	$(if $(QEMU_PIPE),,@echo "To exit from QEMU enter: 'CTRL+a, x'")
	@echo '[QEMU] CPU: $(QEMU_CPU_TYPE_$(SRCARCH))'
	$(Q)$(QEMU) $(QEMU_FLAGS) $(QEMU_EXTRA_FLAGS) -kernel $(KERNEL_NAME).elf

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
	$(cmd_crmodverdir)
	$(Q)$(MAKE) $(build)=$(build-dir)

%/: prepare scripts FORCE
	$(cmd_crmodverdir)
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
