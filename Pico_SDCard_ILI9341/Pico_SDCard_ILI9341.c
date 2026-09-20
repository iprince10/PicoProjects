#include <stdio.h>
#include <stdint.h>
#include "ili9341.h"
#include "timer.h"
#include "uart.h"
#include "bitmap.h"
#include "sdcard.h"

#define IO_BANK0_BASE 0x40014000u

#define GPIO_FUNC_SIO 5u
#define GPIO25_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0xcc))
#define SIO_BASE 0xd0000000u
#define SIO_GPIO_OE (*(volatile uint32_t *)(SIO_BASE + 0x020))
#define SIO_GPIO_OUT (*(volatile uint32_t *)(SIO_BASE + 0x010))
#define SIO_GPIO_OUT_XOR (*(volatile uint32_t *)(SIO_BASE + 0x01c))
#define GPIO25 (1u << 25)

void led_init(void)
{
    // blink code just for testing
    GPIO25_CTRL = GPIO_FUNC_SIO;
    SIO_GPIO_OE |= GPIO25;
    SIO_GPIO_OUT |= GPIO25;
}

int main()
{
    uart0_init();
    spi1_init();
    cs_init(); // after spi1 init always
    sd_dummy_clocks();
    led_init();

    // cmd0 
    uart0_puts("CMD0....\r\n");
    if (sd_cmd0())
    {
        uart0_puts("CMD0 Ok - card in idle\r\n");
    }
    else
    {
        uart0_puts("CMD0 FAIL\r\n");
    }

    // cmd8
    uart0_puts("CMD8....\r\n");
    uint8_t resp = sd_cmd8();
    if (resp == 1)
    {
        uart0_puts("CMD8 Ok - modern card, token echoed\r\n");
    }
    else if (resp == 2)
    {
        uart0_puts("CMD8 - V1 card (old)\r\n");
    }
    else
    {
        uart0_puts("CMD8 FAIL\r\n");
    }

    while (1)
    {
        SIO_GPIO_OUT_XOR = GPIO25;
        delay_ms(500);
    }
}
