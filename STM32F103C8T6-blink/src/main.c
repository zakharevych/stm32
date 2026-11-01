#include <stdint.h>

/* Адреси з RM0008: база блоку (Table 3) + зсув регістра */
#define RCC_APB2ENR  (*(volatile uint32_t *)0x40021018u)   /* RCC 0x40021000 + 0x18 */
#define GPIOC_CRH    (*(volatile uint32_t *)0x40011004u)   /* GPIOC 0x40011000 + 0x04 */
#define GPIOC_BSRR   (*(volatile uint32_t *)0x40011010u)   /* GPIOC 0x40011000 + 0x10 */

#define RCC_IOPCEN   (1u << 4)                             /* тактування порту C */

#define PIN13_SET    (1u << 13)                            /* BS13: пін у 1, LED гасне */
#define PIN13_RESET  (1u << (13 + 16))                     /* BR13: пін у 0, LED світиться */

static void delay(volatile uint32_t n)
{
    while (n--) {}
}

int main(void)
{
    /* 1. Тактування порту C */
    RCC_APB2ENR |= RCC_IOPCEN;

    /* 2. PC13: вихід 2 МГц (MODE=10), push-pull (CNF=00). Піни 8-15 у CRH, PC13 = біти 23:20 */
    GPIOC_CRH = (GPIOC_CRH & ~(0xFu << 20)) | (0x2u << 20);

    /* 3. Блимаємо */
    for (;;) {
        GPIOC_BSRR = PIN13_RESET;
        delay(800000);
        GPIOC_BSRR = PIN13_SET;
        delay(800000);
    }
}
