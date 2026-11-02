// Blinky для Blue Pill (STM32F103C8T6): LED на PC13, горить при 0.
// Лише адреси з RM0008, жодних бібліотек.
#include <stdint.h>

// --- RCC: вмикаємо тактування порту C ---
#define RCC_BASE        0x40021000u
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x18u))
#define RCC_IOPCEN      (1u << 4)            // біт 4: тактування GPIOC

// --- GPIOC ---
#define GPIOC_BASE      0x40011000u
#define GPIOC_CRH       (*(volatile uint32_t *)(GPIOC_BASE + 0x04u))  // налаштування пінів 8..15
#define GPIOC_ODR       (*(volatile uint32_t *)(GPIOC_BASE + 0x0Cu))  // вихідні значення

#define LED_PIN         13u

uint32_t g_blinks = 0;          // піде в .bss (нуль на старті)
uint32_t g_magic  = 0xCAFEBABE; // піде в .data (копіюється з Flash у RAM)

static void delay(volatile uint32_t n)
{
    while (n--) { }
}

// Цю функцію ніхто не викликає. Чи потрапить вона в прошивку — експеримент 5.
void unused_function(void)
{
    g_blinks = 12345;
}

int main(void)
{
    // Якщо Reset_Handler не скопіював .data, тут буде не 0xCAFEBABE -> зависаємо, LED не мигає.
    if (g_magic != 0xCAFEBABE) {
        for (;;) { }
    }

    RCC_APB2ENR |= RCC_IOPCEN;

    // PC13 — у CRH, поле з 4 біт на позиції (13 - 8) * 4 = 20.
    // 0b0010 = вихід push-pull, до 2 МГц.
    GPIOC_CRH = (GPIOC_CRH & ~(0xFu << 20)) | (0x2u << 20);

    for (;;) {
        GPIOC_ODR ^= (1u << LED_PIN);
        g_blinks++;
        delay(200000);
    }
}
