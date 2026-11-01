# Toolchain file: каже CMake, що збираємо для голого ARM, а не для ПК
set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(TOOLCHAIN_PREFIX arm-none-eabi-)
set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}size)

# CMake не вміє запускати зібране на ПК, тому тестову збірку робимо як бібліотеку
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
