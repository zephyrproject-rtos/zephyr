# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Native Simulator (NSI) Makefile.
# It builds the simulator runner itself, and produces the final
# Linux executable by linking it to the embedded cpu library

# By default all the build output is placed under the _build folder, but the user can override it
# setting the NSI_BUILD_PATH
#
# The caller can provide an optional configuration file and point to it with NSI_CONFIG_FILE
# See "Configurable/user overridible variables" below.
#
# By default only the core of the runner will be built, but the caller can set NSI_NATIVE
# to also build the components in native/src/

NSI_CONFIG_FILE?=nsi_config
-include ${NSI_CONFIG_FILE}
#If the file does not exist, we don't use it as a build dependency
NSI_CONFIG_FILE:=$(wildcard ${NSI_CONFIG_FILE})

# Configurable/user overridible variables:
#  Path to the native_simulator (this folder):
NSI_PATH?=./
#  Folder where the build output will be placed
NSI_BUILD_PATH?=$(abspath _build/)
EXE_NAME?=native_simulator.exe
#  Final executable path/file_name which will be produced
NSI_EXE?=${NSI_BUILD_PATH}/${EXE_NAME}
# Number of embedded CPUs/MCUs
NSI_N_CPUS?=1
# Path to all CPUs embedded SW which will be linked with the final executable
NSI_EMBEDDED_CPU_SW?=
#  Host architecture configuration switch
NSI_ARCH?=-m32
#  Coverage switch (GCOV coverage is enabled by default)
NSI_COVERAGE?=--coverage
NSI_LOCALIZE_OPTIONS?=
NSI_BUILD_OPTIONS?=${NSI_ARCH} ${NSI_COVERAGE}
NSI_LINK_OPTIONS?=${NSI_ARCH} ${NSI_COVERAGE}
NSI_BUILD_C_OPTIONS?=
NSI_BUILD_CXX_OPTIONS?=
#  Extra source files to be built in the runner context
NSI_EXTRA_SRCS?=
#  Extra include directories to be used while building in the runner context
NSI_EXTRA_INCLUDES?=
#  Extra libraries to be linked to the final executable
NSI_EXTRA_LIBS?=

SHELL?=bash
#  Compiler
NSI_CC?=gcc
NSI_CXX?=g++
NSI_LINKER?=${NSI_CC}
#  Archive program (it is unlikely you'll need to change this)
NSI_AR?=ar
#  Objcopy program (it is unlikely you'll need to change this)
NSI_OBJCOPY?=objcopy

#  Build debug switch (by default enabled)
NSI_DEBUG?=-g
#  Build optimization level (by default disabled to ease debugging)
NSI_OPT?=-O0
#  Warnings switches (for the runner itself)
NSI_WARNINGS?=-Wall -Wpedantic
#  Preprocessor flags
NSI_CPPFLAGS?=-D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=600 -D_XOPEN_SOURCE_EXTENDED

NO_PIE_CO:=-fno-pie -fno-pic
DEPENDFLAGS:=-MMD -MP
COMMON_BUILD_FLAGS:=-DNSI_RUNNER_BUILD ${NSI_DEBUG} ${NSI_WARNINGS} ${NSI_OPT} ${NO_PIE_CO} \
  -DNSI_N_CPUS=${NSI_N_CPUS} -ffunction-sections -fdata-sections ${DEPENDFLAGS} ${NSI_BUILD_OPTIONS}
CFLAGS:=-std=c11 ${COMMON_BUILD_FLAGS} ${NSI_BUILD_C_OPTIONS}
CXXFLAGS:=${COMMON_BUILD_FLAGS} ${NSI_BUILD_CXX_OPTIONS}
FINALLINK_FLAGS:=${NO_PIE_CO} -no-pie  ${NSI_WARNINGS} \
  -Wl,--gc-sections -ldl -pthread \
  ${NSI_LINK_OPTIONS} -lm

no_default:
	@echo "There is no default rule, please specify what you want to build,\
 or run make help for more info"

RUNNER_LIB:=runner.a

SRCS:=$(shell ls ${NSI_PATH}common/src/*.c)
ifdef NSI_NATIVE
  SRCS+=$(shell ls ${NSI_PATH}native/src/*.c)
endif

INCLUDES:=-I${NSI_PATH}common/src/include/ \
 -I${NSI_PATH}common/src \
 ${NSI_EXTRA_INCLUDES}

ifdef NSI_NATIVE
  INCLUDES+=-I${NSI_PATH}native/src/include/
endif

EXTRA_OBJS:=$(abspath $(addprefix $(NSI_BUILD_PATH)/,$(sort $(patsubst %.c,%.o,${NSI_EXTRA_SRCS:%.cpp=%.c}))))
OBJS:=$(abspath $(addprefix $(NSI_BUILD_PATH)/,${SRCS:${NSI_PATH}%.c=%.o})) ${EXTRA_OBJS}

DEPENDFILES:=$(addsuffix .d,$(basename ${OBJS}))

-include ${DEPENDFILES}

LOCALIZED_EMBSW:=$(abspath $(addprefix $(NSI_BUILD_PATH)/,$(addsuffix .loc_cpusw.o,${NSI_EMBEDDED_CPU_SW})))

${NSI_BUILD_PATH}:
	@if [ ! -d ${NSI_BUILD_PATH} ]; then mkdir -p ${NSI_BUILD_PATH}; fi

#Extra sources build:
${NSI_BUILD_PATH}/%.o: /%.c ${NSI_PATH}Makefile ${NSI_CONFIG_FILE}
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	${NSI_CC} ${NSI_CPPFLAGS} ${INCLUDES} ${CFLAGS} -c $< -o $@

#Extra C++ sources build:
${NSI_BUILD_PATH}/%.o: /%.cpp ${NSI_PATH}Makefile ${NSI_CONFIG_FILE}
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	${NSI_CXX} ${NSI_CPPFLAGS} ${INCLUDES} ${CXXFLAGS} -c $< -o $@

${NSI_BUILD_PATH}/%.o: ${NSI_PATH}/%.c ${NSI_PATH}Makefile ${NSI_CONFIG_FILE}
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	${NSI_CC} ${NSI_CPPFLAGS} ${INCLUDES} ${CFLAGS} -c $< -o $@

${NSI_BUILD_PATH}/linker_script.ld : ${NSI_PATH}/common/other/linker_script.pre.ld | ${NSI_BUILD_PATH}
	${NSI_CC} -x c -E -P $< -o $@  ${DEPENDFLAGS}

${NSI_BUILD_PATH}/${RUNNER_LIB}: ${OBJS}
	if [ -f $@ ]; then rm $@ ; fi
	${NSI_AR} -cr $@ ${OBJS}

${NSI_BUILD_PATH}/%.loc_cpusw.o: /% ${NSI_CONFIG_FILE}
	@if [ -z $< ] || [ ! -f $< ]; then \
	  echo "Error: Input embedded CPU SW ($<) not found \
(NSI_EMBEDDED_CPU_SW=${NSI_EMBEDDED_CPU_SW} )"; \
	  false; \
	fi
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	${NSI_OBJCOPY} --localize-hidden $< $@ -w --localize-symbol=_* ${NSI_LOCALIZE_OPTIONS}

${NSI_EXE}: ${NSI_BUILD_PATH}/${RUNNER_LIB} ${LOCALIZED_EMBSW} ${NSI_EXTRA_LIBS} \
 ${NSI_BUILD_PATH}/linker_script.ld
	${NSI_LINKER} -Wl,--whole-archive ${LOCALIZED_EMBSW} ${NSI_BUILD_PATH}/${RUNNER_LIB} \
	  ${NSI_EXTRA_LIBS} -Wl,--no-whole-archive \
	  -o $@ ${FINALLINK_FLAGS} -T ${NSI_BUILD_PATH}/linker_script.ld

Makefile: ;

link_with_esw: ${NSI_EXE};

runner_lib: ${NSI_BUILD_PATH}/${RUNNER_LIB}

all: link_with_esw

clean:
	@echo "Deleting intermediate compilation results + libraries + executables (*.d .o .a .exe)"
	find $(NSI_BUILD_PATH) -name "*.o" -or -name "*.exe" -or -name "*.a" -or -name "*.d" | xargs rm -f

clean_coverage:
	find $(NSI_BUILD_PATH) -name "*.gcda" -or -name "*.gcno" | xargs rm -f ; true

clean_all: clean clean_coverage ;

.PHONY: clean clean_coverage clean_all link_with_esw runner_lib no_default all ${DEPENDFILES}

ifndef NSI_BUILD_VERBOSE
.SILENT:
endif

help:
	@echo "*******************************"
	@echo "* Native Simulator makefile *"
	@echo "*******************************"
	@echo "Provided rules:"
	@echo " clean           : clean all build output"
	@echo " clean_coverage  : clean all coverage files"
	@echo " clean_all       : clean + clean_coverage"
	@echo " link_with_esw   : Link the runner with the CPU embedded sw"
	@echo " runner_lib      : Build the runner itself (pending the embedded SW)"
	@echo " all             : link_with_esw"
	@echo "Note that you can use TAB to autocomplete rules in the command line in modern OSs"
