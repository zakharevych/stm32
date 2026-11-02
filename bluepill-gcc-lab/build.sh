#!/bin/sh
# Готове рішення: кроки 1-5 з README одним скриптом. Спершу пройди їх руками!
set -e
cd "$(dirname "$0")"

F="-mcpu=cortex-m3 -mthumb -mfloat-abi=soft"

arm-none-eabi-gcc $F -E main.c -o main.i
arm-none-eabi-gcc $F -Os -S main.c -o main.s
arm-none-eabi-gcc $F -Og -g -c startup.c -o startup.o
arm-none-eabi-gcc $F -Og -g -c main.c    -o main.o
arm-none-eabi-gcc $F -T bluepill.ld -nostartfiles --specs=nano.specs \
  -Wl,--gc-sections -Wl,-Map=blinky.map startup.o main.o -o blinky.elf
arm-none-eabi-objcopy -O binary blinky.elf blinky.bin
arm-none-eabi-size blinky.elf

echo "Прошити: openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c \"program blinky.elf verify reset exit\""
