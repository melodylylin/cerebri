#-------------------------------------------------------------------------------
# Zephyr Cerebri Application
#
# Copyright (c) 2023 CogniPilot Foundation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(zros_minimal LANGUAGES C)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(app PRIVATE -Wall -Wextra -Wno-unused-parameter -Werror)

target_sources(app PRIVATE
  src/main.c
  src/publisher.c
  src/subscriber.c
  src/topic_list.c
  )
