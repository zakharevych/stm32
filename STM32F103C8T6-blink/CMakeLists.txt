cmake_minimum_required(VERSION 3.22)
project(bluepill_blink C)

set(CMAKE_C_STANDARD 11)

# Ядро плати: Cortex-M3 (STM32F103), набір інструкцій Thumb, без FPU
set(MCU_FLAGS -mcpu=cortex-m3 -mthumb)

add_executable(${PROJECT_NAME}.elf
    src/startup.c
    src/main.c
)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
    ${MCU_FLAGS}
    -Wall -Wextra
    -ffunction-sections -fdata-sections
    -fno-tree-loop-distribute-patterns  # не міняти цикли в стартапі на виклики memcpy/memset
    $<$<CONFIG:Debug>:-O0 -g3>
    $<$<CONFIG:Release>:-Os>
)

target_link_options(${PROJECT_NAME}.elf PRIVATE
    ${MCU_FLAGS}
    -T${CMAKE_SOURCE_DIR}/linker/STM32F103C8.ld
    -nostartfiles                       # стартап пишемо самі
    -Wl,--gc-sections                   # викинути невикористане
    -Wl,-Map=${PROJECT_NAME}.map        # карта пам'яті для вивчення
    -Wl,--print-memory-usage
)

# Після збірки: розмір і .bin для прошивання
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
)
