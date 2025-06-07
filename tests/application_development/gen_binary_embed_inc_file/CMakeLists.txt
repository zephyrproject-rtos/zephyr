# Copyright (c) 2025 Siemens AG
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gen_binary_embed_inc_file)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

set(gen_dir ${ZEPHYR_BINARY_DIR}/misc/generated/binary_embed_test)
zephyr_include_directories(${gen_dir})

# Generate the test binary files on the fly to avoid placing them in the Zephyr repo.

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/data/file1.bin
  COMMAND
    ${PYTHON_EXECUTABLE} -c
    "file=open('data/file1.bin', 'wb'); \
     exec(\"for i in range(255): file.write(bytes('%c' % (i + 1), 'charmap'))\"); \
     file.close()"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  VERBATIM
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/data/file2.bin
  COMMAND
    ${PYTHON_EXECUTABLE} -c
    "file=open('data/file2.bin', 'wb'); \
     exec(\"for i in range(255): file.write(bytes('%c' % (254 - i + 1), 'charmap'))\"); \
     file.close()"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  VERBATIM
)

set(source_file data/file1.bin)
set(source_files data/file1.bin data/file2.bin)

# Test inclusion of data with default settings.

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/file.embed.inc
  BINARY_FILES  ${source_file}
  SYMBOL_NAMES  inc_file
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/files.embed.inc
  BINARY_FILES  ${source_files}
  SYMBOL_NAMES  inc_file1 inc_file2
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/file.embed.partial.inc
  BINARY_FILES  ${source_file}
  SYMBOL_NAMES  partial_inc_file
  OFFSET        52
  LENGTH        37
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/files.embed.partial.inc
  BINARY_FILES  ${source_files}
  SYMBOL_NAMES  partial_inc_file1 partial_inc_file2
  OFFSET        52
  LENGTH        37
)

# Test inclusion of data as writable.

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/file.embed.rw.inc
  BINARY_FILES  ${source_file}
  SYMBOL_NAMES  rw_inc_file
  READONLY      no
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/files.embed.rw.inc
  BINARY_FILES  ${source_files}
  SYMBOL_NAMES  rw_inc_file1 rw_inc_file2
  READONLY      no
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/file.embed.rw.partial.inc
  BINARY_FILES  ${source_file}
  SYMBOL_NAMES  partial_rw_inc_file
  READONLY      no
  OFFSET        52
  LENGTH        37
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/files.embed.rw.partial.inc
  BINARY_FILES  ${source_files}
  SYMBOL_NAMES  partial_rw_inc_file1 partial_rw_inc_file2
  READONLY      no
  OFFSET        52
  LENGTH        37
)

# Test inclusion of data with alignment specified.

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/file.embed.align.inc
  BINARY_FILES  ${source_file}
  SYMBOL_NAMES  aligned_inc_file
  ALIGN         32
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/files.embed.align.inc
  BINARY_FILES  ${source_files}
  SYMBOL_NAMES  aligned_inc_file1 aligned_inc_file2
  ALIGN         32
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/file.embed.align.partial.inc
  BINARY_FILES  ${source_file}
  SYMBOL_NAMES  partial_aligned_inc_file
  ALIGN         32
  OFFSET        52
  LENGTH        37
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/files.embed.align.partial.inc
  BINARY_FILES  ${source_files}
  SYMBOL_NAMES  partial_aligned_inc_file1 partial_aligned_inc_file2
  ALIGN         32
  OFFSET        52
  LENGTH        37
)

# Test inclusion of data as writable, with alignment specified.

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/file.embed.align.rw.inc
  BINARY_FILES  ${source_file}
  SYMBOL_NAMES  rw_aligned_inc_file
  ALIGN         32
  READONLY      no
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/files.embed.align.rw.inc
  BINARY_FILES  ${source_files}
  SYMBOL_NAMES  rw_aligned_inc_file1 rw_aligned_inc_file2
  ALIGN         32
  READONLY      no
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/file.embed.align.rw.partial.inc
  BINARY_FILES  ${source_file}
  SYMBOL_NAMES  partial_rw_aligned_inc_file
  ALIGN         32
  OFFSET        52
  LENGTH        37
  READONLY      no
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/files.embed.align.rw.partial.inc
  BINARY_FILES  ${source_files}
  SYMBOL_NAMES  partial_rw_aligned_inc_file1 partial_rw_aligned_inc_file2
  ALIGN         32
  OFFSET        52
  LENGTH        37
  READONLY      no
)

# Test inclusion of data into a custom linker section.

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/file.embed.section.inc
  BINARY_FILES  ${source_file}
  SYMBOL_NAMES  section_inc_file
  SECTION       test_section
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/files.embed.section.inc
  BINARY_FILES  ${source_files}
  SYMBOL_NAMES  section_inc_file1 section_inc_file2
  SECTION       test_section
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/file.embed.section.partial.inc
  BINARY_FILES  ${source_file}
  SYMBOL_NAMES  partial_section_inc_file
  SECTION       test_section
  OFFSET        52
  LENGTH        37
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/files.embed.section.partial.inc
  BINARY_FILES  ${source_files}
  SYMBOL_NAMES  partial_section_inc_file1 partial_section_inc_file2
  SECTION       test_section
  OFFSET        52
  LENGTH        37
)

# Test inclusion of data as writable, into a custom linker section.

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/file.embed.section.rw.inc
  BINARY_FILES  ${source_file}
  SYMBOL_NAMES  rw_section_inc_file
  SECTION       test_rw_section
  READONLY      no
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/files.embed.section.rw.inc
  BINARY_FILES  ${source_files}
  SYMBOL_NAMES  rw_section_inc_file1 rw_section_inc_file2
  SECTION       test_rw_section
  READONLY      no
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/file.embed.section.rw.partial.inc
  BINARY_FILES  ${source_file}
  SYMBOL_NAMES  partial_rw_section_inc_file
  SECTION       test_rw_section
  READONLY      no
  OFFSET        52
  LENGTH        37
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/files.embed.section.rw.partial.inc
  BINARY_FILES  ${source_files}
  SYMBOL_NAMES  partial_rw_section_inc_file1 partial_rw_section_inc_file2
  SECTION       test_rw_section
  READONLY      no
  OFFSET        52
  LENGTH        37
)


# Test inclusion of data into a custom linker section, with alignment specified.

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/file.embed.align.section.inc
  BINARY_FILES  ${source_file}
  SYMBOL_NAMES  aligned_section_inc_file
  ALIGN         32
  SECTION       test_section
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/files.embed.align.section.inc
  BINARY_FILES  ${source_files}
  SYMBOL_NAMES  aligned_section_inc_file1 aligned_section_inc_file2
  ALIGN         32
  SECTION       test_section
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/file.embed.align.section.partial.inc
  BINARY_FILES  ${source_file}
  SYMBOL_NAMES  partial_aligned_section_inc_file
  ALIGN         32
  SECTION       test_section
  OFFSET        52
  LENGTH        37
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/files.embed.align.section.partial.inc
  BINARY_FILES  ${source_files}
  SYMBOL_NAMES  partial_aligned_section_inc_file1 partial_aligned_section_inc_file2
  ALIGN         32
  SECTION       test_section
  OFFSET        52
  LENGTH        37
)

# Test inclusion of data as writable, into a custom linker section, with
# alignment also specified.

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/file.embed.section.align.rw.inc
  BINARY_FILES  ${source_file}
  SYMBOL_NAMES  rw_aligned_section_inc_file
  SECTION       test_rw_section
  READONLY      no
  ALIGN         32
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/files.embed.section.align.rw.inc
  BINARY_FILES  ${source_files}
  SYMBOL_NAMES  rw_aligned_section_inc_file1 rw_aligned_section_inc_file2
  SECTION       test_rw_section
  READONLY      no
  ALIGN         32
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/file.embed.section.align.rw.partial.inc
  BINARY_FILES  ${source_file}
  SYMBOL_NAMES  partial_rw_aligned_section_inc_file
  SECTION       test_rw_section
  READONLY      no
  ALIGN         32
  OFFSET        52
  LENGTH        37
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/files.embed.section.align.rw.partial.inc
  BINARY_FILES  ${source_files}
  SYMBOL_NAMES  partial_rw_aligned_section_inc_file1 partial_rw_aligned_section_inc_file2
  SECTION       test_rw_section
  READONLY      no
  ALIGN         32
  OFFSET        52
  LENGTH        37
)

# Test inclusion of data compressed with gzip, without mtime.

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/file.embed.gz.inc
  BINARY_FILES  ${source_file}
  SYMBOL_NAMES  no_mtime_gz_inc_file
  COMPRESS      gzip
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/files.embed.gz.inc
  BINARY_FILES  ${source_files}
  SYMBOL_NAMES  no_mtime_gz_inc_file1 no_mtime_gz_inc_file2
  COMPRESS      gzip
)

# Test inclusion of data compressed with gzip, with mtime specified.

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/file.embed.mtime.gz.inc
  BINARY_FILES  ${source_file}
  SYMBOL_NAMES  mtime_gz_inc_file
  COMPRESS      gzip
  GZIP_MTIME    54
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/files.embed.mtime.gz.inc
  BINARY_FILES  ${source_files}
  SYMBOL_NAMES  mtime_gz_inc_file1 mtime_gz_inc_file2
  COMPRESS      gzip
  GZIP_MTIME    54
)

# Test inclusion of data compressed with gzip, with mtime specified as "now".

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/file.embed.mtime.now.gz.inc
  BINARY_FILES  ${source_file}
  SYMBOL_NAMES  mtime_now_gz_inc_file
  COMPRESS      gzip
  GZIP_MTIME    now
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/file.embed.mtime.now.partial.gz.inc
  BINARY_FILES  ${source_file}
  SYMBOL_NAMES  partial_mtime_now_gz_inc_file
  COMPRESS      gzip
  GZIP_MTIME    now
  OFFSET        52
  LENGTH        37
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/files.embed.mtime.now.gz.inc
  BINARY_FILES  ${source_files}
  SYMBOL_NAMES  mtime_now_gz_inc_file1 mtime_now_gz_inc_file2
  COMPRESS      gzip
  GZIP_MTIME    now
)

generate_binary_embed_inc_file(
  OUTPUT        ${gen_dir}/files.embed.mtime.now.partial.gz.inc
  BINARY_FILES  ${source_files}
  SYMBOL_NAMES  partial_mtime_now_gz_inc_file1 partial_mtime_now_gz_inc_file2
  COMPRESS      gzip
  GZIP_MTIME    now
  OFFSET        52
  LENGTH        37
)

# Plug in the linker script snippet to place the custom sections.
if(NOT CONFIG_CMAKE_LINKER_GENERATOR)
  zephyr_linker_sources(SECTIONS sections.ld)
else()
  zephyr_linker_section(NAME .test_section GROUP ROM_REGION NOINPUT ALIGN_WITH_INPUT)
  zephyr_linker_section_configure(SECTION .test_section KEEP INPUT ".test_section*")
  zephyr_linker_section(NAME .test_rw_section GROUP DATA_REGION NOINPUT ALIGN_WITH_INPUT)
  zephyr_linker_section_configure(SECTION .test_rw_section KEEP INPUT ".test_rw_section*")
endif()
