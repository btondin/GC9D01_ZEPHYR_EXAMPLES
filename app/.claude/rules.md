# Project Rules - Zephyr RTOS Development

## Context7 Usage

Always use context7 for any Zephyr RTOS related queries including:

- Kernel APIs and configuration
- Device drivers (GPIO, I2C, SPI, UART, etc.)
- Threading and scheduling
- Memory management
- Device Tree configuration
- Kconfig options
- Build system (CMake, West)

Automatically invoke Context7 tools without requiring explicit "use context7" in prompts.

## Project Context

This is a Zephyr RTOS project for learning LVGL concepts using ILI9341 TFT display .

- Target: nRF52832 (custom board files located in C:\nordic\myboards\boards\Bruno\bruno_nrf52832)
- Display: 240*320 ILI9341
- Communication: SPI 

## File Access Rules

- Automatically allow read access to files within the folder: C:\nordic\myboards\boards\Bruno\bruno_nrf52832
- Never modify any file outside the folder: C:\nordic\myapps\LVGL_AULA
- **NEVER modify the file: src/main.c** - This file must be asked everytime needs change
