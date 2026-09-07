#include <stdint.h>

#define SIO_BASE 0xd0000000u
#define SIO_GPIO_OE (*(volatile uint32_t *)(SIO_BASE + 0x020))
#define SIO_GPIO_OE_SET (*(volatile uint32_t *)(SIO_BASE + 0x024))
#define SIO_GPIO_OE_CLEAR (*(volatile uint32_t *)(SIO_BASE + 0x028))
#define SIO_GPIO_OUT (*(volatile uint32_t *)(SIO_BASE + 0x010))
#define SIO_GPIO_OUT_SET (*(volatile uint32_t *)(SIO_BASE + 0x014))
#define SIO_GPIO_OUT_CLEAR (*(volatile uint32_t *)(SIO_BASE + 0x018))

#define IO_BANK0_BASE 0x40014000u
#define GPIO19_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x09c)) // spio TX
#define GPIO16_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x084)) // spio RX
#define GPIO17_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x08c)) // spio CS
#define GPIO18_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x094)) // spio CLK
#define GPIO20_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x0a4)) // spio C/D
#define GPIO21_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x0ac)) // spio Hardware Reset
#define GPIO_FUNC_SPI0 (1u)
#define GPIO_FUNC_SIO (5u)

#define PADS_BANK0_BASE 0x4001c000u
#define PAD_GPIO19 (*(volatile uint32_t *)(PADS_BANK0_BASE + 0x50))
#define PAD_GPIO16 (*(volatile uint32_t *)(PADS_BANK0_BASE + 0x44))
#define PAD_GPIO17 (*(volatile uint32_t *)(PADS_BANK0_BASE + 0x48))
#define PAD_GPIO18 (*(volatile uint32_t *)(PADS_BANK0_BASE + 0x4c))
#define PAD_GPIO20 (*(volatile uint32_t *)(PADS_BANK0_BASE + 0x54))
#define PAD_GPIO21 (*(volatile uint32_t *)(PADS_BANK0_BASE + 0x58))

#define SPI0_BASE 0x4003c000u
#define SPI0_SSPCR0 (*(volatile uint32_t *)(SPI0_BASE + 0x000))
#define SPI0_SSPCR1 (*(volatile uint32_t *)(SPI0_BASE + 0x004))
#define SPI0_SSPCR1_SSE (1u << 1)
#define SPI0_SSPDR (*(volatile uint32_t *)(SPI0_BASE + 0x008))
#define SPI0_SSPSR (*(volatile uint32_t *)(SPI0_BASE + 0x00c))
#define SPI0_SSPSR_BSY (1u << 4)
#define SPI0_SSPSR_TNF (1u << 1)
#define SPI0_SSPCPSR (*(volatile uint32_t *)(SPI0_BASE + 0x010))

#define TIMER_BASE 0x40054000u
#define TIME_HR (*(volatile uint32_t *)(TIMER_BASE + 0x08))
#define TIME_LR (*(volatile uint32_t *)(TIMER_BASE + 0x0c))

#define CLOCKS_BASE 0x40008000u
#define CLK_PERI_CTRL (*(volatile uint32_t *)(CLOCKS_BASE + 0x48))
#define CLK_PERI_CTRL_ENABLE (1u << 11)

#define RESETS_BASE 0x4000c000u
#define RESETS_RESET (*(volatile uint32_t *)(RESETS_BASE + 0x0))
#define RESETS_RESET_SPI0 (1u << 16)
#define RESETS_RESET_DONE (*(volatile uint32_t *)(RESETS_BASE + 0x8))

uint64_t read_timer(void);
void delay_ms(uint64_t milliseconds);
void delay_us(uint64_t microseconds);

void ili9341_init();
void spi0_init();
void ili9341_hw_reset();
void spi_write_byte(uint8_t byte);
void ili9341_write_command(uint8_t cmd);
void ili9341_write_data(uint8_t data);
void ili9341_send_init_commands();
void ili9341_fill_red(void);

int main(void)
{
    ili9341_init();
    ili9341_fill_red();
    while (1)
    {
    }
}

void ili9341_init()
{
    PAD_GPIO16 = 0x63;
    PAD_GPIO17 = 0x23;
    PAD_GPIO18 = 0x23;
    PAD_GPIO19 = 0x23;
    PAD_GPIO20 = 0x23;
    PAD_GPIO21 = 0x23;

    GPIO16_CTRL = GPIO_FUNC_SPI0; // spi0 rx
    GPIO17_CTRL = GPIO_FUNC_SPI0; // spi0 csn
    GPIO18_CTRL = GPIO_FUNC_SPI0; // spi0 sck
    GPIO19_CTRL = GPIO_FUNC_SPI0; // spi0 tx
    GPIO20_CTRL = GPIO_FUNC_SIO;  // sio d/c pin
    GPIO21_CTRL = GPIO_FUNC_SIO;  // sio hardware reset pin

    SIO_GPIO_OE_SET = (1u << 20) | (1u << 21); // enable output for gpio 20 nd 21
    SIO_GPIO_OUT_SET = (1u << 21);             // rst high
    SIO_GPIO_OUT_CLEAR = (1u << 20);           // clear d/c pin

    spi0_init();

    ili9341_hw_reset();

    ili9341_send_init_commands();
}

void spi0_init(void)
{
    CLK_PERI_CTRL |= CLK_PERI_CTRL_ENABLE;
    RESETS_RESET &= ~(RESETS_RESET_SPI0);
    while (!(RESETS_RESET_DONE & RESETS_RESET_SPI0))
    {
    };

    SPI0_SSPCPSR = 2;
    SPI0_SSPCR0 = 0x0507;
    SPI0_SSPCR1 = SPI0_SSPCR1_SSE;
    delay_ms(100);
}

void ili9341_hw_reset()
{
    SIO_GPIO_OUT_CLEAR = (1u << 21);
    delay_ms(1);
    SIO_GPIO_OUT_SET = (1u << 21);
    delay_ms(120);
}

void spi_write_byte(uint8_t byte)
{
    while (!(SPI0_SSPSR & SPI0_SSPSR_TNF))
    {
    }

    SPI0_SSPDR = byte;
    while (SPI0_SSPSR & SPI0_SSPSR_BSY)
    {
    }
    volatile uint32_t dummy = SPI0_SSPDR;
    (void)dummy;
}

void ili9341_write_command(uint8_t cmd)
{
    SIO_GPIO_OUT_CLEAR = (1u << 20); // DC = low command is sending
    spi_write_byte(cmd);
}

void ili9341_write_data(uint8_t data)
{
    SIO_GPIO_OUT_SET = (1u << 20); // DC = high data is sending
    spi_write_byte(data);
}

void ili9341_send_init_commands(void)
{
    ili9341_write_command(0x11); // exit sleep mode
    delay_ms(120);

    ili9341_write_command(0x3A); // pixel fomart to rgb565
    ili9341_write_data(0x55);

    ili9341_write_command(0x36); // memory access orientation
    ili9341_write_data(0x08);

    ili9341_write_command(0x29); // turn display on
    delay_ms(100);
}

void ili9341_fill_red(void)
{
    ili9341_write_command(0x2A);
    ili9341_write_data(0x00);
    ili9341_write_data(0x00);
    ili9341_write_data(0x00);
    ili9341_write_data(0xEF);

    ili9341_write_command(0x2B);
    ili9341_write_data(0x00);
    ili9341_write_data(0x00);
    ili9341_write_data(0x01);
    ili9341_write_data(0x3F);

    ili9341_write_command(0x2C);

    for (uint32_t i = 0; i < 76800; i++)
    {
        ili9341_write_data(0x1C);
        ili9341_write_data(0x1F);
    }
}

uint64_t read_timer(void)
{
    uint32_t high1, low, high2;
    do
    {
        high1 = TIME_HR;
        low = TIME_LR;
        high2 = TIME_HR;
    } while (high1 != high2);

    return ((uint64_t)high2 << 32) | low;
}

void delay_ms(uint64_t milliseconds)
{
    uint64_t start = read_timer();
    uint64_t target_us = milliseconds * 1000;

    while ((read_timer() - start) < target_us)
    {
        // wait;
    }
}

void delay_us(uint64_t microseconds)
{
    uint64_t start = read_timer();
    uint64_t target_us = microseconds;

    while ((read_timer() - start) < target_us)
    {
        // wait;
    }
}
