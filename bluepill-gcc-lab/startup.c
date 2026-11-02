// Мінімальний startup для STM32F103 (Cortex-M3, без FPU).
#include <stdint.h>

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;
int main(void);

void Reset_Handler(void)
{
    // FPU вмикати не треба: на Cortex-M3 його немає.

    // .data: копіюємо початкові значення з Flash у RAM
    uint32_t *src = &_sidata;
    for (uint32_t *dst = &_sdata; dst < &_edata; ) *dst++ = *src++;

    // .bss: обнуляємо
    for (uint32_t *dst = &_sbss; dst < &_ebss; ) *dst++ = 0;

    main();
    for (;;) { }
}

void Default_Handler(void) { for (;;) { } }

void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));

// Таблиця векторів: 16 системних записів. Лінкер кладе її на 0x08000000.
__attribute__((section(".isr_vector"), used))
const void * const g_vector_table[16] = {
    &_estack,            // 0: початкове значення SP
    Reset_Handler,       // 1: звідки стартувати
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0, 0, 0, 0,          // зарезервовано
    SVC_Handler,
    DebugMon_Handler,
    0,                   // зарезервовано
    PendSV_Handler,
    SysTick_Handler,
};
