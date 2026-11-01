# Toolchain file: каже CMake, що ми крос-компілюємо під голий ARM (без ОС).
# Підключається через CMakePresets.json -> "toolchainFile".

set(CMAKE_SYSTEM_NAME      Generic)   # немає ОС
set(CMAKE_SYSTEM_PROCESSOR arm)

set(TOOLCHAIN_PREFIX arm-none-eabi-)

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_OBJDUMP      ${TOOLCHAIN_PREFIX}objdump)
set(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}size)

# CMake при конфігурації пробує зібрати і запустити тестову програму.
# Під МК запустити її не можна — просимо збирати лише статичну бібліотеку.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Шукати бібліотеки/заголовки лише в sysroot тулчейну, програми — на хості.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
