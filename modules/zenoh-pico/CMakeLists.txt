# SPDX-License-Identifier: Apache-2.0
if (CONFIG_ZENOH_PICO)

add_compile_definitions(ZENOH_ZEPHYR)
add_compile_options(-Wno-unused-variable)

zephyr_library_named(zenoh-pico)

#message(STATUS "zenoh pico module dir: ${ZEPHYR_CURRENT_MODULE_DIR}")

zephyr_include_directories(
  ${ZEPHYR_CURRENT_MODULE_DIR}/include
  )

zephyr_library_include_directories(
  ${ZEPHYR_CURRENT_MODULE_DIR}/include
  )

file(GLOB_RECURSE Sources
  "${ZEPHYR_CURRENT_MODULE_DIR}/src/api/*.c"
  "${ZEPHYR_CURRENT_MODULE_DIR}/src/collections/*.c"
  "${ZEPHYR_CURRENT_MODULE_DIR}/src/link/*.c"
  "${ZEPHYR_CURRENT_MODULE_DIR}/src/net/*.c"
  "${ZEPHYR_CURRENT_MODULE_DIR}/src/protocol/*.c"
  "${ZEPHYR_CURRENT_MODULE_DIR}/src/session/*.c"
  "${ZEPHYR_CURRENT_MODULE_DIR}/src/transport/*.c"
  "${ZEPHYR_CURRENT_MODULE_DIR}/src/utils/*.c"
)

#message(STATUS "Sources: ${Sources}")

zephyr_library_sources(
  ${Sources}
)

endif()
