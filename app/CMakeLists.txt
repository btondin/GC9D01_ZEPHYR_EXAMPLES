# Copyright (c) 2024
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Register the custom GC9D01 driver module before find_package(Zephyr)
# so its DTS bindings and Kconfig are processed during CMake configuration.
list(APPEND EXTRA_ZEPHYR_MODULES
    ${CMAKE_CURRENT_SOURCE_DIR}/../custom_driver_module
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(animated_eye)

# Collect all .c sources under src/ (main.c + eye_renderer.c)
FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

# Add src/eye_data/ so #include "eye_data/defaultEye.h" resolves correctly
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
