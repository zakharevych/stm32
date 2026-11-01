/*
 * Урок 1 — bare-metal blinky на Nucleo-F411RE.
 *
 * Жодних бібліотек: адреси регістрів беремо прямо з Reference Manual RM0383.
 *   - RCC:   розділ 6  (карта регістрів — 6.3.x), база 0x4002_3800
 *   - GPIO:  розділ 8  (карта регістрів — 8.4.x), GPIOA база 0x4002_0000
 *
 * Зелений світлодіод LD2 на платі підключений до PA5 (active high).
 * Після reset МК працює від HSI = 16 МГц.
 */
#include <stdint.h>

/* ---- RCC ---------------------------------------------------------------- */
#define RCC_BASE        0x40023800UL
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define RCC_AHB1ENR_GPIOAEN   (1UL << 0)

/* ---- GPIOA -------------------------------------------------------------- */
#define GPIOA_BASE      0x40020000UL
#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL))
#define GPIOA_ODR       (*(volatile uint32_t *)(GPIOA_BASE + 0x14UL))

#define LED_PIN         5U

/* Груба програмна затримка. volatile не дає компілятору викинути порожній цикл.
 * Точний час зробимо в наступному уроці через SysTick. */
static void delay(uint32_t ticks)
{
    for (volatile uint32_t i = 0; i < ticks; ++i) {
    }
}

int main(void)
{
    /* 1. Подаємо тактування на порт A. Без цього запис у GPIOA ігнорується. */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    /* Після ввімкнення тактування периферії потрібна пауза в кілька тактів
     * до першого звернення (errata sheet STM32F411, пункт "Delay after an RCC
     * peripheral clock enabling"). Холостий readback через шину це гарантує. */
    (void)RCC_AHB1ENR;

    /* 2. PA5 -> режим General purpose output (MODER5 = 0b01).
     * Змінюємо ЛИШЕ біти [11:10]: reset-значення GPIOA_MODER = 0xA800_0000,
     * там PA13/PA14 вже в Alternate Function — це SWD, по якому ти дебажиш.
     * Перезапишеш весь регістр — втратиш дебагер. */
    GPIOA_MODER &= ~(3UL << (LED_PIN * 2U));
    GPIOA_MODER |=  (1UL << (LED_PIN * 2U));

    /* 3. Нескінченний цикл: інвертуємо біт виходу. */
    for (;;) {
        GPIOA_ODR ^= (1UL << LED_PIN);
        delay(400000U);
    }
}
