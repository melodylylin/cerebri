# Copyright (c) 2023, CogniPilot Foundation
# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(cerebri_core_workqueues)


# we need to be able to include generated header files
zephyr_include_directories()

zephyr_library_sources(
  src/workq.c
  )

add_dependencies(app cerebri_core_workqueues)
