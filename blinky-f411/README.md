# Урок 1 — Bare-metal blinky на C (Nucleo-F411RE)

Мета: зрозуміти, що відбувається між подачею живлення і першим рядком `main()`,
і помигати світлодіодом LD2 (PA5) без HAL, без CMSIS — лише адреси з Reference Manual.

Документи, які варто мати під рукою (st.com):
- **RM0383** — Reference Manual STM32F411 (регістри RCC — розділ 6, GPIO — розділ 8)
- **DS10314** — datasheet STM32F411xC/E (карта пам'яті, розпіновка)
- **UM1724** — user manual плат Nucleo-64 (що куди підключено на платі)

---

## 1. Встановлення (macOS)

Потрібен Homebrew (https://brew.sh).

```bash
brew install --cask gcc-arm-embedded      # офіційний Arm GNU Toolchain: gcc, g++, gdb, newlib
brew install cmake ninja open-ocd
```

Перевірка — кожна команда має надрукувати версію:

```bash
arm-none-eabi-gcc --version
arm-none-eabi-gdb --version
cmake --version && ninja --version
openocd --version
```

Розширення VS Code (або відкрий папку проєкту — VS Code сам запропонує їх з `.vscode/extensions.json`):
- CMake Tools (`ms-vscode.cmake-tools`)
- C/C++ (`ms-vscode.cpptools`)
- Cortex-Debug (`marus25.cortex-debug`)

## 2. Перевір зв'язок з платою

Підключи Nucleo по USB і запусти:

```bash
openocd -f board/st_nucleo_f4.cfg
```

Очікуєш рядки на кшталт `Cortex-M4 r0p1 processor detected` і
`Listening on port 3333 for gdb connections`. `Ctrl+C` — вийти.

## 3. Збірка і прошивка

З терміналу:

```bash
cmake --preset debug                             # конфігурація (один раз)
cmake --build --preset debug                     # збірка -> build/debug/blinky_f411.elf / .bin
cmake --build --preset debug --target flash      # прошивка через OpenOCD
cmake --build --preset debug --target disasm     # дизасемблер у build/debug/blinky_f411.lst
```

У VS Code: відкрий папку → CMake Tools підхопить пресет **F411RE Debug** →
`F7` збірка → `F5` дебаг (зупиниться на `main`).

Запасний варіант прошивки: плата монтується як USB-диск (`NODE_F411RE` чи схоже) —
перетягни туди `build/debug/blinky_f411.bin`.

## 4. Що в проєкті

| Файл | Роль |
|---|---|
| `src/startup.c` | Таблиця векторів + `Reset_Handler`: вмикає FPU, копіює `.data`, обнуляє `.bss`, кличе `main()` |
| `linker/stm32f411re.ld` | Карта пам'яті: що лежить у FLASH, що в RAM, де стек |
| `src/main.c` | Тактування GPIOA → PA5 у режим виходу → нескінченне мигання |
| `cmake/arm-none-eabi.cmake` | Toolchain file: крос-компіляція під голий ARM |
| `CMakeLists.txt` | Прапорці ядра, лінкування, цілі `flash` і `disasm` |
| `svd/STM32F411.svd` | Опис регістрів — Cortex-Debug показує периферію по іменах |

### Послідовність старту

1. Ядро читає `0x0800_0000` → початковий SP (`_estack` = `0x2002_0000`, кінець RAM).
2. Ядро читає `0x0800_0004` → адреса `Reset_Handler` (`0x0800_0045`), стрибає туди.
3. `Reset_Handler` готує середовище C і викликає `main()`.

### Три речі, на яких найчастіше ламаються

- **Без тактування периферія "глуха".** Запис у GPIOA до `RCC_AHB1ENR |= GPIOAEN` просто ігнорується.
- **Read-modify-write з маскою, а не присвоєння.** Reset-значення `GPIOA_MODER = 0xA800_0000`:
  PA13/PA14 — це SWD. Перезапишеш регістр цілком — втратиш дебагер.
- **`volatile` на регістрах обов'язковий.** Без нього оптимізатор має право викинути "зайві" читання/записи.

### Факт, який варто побачити самому

Збери `release` (`cmake --preset release && cmake --build --preset release`) і подивись дизасемблер
`Reset_Handler`: на `-Os` GCC **сам замінив наші цикли копіювання на виклики `memcpy`/`memset`**.
Саме тому лінкуємо newlib-nano (`--specs=nano.specs`) навіть у "повністю з нуля" проєкті —
інакше отримав би `undefined reference to memcpy`.

---

## 5. Експерименти під дебагером

1. `F5` → зупинка на `main`. Відкрий панель периферії Cortex-Debug → **RCC → AHB1ENR** і **GPIOA → MODER / ODR**.
   Йди по кроках (`F10`) і дивись, як змінюються біти.
2. Постав breakpoint на рядок з `GPIOA_ODR ^=`. Кожне `F5` (Continue) — LED перемикається.
3. У панелі периферії вручну зміни біт `ODR5` — LED реагує без коду.
4. Відкрий `build/debug/blinky_f411.map` і знайди, за якими адресами лягли `g_vector_table`, `Reset_Handler`, `main`.

## 6. Домашнє завдання

1. **BSRR замість ODR.** Перепиши мигання через `GPIOA_BSRR` (зсув `0x18`): нижні 16 біт — set, верхні — reset.
   Поясни собі, чому `ODR ^=` — не атомарна операція і коли це стане проблемою.
2. **Кнопка.** Користувацька кнопка B1 — на **PC13**, active low. Увімкни тактування GPIOC,
   налаштуй PC13 як вхід, читай `GPIOC_IDR` (зсув `0x10`). LED світиться, поки кнопку натиснуто.
   Адреси GPIOC знайди сам у RM0383 (розділ 2.3, memory map).
3. **Логічний аналізатор.** Підключи канал до PA5 (пін D13 на Arduino-роз'ємі) і GND.
   Поміряй період мигання для `debug` (`-Og`) і `release` (`-Os`). Порахуй, скільки тактів
   16 МГц займає одна ітерація `delay()`. Чому значення різні?
4. **`.data` і `.bss` наживо.** Додай у `main.c` глобальні `uint32_t g_init = 0xCAFEBABE;` і `uint32_t g_zero[8];`,
   використай їх у коді. Подивись, як змінився вивід `size`/`--print-memory-usage`,
   і в дебагері перевір, що до входу в `main` вони вже мають правильні значення.

## 7. Питання для самоперевірки

1. Чому адреса `Reset_Handler` у таблиці векторів непарна (`0x0800_0045`)?
2. Чому `_estack` = `0x2002_0000`, а не `0x2001_FFFF`?
3. Де фізично лежить число `0xCAFEBABE` з ДЗ-4 до старту, і хто переносить його в RAM?
4. Що станеться, якщо прибрати `KEEP(...)` навколо `.isr_vector` у linker script?

<details>
<summary>Відповіді</summary>

1. Молодший біт = 1 означає Thumb-режим. Cortex-M вміє виконувати лише Thumb, тож біт 0 має бути 1, інакше — fault на першій же інструкції.
2. Стек на ARM — *full descending*: `push` спершу зменшує SP, потім пише. Перший запис піде на `0x2001_FFFC`.
3. У FLASH, одразу після коду (`_sidata` = `LOADADDR(.data)`). Копіює цикл у `Reset_Handler`.
4. З `--gc-sections` лінкер побачить, що на масив ніхто не посилається з коду, і викине його.
   На `0x0800_0000` опиниться код замість SP/Reset — МК не стартує.

</details>

## 8. Якщо щось не працює

- **`openocd`: `open failed` або ST-LINK не знайдено** — інший кабель (не "charge-only"),
  без USB-хаба. Перевір, що Mac бачить програматор: `ioreg -p IOUSB | grep -i stlink`.
- **OpenOCD скаржиться на стару прошивку ST-LINK** — онови через STM32CubeProgrammer (Firmware upgrade).
- **Cortex-Debug не знаходить gdb** — у `.vscode/settings.json` додай
  `"cortex-debug.gdbPath": "/opt/homebrew/bin/arm-none-eabi-gdb"`.
- **Плата перестала підключатись після експериментів з `GPIOA_MODER`** — підключайся під reset:
  `openocd -f board/st_nucleo_f4.cfg -c "reset_config srst_only connect_assert_srst" -c "init; reset halt; flash erase_sector 0 0 last; exit"`
  (стирає всю Flash), потім прошивай нормально.

---

**Далі (урок 2):** SysTick і точна затримка, переключення на 100 МГц через PLL (з wait states для Flash),
перше переривання. Потім — переносимо цей самий код на C++.
