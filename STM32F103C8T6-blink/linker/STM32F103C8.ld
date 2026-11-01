/* Linker script для STM32F103C8T6: що де лежить у пам'яті */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}

/* Вершина стека = перший байт ЗА межею RAM (стек росте вниз) */
_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    /* Таблиця векторів мусить бути першою у Flash: ядро читає її з 0x08000000 */
    .isr_vector :
    {
        KEEP(*(.isr_vector))
    } > FLASH

    /* Код і константи живуть у Flash */
    .text :
    {
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* Початкові значення змінних лежать у Flash (LMA), а змінні живуть у RAM (VMA) */
    _sidata = LOADADDR(.data);

    .data :
    {
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    /* Змінні без початкового значення: у Flash не зберігаємо, лише обнуляємо */
    .bss :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /DISCARD/ : { *(.ARM.exidx*) }
}
