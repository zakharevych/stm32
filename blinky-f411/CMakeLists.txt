cmake_minimum_required(VERSION 3.21)

project(blinky_f411 LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)          # gnu11: потрібно для __asm

# ---- Прапорці ядра: Cortex-M4 з FPU одинарної точності ---------------------
set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker/stm32f411re.ld)

add_executable(${PROJECT_NAME}
    src/startup.c
    src/main.c
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    SUFFIX ".elf"
    LINK_DEPENDS ${LINKER_SCRIPT}   # перелінковувати при зміні .ld
)

target_compile_options(${PROJECT_NAME} PRIVATE
    ${MCU_FLAGS}
    -Wall -Wextra -Wpedantic
    -ffunction-sections -fdata-sections   # кожна функція/змінна — окрема секція...
    -g3
    $<$<CONFIG:Debug>:-Og>
    $<$<CONFIG:Release>:-Os>
)

target_link_options(${PROJECT_NAME} PRIVATE
    ${MCU_FLAGS}
    -T${LINKER_SCRIPT}
    -nostartfiles                         # точка входу і startup — наші власні
    --specs=nano.specs                    # newlib-nano: компілятор може сам вставити
    --specs=nosys.specs                   #   виклики memcpy/memset — хай вони будуть
    -Wl,--gc-sections                     # ...а тут невикористані секції викидаються
    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--print-memory-usage
)

# ---- Після збірки: .bin для прошивальників + розмір секцій -----------------
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Creating ${PROJECT_NAME}.bin"
)

# ---- cmake --build --preset debug --target flash ---------------------------
add_custom_target(flash
    COMMAND openocd -f board/st_nucleo_f4.cfg
            -c "program $<TARGET_FILE:${PROJECT_NAME}> verify reset exit"
    DEPENDS ${PROJECT_NAME}
    USES_TERMINAL
)

# ---- cmake --build --preset debug --target disasm  (подивитись, що згенерував компілятор)
add_custom_target(disasm
    COMMAND ${CMAKE_OBJDUMP} -d -S $<TARGET_FILE:${PROJECT_NAME}> > ${PROJECT_NAME}.lst
    DEPENDS ${PROJECT_NAME}
    COMMENT "Writing ${PROJECT_NAME}.lst"
)
