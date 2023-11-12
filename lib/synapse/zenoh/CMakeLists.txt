# Copyright (c) 2023, CogniPilot Foundation
# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(cerebri_synapse_zenoh)

zephyr_include_directories()

zephyr_library_sources(
  main.c
  )

target_link_libraries(cerebri_synapse_zenoh PRIVATE zenoh-pico)
