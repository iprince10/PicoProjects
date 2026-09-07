#include <stdint.h>
#include <bitmap.h>
#include <timer.h>
#include <uart.h>
#include <ili9341.h>
#include <lora_rx.h>

#define IO_BANK0_BASE 0x40014000u
#define GPIO25_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x0cc))
#define GPIO9_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x04c))
#define GPIO10_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x054))

#define GPIO_FUNC_SIO 5u
#define GPIO25 (1u << 25u) // LED
#define GPIO9 (1u << 9)    // BUZZER
#define GPIO10 (1u << 10)  // switch

#define SIO_BASE 0xd0000000u
#define SIO_GPIO_IN (*(volatile uint32_t *)(SIO_BASE + 0x004))
#define SIO_GPIO_OE (*(volatile uint32_t *)(SIO_BASE + 0x020))
#define SIO_GPIO_OUT_SET (*(volatile uint32_t *)(SIO_BASE + 0x14))
#define SIO_GPIO_OUT_CLR (*(volatile uint32_t *)(SIO_BASE + 0x18))
#define SIO_GPIO_OE_SET (*(volatile uint32_t *)(SIO_BASE + 0x24))
#define SIO_GPIO_OE_CLR (*(volatile uint32_t *)(SIO_BASE + 0x28))

#define PAD_BANK0_BASE 0x4001c000u
#define PAD_GPIO10_CTRL (*(volatile uint32_t *)(PAD_BANK0_BASE + 0x2c))
#define PAD_GPIO_CTRL_IE (1u << 6)
#define PAD_GPIO_CTRL_PUE (1u << 3)
#define PAD_GPIO_CTRL_PDE (1u << 2)

#define DEBOUNCE_US 30000UL            // 30 ms debounce window
#define ALERT_TIMEOUT_US 15000000ULL   // 15 s auto-silence
#define OFFLINE_TIMEOUT_US 15000000ULL // 1 minute offline tx side timeout

#define FULL_CM 20
#define EMPTY_CM 150

void buzzer_switch_led_init(void)
{
    // buzzer block
    GPIO9_CTRL = GPIO_FUNC_SIO; // GPIO9 -> SIO
    SIO_GPIO_OUT_CLR = GPIO9;   // GPIO LOW: BC547 off, buzzer off
    SIO_GPIO_OE_SET = GPIO9;    // now enable output

    // switch block
    GPIO10_CTRL = GPIO_FUNC_SIO;
    SIO_GPIO_OE_CLR = GPIO10;
    PAD_GPIO10_CTRL = (PAD_GPIO10_CTRL & ~PAD_GPIO_CTRL_PDE) | PAD_GPIO_CTRL_IE | PAD_GPIO_CTRL_PUE; // internal pull-up: idle=HIGH, pressed=LOW

    // LED Block
    GPIO25_CTRL = GPIO_FUNC_SIO;
    SIO_GPIO_OUT_SET = GPIO25; // onboard LED ON
    SIO_GPIO_OE_SET = GPIO25;  // output enable for led pin gpio 25
}

int main(void)
{
    ili9341_init();
    ili9341_fill_white();
    draw_tank_border();
    display_init_log();
    lora_rx_init();
    delay_ms(1000);
    uart1_init();
    uart0_init();
    uart0_puts("READY\r\n");
    // set_param_config_rx();
    check_config_rx();
    buzzer_switch_led_init();

    // Temporary power-on buzzer test.  A BC547 low-side switch is active-HIGH.
    SIO_GPIO_OUT_SET = GPIO9; // buzzer ON
    delay_ms(50);
    SIO_GPIO_OUT_CLR = GPIO9; // buzzer OFF

    // parser state persistent variables
    uint8_t frame_index = 0;
    uint8_t frame_buffer[6];
    uint8_t last_percent = 255; // impossible value, guarantees the first reading always draws

    uint64_t last_valid_frame_time = 0; // time to check device offline
    uint8_t offline_shown = 0;          // offline status flag

    uint8_t tank_full = 0;
    uint8_t silenced = 0;          // button status flag
    uint64_t alert_start_time = 0; // buzzer auto kill timer

    uint8_t switch_confirmed = 1;   // debounced switch state : 1 = not pressed, 0 = pressed
    uint8_t switch_before = 1;      // raw pin reading from the previous loop pass
    uint64_t switch_start_time = 0; // timestamp of the last time the raw reading changed

    while (1)
    {
        // if (wait_aux_high())
        // {
        while (uart1_has_data())
        {
            uint8_t byte = (uint8_t)uart1_getc();

            //   Print readable hex to serial monitor
            const char hex[] = "0123456789ABCDEF";
            uart0_putc(hex[(byte >> 4) & 0x0F]);
            uart0_putc(hex[byte & 0x0F]);
            uart0_putc(' ');

            if (frame_index == 0 && byte == 0xAA)
            {
                frame_buffer[frame_index] = byte;
                frame_index++;
            }
            else if (frame_index >= 1 && frame_index < 6)
            {
                frame_buffer[frame_index] = byte;
                frame_index++;

                if (frame_index == 6)
                {
                    uint8_t checksum = frame_buffer[1] ^ frame_buffer[2] ^ frame_buffer[3];
                    if (checksum == frame_buffer[4] && frame_buffer[5] == 0x55)
                    {
                        last_valid_frame_time = read_timer();

                        if (offline_shown)
                        {
                            offline_shown = 0;
                            last_percent = 255; // force the gauge to fully repaint, covering the offline screen
                        }

                        uint8_t status = frame_buffer[1];

                        if (status == 1)
                        {
                            // SIO_GPIO_OUT_SET = GPIO9; // BUZZER OFF
                            tank_full = 0;
                            silenced = 0;

                            uint32_t distance_cm = (uint64_t)(frame_buffer[2] << 8) | frame_buffer[3];
                            // char buff[16];
                            // format_distance(buff, distance_cm);
                            // display_add_line(buff);

                            int32_t diff = (int32_t)EMPTY_CM - (int32_t)distance_cm;
                            int32_t p = (diff * 100) / (EMPTY_CM - FULL_CM);
                            if (p < 0)
                                p = 0;
                            if (p > 100)
                                p = 100;
                            uint8_t percent_full = (uint8_t)p;

                            if (percent_full != last_percent)
                            {
                                update_tank_gauge(percent_full);
                                last_percent = percent_full;
                            }

                            uart0_putnum(distance_cm);
                            uart0_puts(" cm\r\n");
                            frame_index = 0;
                        }
                        else if (status == 2)
                        {
                            // SIO_GPIO_OUT_CLR = GPIO9; // buzzer ON

                            if (!tank_full)
                            {
                                tank_full = 1;
                                alert_start_time = read_timer();
                            }

                            // display_add_line("TANK FULL");
                            if (last_percent != 100)
                            {
                                update_tank_gauge(100);
                                last_percent = 100;
                            }
                            uart0_puts("TANK FULL\r\n");
                            frame_index = 0;
                        }
                        else
                        {
                            uart0_puts("no measurement\r\n");
                            // display_add_line("no measurement");
                            frame_index = 0;
                        }
                    }
                    else
                    {
                        frame_index = 0;
                    }
                }
            }
            else
            {
            }
        }
        // }  // aux comment bracket 

        //========== SWITCH AND BUZZER BLOCK ============//

        // --- read the switch pin right now ---
        uint8_t switch_now = (SIO_GPIO_IN & GPIO10) ? 1 : 0; // 1 = not pressed, 0 = pressed (pull-up wiring)
        uint64_t current_time = read_timer();

        // --- debounce: only trust a change once it holds steady for a while ---
        if (switch_now != switch_before)
        {
            // the raw reading just moved, restart the debounce timer
            switch_start_time = current_time;
            switch_before = switch_now;
        }

        if ((current_time - switch_start_time) >= DEBOUNCE_US && switch_confirmed != switch_now)
        {
            // the raw reading has held steady long enough, accept it as the real state
            uint8_t switch_confirmed_before = switch_confirmed;
            switch_confirmed = switch_now;

            // only act on the moment it goes from "not pressed" to "pressed" (a real press, not a hold)
            if (switch_confirmed_before == 1 && switch_confirmed == 0 && tank_full && !silenced)
            {
                silenced = 1; // button silences the current tank-full alert
            }
        }

        // --- auto-silence if the tank has stayed full too long with no button press ---
        if (tank_full && !silenced && (current_time - alert_start_time) >= ALERT_TIMEOUT_US)
        {
            silenced = 1;
        }

        // --- the buzzer pin only ever gets written here, based on the two flags above ---
        if (tank_full && !silenced)
        {
            SIO_GPIO_OUT_SET = GPIO9; // GPIO HIGH -> BC547 on -> buzzer ON
        }
        else
        {
            SIO_GPIO_OUT_CLR = GPIO9; // GPIO LOW -> BC547 off -> buzzer OFF
        }

        //========== TX OFFLINE BLOCK ===========//

        // --- check if the transmitter has gone quiet ---
        if (!offline_shown && (current_time - last_valid_frame_time) >= OFFLINE_TIMEOUT_US)
        {
            draw_tx_offline();
            offline_shown = 1;
        }
    }
}
