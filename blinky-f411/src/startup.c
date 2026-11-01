/*
 * Startup для STM32F411 (Cortex-M4F), написаний на C.
 *
 * Що відбувається після подачі живлення / reset:
 *   1. Ядро читає слово за адресою 0x0800_0000 -> це початкове значення SP.
 *   2. Ядро читає слово за адресою 0x0800_0004 -> адреса Reset_Handler, стрибає туди.
 *   3. Reset_Handler готує пам'ять так, як цього очікує C (.data / .bss), і кличе main().
 *
 * Таблиця векторів тут містить лише 16 системних винятків ядра.
 * Вектори переривань периферії (EXTI, TIM, USART...) додамо, коли знадобляться.
 */
#include <stdint.h>

/* Символи, які визначає linker script (див. linker/stm32f411re.ld).
 * Це не змінні, а АДРЕСИ — тому завжди беремо &symbol. */
extern uint32_t _estack;
extern uint32_t _sidata;   /* де лежать початкові значення .data у FLASH */
extern uint32_t _sdata;    /* початок .data в RAM */
extern uint32_t _edata;    /* кінець  .data в RAM */
extern uint32_t _sbss;     /* початок .bss в RAM */
extern uint32_t _ebss;     /* кінець  .bss в RAM */

int main(void);

void Reset_Handler(void);
void Default_Handler(void);

/* weak + alias: якщо ти пізніше напишеш свою функцію HardFault_Handler,
 * лінкер візьме її; інакше всі ці імена вказують на Default_Handler. */
void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));

/* Таблиця векторів. section(".isr_vector") + KEEP у linker script кладуть її
 * рівно на 0x0800_0000. used — щоб компілятор не викинув "невикористаний" масив. */
__attribute__((section(".isr_vector"), used))
const uintptr_t g_vector_table[16] = {
    (uintptr_t)&_estack,            /*  0: початковий SP                  */
    (uintptr_t)Reset_Handler,       /*  1: Reset                          */
    (uintptr_t)NMI_Handler,         /*  2: NMI                            */
    (uintptr_t)HardFault_Handler,   /*  3: HardFault                      */
    (uintptr_t)MemManage_Handler,   /*  4: MemManage                      */
    (uintptr_t)BusFault_Handler,    /*  5: BusFault                       */
    (uintptr_t)UsageFault_Handler,  /*  6: UsageFault                     */
    0, 0, 0, 0,                     /*  7-10: зарезервовано               */
    (uintptr_t)SVC_Handler,         /* 11: SVCall                         */
    (uintptr_t)DebugMon_Handler,    /* 12: Debug Monitor                  */
    0,                              /* 13: зарезервовано                  */
    (uintptr_t)PendSV_Handler,      /* 14: PendSV                         */
    (uintptr_t)SysTick_Handler,     /* 15: SysTick                        */
};

/* Coprocessor Access Control Register (Cortex-M4 System Control Block). */
#define SCB_CPACR   (*(volatile uint32_t *)0xE000ED88UL)

void Reset_Handler(void)
{
    /* 0. Вмикаємо FPU (CP10 + CP11 = full access). Ми компілюємо з
     *    -mfloat-abi=hard, і перша ж FP-інструкція при вимкненому FPU
     *    дасть UsageFault. Робимо це ДО будь-якого іншого коду. */
    SCB_CPACR |= (0xFUL << 20);
    __asm volatile ("dsb\n\tisb" ::: "memory");

    /* 1. Копіюємо початкові значення ініціалізованих глобальних змінних
     *    (int x = 42;) з FLASH у RAM. */
    uint32_t *src = &_sidata;
    for (uint32_t *dst = &_sdata; dst < &_edata; ) {
        *dst++ = *src++;
    }

    /* 2. Обнуляємо неініціалізовані глобальні змінні (int y;) —
     *    стандарт C гарантує, що вони дорівнюють нулю. */
    for (uint32_t *dst = &_sbss; dst < &_ebss; ) {
        *dst++ = 0;
    }

    /* 3. Поїхали. */
    main();

    /* main() на МК не повинен повертатись. Якщо повернувся — зависаємо тут. */
    for (;;) {
    }
}

/* Сюди потрапляє будь-який виняток без власного обробника.
 * Під дебагером побачиш, що програма "висить" саме тут — це підказка,
 * що стався fault. */
void Default_Handler(void)
{
    for (;;) {
    }
}
