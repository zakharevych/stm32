# Практика: `arm-none-eabi-gcc` вручну — blinky для Blue Pill

**Мета:** зібрати прошивку власними руками, крок за кроком, без CMake. Подивитись, що з'являється після кожного кроку. Потім навмисно зламати збірку кількома способами і зрозуміти, що каже gcc.

**Час:** ≈ 1,5 год · **Плата:** Blue Pill (STM32F103C8T6) + ST-LINK V2

> Вивід у цьому файлі записано на GCC 13.2. У тебе на Mac GCC 15, тож розміри можуть відрізнятись на кілька байт, а шляхи в повідомленнях будуть інші. Суть та сама.

## Що в архіві

| Файл | Що це |
|---|---|
| `main.c` | Blinky: LED на PC13 мигає |
| `startup.c` | Код старту: таблиця векторів і `Reset_Handler` (копіює `.data`, обнуляє `.bss`, кличе `main`) |
| `bluepill.ld` | Linker script: де Flash (64K), де RAM (20K) і що куди класти |
| `fpu_test.c` | Одна функція з `float` — для експериментів з FPU |
| `build.sh` | Готове рішення: усі кроки одним скриптом. Відкривай лише для перевірки |

## Нагадування: що з чого виходить

```
main.c ──(-E)──► main.i ──(-S)──► main.s ──(-c)──► main.o ─┐
                 C без #include    асемблер          байти    ├─(лінкер + bluepill.ld)─► blinky.elf ──(objcopy)──► blinky.bin
startup.c ─────────────────────────────────(-c)──► startup.o ┘                         програма + «паспорт»       голі байти для Flash
```

---

## Крок 0. Прапорці «для якого чипа»

Щоб не писати їх щоразу, збережи в змінну терміналу:

```bash
cd bluepill-gcc-lab
F="-mcpu=cortex-m3 -mthumb -mfloat-abi=soft"
```

- `-mcpu=cortex-m3` — ядро Blue Pill.
- `-mthumb` — набір команд Thumb (інших Cortex-M не розуміє).
- `-mfloat-abi=soft` — `float` рахувати програмно, бо FPU на F103 немає. `-mfpu` не пишемо взагалі.

## Крок 1. Препроцесор: `-E`

```bash
arm-none-eabi-gcc $F -E main.c -o main.i
wc -l main.c main.i
grep -n "0x40011000u" main.i
```

**Що побачиш:** `main.i` довший за `main.c` (106 рядків проти 49), бо `#include <stdint.h>` вставив туди вміст заголовка. Знайди рядок з `GPIOC_CRH = ...`: замість імені там тепер «сирий» вираз:

```c
(*(volatile uint32_t *)(0x40011000u + 0x04u)) = ...
```

**Висновок:** `#define` — це просто заміна тексту ще до компіляції. Компілятор імен на кшталт `GPIOC_CRH` взагалі не бачить.

## Крок 2. Компілятор: `-S` (C → асемблер)

```bash
arm-none-eabi-gcc $F -O0 -S main.c -o main_O0.s
arm-none-eabi-gcc $F -Os -S main.c -o main_Os.s
wc -l main_O0.s main_Os.s
```

Відкрий обидва `.s` і знайди `main:`.

**Що побачиш:** у `-Os` команд помітно менше. Ось шматок, який вмикає тактування GPIOC (рядок `RCC_APB2ENR |= RCC_IOPCEN`):

```asm
ldr  r3, [r2, #24]     @ прочитати RCC_APB2ENR (0x40021000 + 24 = +0x18)
orr  r3, r3, #16       @ встановити біт 4 (16 = 1 << 4)
str  r3, [r2, #24]     @ записати назад
```

**Висновок:** одна команда C `|=` — це три команди процесора: прочитати, змінити, записати. Саме тому `ODR ^=` не атомарна операція (ДЗ 1 модуля 1).

## Крок 3. Асемблер: `-c` (отримуємо `.o`)

```bash
arm-none-eabi-gcc $F -Og -g -c main.c    -o main.o
arm-none-eabi-gcc $F -Og -g -c startup.c -o startup.o
file main.o
arm-none-eabi-nm main.o
arm-none-eabi-nm startup.o
```

**Що побачиш:**

```
main.o: ELF 32-bit LSB relocatable, ARM, EABI5 ...

00000000 t delay
00000000 B g_blinks
00000000 D g_magic
00000020 T main
00000012 T unused_function
...
         U _sdata
         U _sidata
         U main          ← це з startup.o
```

`nm` показує імена з однобуквеною міткою:

| Буква | Що означає |
|---|---|
| `T` | Функція (код) |
| `t` | Функція, видима лише всередині файла (`static`) |
| `D` | Змінна з початковим значенням (`.data`) |
| `B` | Змінна, що на старті дорівнює нулю (`.bss`) |
| `R` | Константа, лише для читання |
| `W` | «Слабке» ім'я: використовується, якщо ніхто не дав сильного (обробники переривань) |
| `U` | **Невідоме** — цей файл ним користується, але не має його |

**Висновок:** у `.o` адреси всі нульові, а частина імен має мітку `U`. `startup.o` кличе `main`, але сам не знає, де вона. Знайти й з'єднати — робота лінкера.

## Крок 4. Лінкер (`.o` → `.elf`)

```bash
arm-none-eabi-gcc $F -T bluepill.ld -nostartfiles --specs=nano.specs \
  -Wl,--gc-sections -Wl,-Map=blinky.map \
  startup.o main.o -o blinky.elf

file blinky.elf
arm-none-eabi-nm -n blinky.elf
```

**Що побачиш** (`-n` сортує за адресою):

```
08000000 R g_vector_table      ← таблиця векторів на самому початку Flash
08000042 T Reset_Handler
080000a4 T main
20000000 D g_magic             ← у RAM
20000004 B g_blinks            ← у RAM
20005000 R _estack             ← кінець RAM = вершина стеку
```

**Висновок:** після лінкера в кожного імені справжня адреса. Flash починається з `0x0800_0000`, RAM — з `0x2000_0000`, як написано в `bluepill.ld`. Усі `U` зникли — лінкер їх знайшов.

Відкрий `blinky.map` у редакторі й знайди `g_magic`. Там видно, з якого `.o` прийшло кожне ім'я і скільки байтів воно займає.

## Крок 5. `objcopy` і `size`

```bash
arm-none-eabi-size blinky.elf
arm-none-eabi-objcopy -O binary blinky.elf blinky.bin
ls -l blinky.elf blinky.bin
xxd blinky.bin | head -2
```

**Що побачиш:**

```
   text    data     bss     dec     hex filename
    256       4       4     264     108 blinky.elf
```

- `text` — код і константи (Flash);
- `data` — `g_magic`: 4 байти в RAM, а його початкове значення ще 4 байти лежить у Flash;
- `bss` — `g_blinks`: 4 байти в RAM, обнуляються на старті.

`blinky.bin` = `text` + `data` = 260 байт. А `.elf` у рази більший, бо в ньому «паспорт» і дебаг-інформація (`-g`).

Перші байти `.bin`:

```
00000000: 0050 0020 4300 0008 ...
```

Читаємо задом наперед (little-endian): `0x20005000` — це `_estack`, перше слово таблиці векторів. `0x08000043` — адреса `Reset_Handler` + 1. Звідки +1 — питання 1 самоперевірки модуля 1.

## Крок 6. Прошивка

ST-LINK V2 → Blue Pill: `3.3V`, `GND`, `SWDIO` → `DIO`, `SWCLK` → `DCLK` (4 піни на торці плати).

```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program blinky.elf verify reset exit"
```

Чекаємо `** Verified OK **`. LED на PC13 мигає кілька разів на секунду.

Якщо в логу `UNEXPECTED idcode` — на платі клон чипа (CKS32, GD32, APM32, такі на Blue Pill трапляються часто). Додай перед `target` рядок із тим idcode, який OpenOCD надрукував:
```bash
openocd -f interface/stlink.cfg -c "set CPUTAPID 0x2ba01477" -f target/stm32f1x.cfg -c "program blinky.elf verify reset exit"
```

---

## Експерименти: ламаємо навмисно

Для кожного: спершу вгадай, що буде, потім запусти.

### Е1. Забули `-mcpu`

```bash
arm-none-eabi-gcc -O2 -S -o - fpu_test.c | grep -E "\.cpu|\.arch|\.arm"
```

```
.cpu arm7tdmi
.arch armv4t
.arm
```

Без `-mcpu` gcc генерує код для ARM7 1990-х років, у режимі ARM. Cortex-M3 такого не виконає. Помилки при збірці не буде — прошивка просто впаде.

### Е2. Забули `-mthumb`, але `-mcpu` є

```bash
arm-none-eabi-gcc -mcpu=cortex-m3 -O2 -S -o - fpu_test.c | grep thumb
arm-none-eabi-gcc -mcpu=cortex-m3 -marm -c fpu_test.c
```

Перша команда покаже `.thumb`: знаючи ядро, gcc сам обирає Thumb, бо іншого Cortex-M3 не вміє. Друга команда (явно просимо ARM-режим) дасть:

```
cc1: error: target CPU does not support ARM mode
```

`-mthumb` все одно пишемо завжди — так видно наміри, і він не залежить від поведінки конкретної версії gcc.

### Е3. `float` без FPU і з FPU

```bash
# Blue Pill
arm-none-eabi-gcc $F -O2 -S -o - fpu_test.c | grep -E "bl|vmul|bx"
# Прапорці F411 (Nucleo / Black Pill)
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -O2 -S -o - fpu_test.c | grep -E "bl|vmul|bx"
```

| Blue Pill (`soft`) | F411 (`hard`) |
|---|---|
| `bl __aeabi_fmul` — виклик бібліотечної функції, яка множить програмно, десятками команд | `vmul.f32 s0, s0, s1` — одна команда FPU |

Якщо прошити F411-варіант у Blue Pill, процесор дійде до `vmul.f32`, не впізнає команду і впаде в HardFault.

### Е4. Змішали `soft` і `hard` в одній прошивці

```bash
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -c fpu_test.c -o fpu_hard.o
arm-none-eabi-gcc $F -T bluepill.ld -nostartfiles --specs=nano.specs startup.o main.o fpu_hard.o -o bad.elf
```

```
ld: error: fpu_hard.o uses VFP register arguments, bad.elf does not
```

Лінкер ловить це сам: одна частина передає `float` у регістрах FPU (`s0`), інша — у звичайних (`r0`). Вони б не зрозуміли одна одну.

### Е5. Хто викидає непотрібний код: `--gc-sections`

У `main.c` є `unused_function()`, яку ніхто не викликає.

```bash
arm-none-eabi-nm blinky.elf | grep unused        # є!
```

Вона в прошивці, хоча є `--gc-sections`. Річ у тім, що лінкер викидає **секції**, а не окремі функції. Без додаткових прапорців уся `main.c` — одна секція `.text`, і якщо з неї потрібна хоча б `main`, береться вся.

Тепер попросимо компілятор класти кожну функцію в окрему секцію:

```bash
arm-none-eabi-gcc $F -Og -g -ffunction-sections -fdata-sections -c main.c    -o main_fs.o
arm-none-eabi-gcc $F -Og -g -ffunction-sections -fdata-sections -c startup.c -o startup_fs.o
arm-none-eabi-gcc $F -T bluepill.ld -nostartfiles --specs=nano.specs \
  -Wl,--gc-sections -Wl,--print-gc-sections startup_fs.o main_fs.o -o gc.elf
arm-none-eabi-size gc.elf
```

```
ld: removing unused section '.text.unused_function' in file 'main_fs.o'
   text    data     bss
    244       4       4      ← на 12 байт менше
```

Звідси правило з модуля 1: `-ffunction-sections -fdata-sections` працюють лише в парі з `--gc-sections`.

Бонус: закоментуй у `main.c` перевірку `if (g_magic != ...)` і збери знову, як у кроці 4. `g_magic` зникне з `nm`, хоча змінна є в коді: її ніхто не читає, і лінкер викинув цілу секцію `.data` з `main.o`.

### Е6. Забули `-nostartfiles`

```bash
arm-none-eabi-gcc $F -T bluepill.ld --specs=nano.specs startup.o main.o -o x.elf
```

```
.../thumb/v7-m/nofp/crt0.o: in function `_mainCRTStartup':
undefined reference to `__bss_start__'
```

gcc підставив **свій** стандартний код старту `crt0.o`. Він шукає імена, яких у нашому linker script немає (у нас `_sbss`, у нього `__bss_start__`). Старт у нас свій, `startup.c`, тому стандартний вимикаємо.

Подивись на шлях: `thumb/v7-m/nofp`. Це multilib — готова збірка бібліотек саме під «Thumb, ARMv7-M, без FPU». gcc обрав її за нашими прапорцями. Перевір сам:

```bash
arm-none-eabi-gcc $F -print-multi-directory                                                 # thumb/v7-m/nofp
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -print-multi-directory  # thumb/v7e-m+fp/hard
```

### Е7. Забули `-T bluepill.ld`

```bash
arm-none-eabi-gcc $F -nostartfiles --specs=nano.specs startup.o main.o -o x.elf
```

```
warning: cannot find entry symbol _start; defaulting to 00008000
undefined reference to `_sdata'
```

Без linker script лінкер не знає ні адрес Flash і RAM, ні наших імен `_sdata`, `_sidata` тощо. Їх визначає саме `bluepill.ld`.

### Е8. Рівні оптимізації

```bash
for o in O0 Og Os O2; do arm-none-eabi-gcc $F -$o -c main.c -o m.o; arm-none-eabi-size m.o | tail -1; done
```

| | `-O0` | `-Og` | `-Os` | `-O2` |
|---|---|---|---|---|
| `text` у `main.o` | 160 | 124 | 104 | 104 |

Однакові розміри `-Os` і `-O2` — це для такого маленького коду. На великому проєкті `-O2` буде більшим.

Проший `-O0` і `-Os` (кроки 3–6) і порівняй, як швидко мигає LED. `delay()` той самий, а період різний. Це тема ДЗ 3 модуля 1.

---

## Шпаргалка прапорців gcc

| Прапорець | Простими словами |
|---|---|
| `-mcpu=cortex-m3` / `-mthumb` / `-mfloat-abi=soft` | Для якого чипа (Blue Pill) |
| `-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard` | Для якого чипа (F411) |
| `-E` / `-S` / `-c` | Зупинитись після препроцесора / компілятора / асемблера |
| `-o файл` | Куди записати результат |
| `-O0` `-Og` `-Os` `-O2` | Оптимізація: немає / для дебагу / за розміром / за швидкістю |
| `-g` | Додати дебаг-інформацію |
| `-ffunction-sections -fdata-sections` | Кожна функція і змінна — в окрему секцію |
| `-T файл.ld` | Linker script |
| `-nostartfiles` | Без стандартного коду старту |
| `--specs=nano.specs` | Маленька бібліотека newlib-nano |
| `-Wl,--gc-sections` | Лінкеру: викинь секції, які ніхто не використовує |
| `-Wl,-Map=файл.map` | Лінкеру: запиши карту пам'яті |
| `-Wl,...` | Передати опцію лінкеру |
| `-print-multi-directory` | Яку збірку бібліотек gcc обрав під ці прапорці |

## Самоперевірка

1. Що лежить у `main.i`, `main.s`, `main.o`, `blinky.elf`, `blinky.bin`? Який із них можна прочитати очима, а який — прошити?
2. `nm main.o` показує `U _sdata`. Хто визначає це ім'я і на якому кроці воно отримає адресу?
3. Чому `data = 4` у виводі `size` займає місце і у Flash, і в RAM?
4. Чому `unused_function` лишилась у прошивці, хоча є `--gc-sections`? Що треба додати?
5. Ти випадково зібрав Blue Pill з `-mfloat-abi=hard`. Що станеться при збірці? А при запуску?

## Критерій «практику пройдено»

- [ ] Пройдено кроки 1–6, LED на Blue Pill мигає з прошивки, зібраної вручну
- [ ] Виконано експерименти Е1–Е8, для кожного можеш пояснити причину
- [ ] Відповідаєш на 5 питань самоперевірки
