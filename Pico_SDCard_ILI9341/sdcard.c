#include <stdint.h>
#include <timer.h>

#define SIO_BASE 0xd0000000u
#define SIO_GPIO_OE (*(volatile uint32_t *)(SIO_BASE + 0x020))
#define SIO_GPIO_OE_SET (*(volatile uint32_t *)(SIO_BASE + 0x024))
#define SIO_GPIO_OE_CLEAR (*(volatile uint32_t *)(SIO_BASE + 0x028))
#define SIO_GPIO_OUT (*(volatile uint32_t *)(SIO_BASE + 0x010))
#define SIO_GPIO_OUT_SET (*(volatile uint32_t *)(SIO_BASE + 0x014))
#define SIO_GPIO_OUT_CLEAR (*(volatile uint32_t *)(SIO_BASE + 0x018))
#define SIO_GPIO_OUT_XOR (*(volatile uint32_t *)(SIO_BASE + 0x01d))
#define SIO_GPIO_IN (*(volatile uint32_t *)(SIO_BASE + 0x004))

#define IO_BANK0_BASE 0x40014000u
#define GPIO10_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x054)) // spi1 SCK
#define GPIO11_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x05c)) // spi1 TX
#define GPIO12_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x064)) // spi1 RX
#define GPIO13_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x06c)) // spio CSn
#define GPIO_FUNC_SPI1 (1u)
#define GPIO_FUNC_SIO (5u)

#define PADS_BANK0_BASE 0x4001c000u
#define PAD_GPIO10 (*(volatile uint32_t *)(PADS_BANK0_BASE + 0x2c))
#define PAD_GPIO11 (*(volatile uint32_t *)(PADS_BANK0_BASE + 0x30))
#define PAD_GPIO12 (*(volatile uint32_t *)(PADS_BANK0_BASE + 0x34))
#define PAD_GPIO13 (*(volatile uint32_t *)(PADS_BANK0_BASE + 0x38))

#define SPI1_BASE 0x40040000u
#define SPI1_SSPCR0 (*(volatile uint32_t *)(SPI1_BASE + 0x000)) // synchronous serial port control register
#define SPI1_SSPCR1 (*(volatile uint32_t *)(SPI1_BASE + 0x004))
#define SPI1_SSPCR1_SSE (1u << 1)                                // synchronous serial port enable
#define SPI1_SSPDR (*(volatile uint32_t *)(SPI1_BASE + 0x008))   // data
#define SPI1_SSPSR (*(volatile uint32_t *)(SPI1_BASE + 0x00c))   // status
#define SPI1_SSPSR_BSY (1u << 4)                                 // busy flag
#define SPI1_SSPSR_TNF (1u << 1)                                 // Transmit FIFO not full , 1 is set when not full
#define SPI1_SSPCPSR (*(volatile uint32_t *)(SPI1_BASE + 0x010)) // clock prescale divisior

#define CLOCKS_BASE 0x40008000u
#define CLK_PERI_CTRL (*(volatile uint32_t *)(CLOCKS_BASE + 0x48))
#define CLK_PERI_CTRL_ENABLE (1u << 11) // start / stops the clock generator

#define RESETS_BASE 0x4000c000u
#define RESETS_RESET (*(volatile uint32_t *)(RESETS_BASE + 0x0))      // reset control
#define RESETS_RESET_SPI1 (1u << 17)                                  // SPI1 Reset bit is 17
#define RESETS_RESET_DONE (*(volatile uint32_t *)(RESETS_BASE + 0x8)) // reset reset done, bit is set when reset done signal has been sent by peripheral

void spi1_init(void)
{
    CLK_PERI_CTRL |= CLK_PERI_CTRL_ENABLE; // enable clock , by default clock is gated for the spi0 peripheral
    RESETS_RESET &= ~(RESETS_RESET_SPI1);  // do a software reset and wait for reset done signal
    while (!(RESETS_RESET_DONE & RESETS_RESET_SPI1))
    {
    };

    // the clock freq here is 10.42 MHz because of 2 in cpsr and 5 in sspcr0

    SPI1_SSPCPSR = 2;              // this is for spio clock register
    SPI1_SSPCR0 = 0x0507;          // control register of spi0 , the 5 is the clock divisor inside control register 0
    SPI1_SSPCR1 = SPI1_SSPCR1_SSE; // synchronous serial port enable
    delay_ms(100);
}
