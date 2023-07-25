# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Native Simulator (NSI) Makefile.
# It builds the simulator runner itself, and produces the final
# Linux executable by linking it to the the embedded cpu library

# By default all the build output is placed under the _build folder, but the user can override it
# setting the NSI_BUILD_PATH

NSI_CONFIG_FILE?=nsi_config
-include ${NSI_CONFIG_FILE}
#If the file does not exist, we don't use it as a build dependency
NSI_CONFIG_FILE:=$(wildcard ${NSI_CONFIG_FILE})

NSI_PATH?=./
NSI_BUILD_PATH?=$(abspath _build/)
EXE_NAME?=native_simulator.exe
NSI_EXE?=${NSI_BUILD_PATH}/${EXE_NAME}
NSI_EMBEDDED_CPU_SW?=
NSI_ARCH?=-m32
NSI_COVERAGE?=--coverage
NSI_BUILD_OPTIONS?=${NSI_ARCH} ${NSI_COVERAGE}
NSI_LINK_OPTIONS?=${NSI_ARCH} ${NSI_COVERAGE}
NSI_EXTRA_SRCS?=
NSI_EXTRA_LIBS?=

SHELL?=bash
NSI_CC?=gcc
NSI_AR?=ar
NSI_OBJCOPY?=objcopy

no_default:
	@echo "There is no default rule, please specify what you want to build,\
 or run make help for more info"

NSI_DEBUG?=-g
NSI_OPT?=-O0
NSI_WARNINGS?=-Wall -Wpedantic
NSI_CPPFLAGS?=-D_POSIX_C_SOURCE=200809 -D_XOPEN_SOURCE=600 -D_XOPEN_SOURCE_EXTENDED
NO_PIE_CO:=-fno-pie -fno-pic
DEPENDFLAGS:=-MMD -MP
CFLAGS:=${NSI_DEBUG} ${NSI_WARNINGS} ${NSI_OPT} ${NO_PIE_CO} \
  -ffunction-sections -fdata-sections ${DEPENDFLAGS} -std=c11 ${NSI_BUILD_OPTIONS}
FINALLINK_FLAGS:=${NO_PIE_CO} -no-pie  ${NSI_WARNINGS} \
  -Wl,--gc-sections -lm -ldl -pthread \
  ${NSI_LINK_OPTIONS}

RUNNER_LIB:=runner.a

SRCS:=$(shell ls ${NSI_PATH}common/src/*.c ${NSI_PATH}native/src/*.c )

INCLUDES:=-I${NSI_PATH}common/src/include/ \
 -I${NSI_PATH}native/src/include/ \
 -I${NSI_PATH}common/src

EXTRA_OBJS:=$(abspath $(addprefix $(NSI_BUILD_PATH)/,$(sort ${NSI_EXTRA_SRCS:%.c=%.o})))
OBJS:=$(abspath $(addprefix $(NSI_BUILD_PATH)/,${SRCS:${NSI_PATH}%.c=%.o})) ${EXTRA_OBJS}

DEPENDFILES:=$(addsuffix .d,$(basename ${OBJS}))

-include ${DEPENDFILES}

${NSI_BUILD_PATH}:
	@if [ ! -d ${NSI_BUILD_PATH} ]; then mkdir -p ${NSI_BUILD_PATH}; fi

#Extra sources build:
${NSI_BUILD_PATH}/%.o: /%.c ${NSI_PATH}Makefile ${NSI_CONFIG_FILE}
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	${NSI_CC} ${NSI_CPPFLAGS} ${INCLUDES} ${CFLAGS} -c $< -o $@

${NSI_BUILD_PATH}/%.o: ${NSI_PATH}/%.c ${NSI_PATH}Makefile ${NSI_CONFIG_FILE}
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	${NSI_CC} ${NSI_CPPFLAGS} ${INCLUDES} ${CFLAGS} -c $< -o $@

${NSI_BUILD_PATH}/linker_script.ld : ${NSI_PATH}/common/other/linker_script.pre.ld | ${NSI_BUILD_PATH}
	${NSI_CC} -x c -E -P $< -o $@  ${DEPENDFLAGS}

${NSI_BUILD_PATH}/${RUNNER_LIB}: ${OBJS}
	if [ -f $@ ]; then rm $@ ; fi
	${NSI_AR} -cr $@ ${OBJS}

${NSI_EXE}: ${NSI_BUILD_PATH}/${RUNNER_LIB} ${NSI_EMBEDDED_CPU_SW} ${NSI_EXTRA_LIBS} \
 ${NSI_BUILD_PATH}/linker_script.ld
	@if [ -z ${NSI_EMBEDDED_CPU_SW} ] || [ ! -f ${NSI_EMBEDDED_CPU_SW} ]; then \
	  echo "Error: Input embedded CPU SW not found (NSI_EMBEDDED_CPU_SW=${NSI_EMBEDDED_CPU_SW} )"; \
	  false; \
	fi
	${NSI_OBJCOPY} --localize-hidden ${NSI_EMBEDDED_CPU_SW} ${NSI_BUILD_PATH}/cpu_0.sw.o \
	  -w --localize-symbol=_*
	${NSI_CC} -Wl,--whole-archive ${NSI_BUILD_PATH}/cpu_0.sw.o ${NSI_BUILD_PATH}/${RUNNER_LIB} \
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
