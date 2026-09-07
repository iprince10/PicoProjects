#include <stdint.h>
#include <bitmap.h>
#include <timer.h>
#include <sr04t.h>
#include <ili9341.h>

#define IO_BANK0_BASE 0x40014000u
#define SIO_BASE 0xd0000000u
#define SIO_GPIO_OE (*(volatile uint32_t *)(SIO_BASE + 0x020))
#define LED_PIN_25 25u
#define GPIO_FUNC_SIO (5u)
#define GPIO25_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x0cc))

int main(void)
{
    ili9341_init();
    ili9341_fill_white();
    display_init_log();
    uart0_init();
    gpio3_init_input();
    gpio2_init_output();

    GPIO25_CTRL = GPIO_FUNC_SIO;
    SIO_GPIO_OE |= (1u << LED_PIN_25); // output enable for led pin gpio 25

    uart0_puts("Ready\r\n");
    __asm volatile("cpsie i");

    while (1)
    {
        // Reset state for clean cycle
        __asm volatile("cpsid i");
        __asm volatile("" ::: "memory");
        rise_captured = 0;
        ready_flag = 0;
        rise_time = 0;
        fail_time = 0;
        __asm volatile("" ::: "memory");
        __asm volatile("cpsie i");

        sr04t_send_trigger();
        delay_ms(100); // wait for echo

        // Read result
        __asm volatile("cpsid i");
        __asm volatile("" ::: "memory");
        uint64_t r = rise_time, f = fail_time;
        uint8_t flag = ready_flag;
        ready_flag = 0;
        __asm volatile("" ::: "memory");
        __asm volatile("cpsie i");

        if (flag && f > r)
        {
            uint64_t duration_us = f - r;
            uint64_t distance_cm = duration_us / 58;

            if (distance_cm > 500 || distance_cm <= 20)
            {
                // max & min distance
                uart0_puts("TANK FULL\r\n");
                display_add_line("TANK FULL");
            }
            else
            {
                uart0_putnum(distance_cm);
                uart0_puts(" cm\r\n");

                char buff[16];
                format_distance(buff, distance_cm);
                display_add_line(buff);
            }
        }
        else
        {
            uart0_puts("no measurement\r\n");
            display_add_line("no measurement");
        }

        delay_ms(500); // wait between triggers
    }
}
