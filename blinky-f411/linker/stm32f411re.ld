/*
 * Linker script для STM32F411RE (Nucleo-F411RE).
 *
 * Лінкер отримує купу об'єктних файлів із секціями .text/.data/.bss/...
 * і за цим файлом вирішує, ЩО і КУДИ покласти в пам'яті МК.
 *
 * Карта пам'яті — datasheet STM32F411xE, розділ "Memory mapping":
 *   Flash 512 KB з 0x0800_0000
 *   SRAM  128 KB з 0x2000_0000
 */

ENTRY(Reset_Handler)          /* для дебагера та ELF-заголовка */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* Стек росте вниз від кінця RAM. */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* Мінімальний запас під стек: лінкер впаде з помилкою, якщо .data + .bss
 * з'їдять пам'ять і на стек лишиться менше. */
_min_stack_size = 0x800;      /* 2 KB */

SECTIONS
{
    /* Таблиця векторів — строго першою у Flash. */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))  /* KEEP: не викидати при --gc-sections */
        . = ALIGN(4);
    } > FLASH

    /* Код. */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        . = ALIGN(4);
    } > FLASH

    /* Константи (const-масиви, рядкові літерали). */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* Таблиці розмотування стеку (з'являються з libgcc / у C++). */
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH

    /* Звідси Reset_Handler копіюватиме початкові значення .data. */
    _sidata = LOADADDR(.data);

    /* Ініціалізовані глобальні змінні.
     * Живуть у RAM (VMA), але їхні початкові значення зберігаються у FLASH (LMA). */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    /* Неініціалізовані глобальні змінні: у FLASH не займають місця,
     * Reset_Handler просто обнуляє цей діапазон. */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* Перевірка, що під стек лишилось місце. */
    ._stack_check (NOLOAD) :
    {
        . = ALIGN(8);
        . = . + _min_stack_size;
        . = ALIGN(8);
    } > RAM

    /* Метадані ARM, які не потрапляють у прошивку. */
    .ARM.attributes 0 : { *(.ARM.attributes) }
}
