#include <stdint.h>

/* Символи з linker script */
extern uint32_t _estack;
extern uint32_t _sidata, _sdata, _edata;
extern uint32_t _sbss, _ebss;

extern int main(void);

void Reset_Handler(void);
void Default_Handler(void);

/* Слабкі обробники: якщо не визначиш свій, буде Default_Handler */
void NMI_Handler(void)         __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)    __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)         __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)    __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)      __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)     __attribute__((weak, alias("Default_Handler")));

/* Таблиця векторів: слово 0 = початок стека, слово 1 = Reset_Handler.
   Далі системні винятки ядра. Переривання периферії додамо пізніше. */
__attribute__((section(".isr_vector"), used))
void (* const g_vector_table[])(void) = {
    (void (*)(void))(&_estack),   /* 0: початкове значення MSP */
    Reset_Handler,                /* 1: перша інструкція */
    NMI_Handler,                  /* 2 */
    HardFault_Handler,            /* 3 */
    MemManage_Handler,            /* 4 */
    BusFault_Handler,             /* 5 */
    UsageFault_Handler,           /* 6 */
    0, 0, 0, 0,                   /* 7-10: зарезервовано */
    SVC_Handler,                  /* 11 */
    DebugMon_Handler,             /* 12 */
    0,                            /* 13: зарезервовано */
    PendSV_Handler,               /* 14 */
    SysTick_Handler,              /* 15 */
};

void Reset_Handler(void)
{
    /* 1. Скопіювати початкові значення .data з Flash у RAM */
    uint32_t *src = &_sidata;
    for (uint32_t *dst = &_sdata; dst < &_edata; ) {
        *dst++ = *src++;
    }

    /* 2. Обнулити .bss */
    for (uint32_t *dst = &_sbss; dst < &_ebss; ) {
        *dst++ = 0;
    }

    /* 3. Тепер "світ C" готовий: викликаємо main */
    main();

    /* main не мусить повертатися; якщо повернулась, зависаємо */
    for (;;) {}
}

void Default_Handler(void)
{
    for (;;) {}
}
