#include <stdint.h>
#include "timer.h"

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
#define SPI1_SSPSR_TNF (1u << 1)                                 // Transmit FIFO not full , 1 is set when not full, 0 when full
#define SPI1_SSPSR_RNE (1u << 2)                                 // Receive FIFO not empty , 0 is empty , 1 is not empty
#define SPI1_SSPCPSR (*(volatile uint32_t *)(SPI1_BASE + 0x010)) // clock prescale divisior

#define CLOCKS_BASE 0x40008000u
#define CLK_PERI_CTRL (*(volatile uint32_t *)(CLOCKS_BASE + 0x48))
#define CLK_PERI_CTRL_ENABLE (1u << 11) // start / stops the clock generator

#define RESETS_BASE 0x4000c000u
#define RESETS_RESET (*(volatile uint32_t *)(RESETS_BASE + 0x0))      // reset control
#define RESETS_RESET_SPI1 (1u << 17)                                  // SPI1 Reset bit is 17
#define RESETS_RESET_DONE (*(volatile uint32_t *)(RESETS_BASE + 0x8)) // reset reset done, bit is set when reset done signal has been sent by peripheral

// the first command & repsonse defines
#define SD_R1_TIMEOUT 2000      // time- out value after sending init command
#define R1_IDLE_STATE (1u << 0) // 1 = card still in idle (not ready)
#define R1_ERASE_RESET (1u << 1)
#define R1_ILLEGAL_COMMAND (1u << 2) // 1 = card doesn't know that command
#define R1_CRC_ERROR (1u << 3)
#define R1_ERASE_SEQ_ERROR (1u << 4)
#define R1_ADDRESS_ERROR (1u << 5)
#define R1_PARAMETER_ERROR (1u << 6)
// bit 7 is always 0 in a valid R1 , that's how we detect the response at all

void spi1_init(void)
{

    PAD_GPIO10 = 0x23; // SCK
    PAD_GPIO11 = 0x23; // MOSI (TX)
    PAD_GPIO12 = 0x63; // MISO (RX) - 0x63 = IE set so we can read the card
    GPIO10_CTRL = GPIO_FUNC_SPI1;
    GPIO11_CTRL = GPIO_FUNC_SPI1;
    GPIO12_CTRL = GPIO_FUNC_SPI1;

    CLK_PERI_CTRL |= CLK_PERI_CTRL_ENABLE; // enable clock , by default clock is gated for the spi0 peripheral
    RESETS_RESET &= ~(RESETS_RESET_SPI1);  // do a software reset and wait for reset done signal
    while (!(RESETS_RESET_DONE & RESETS_RESET_SPI1))
    {
    };

    // the clock freq here is 10.42 MHz because of 2 in cpsr and 5 in sspcr0

    SPI1_SSPCPSR = 54;             // this is for spio clock register 386KHz for init slower pulse at the start
    SPI1_SSPCR0 = 0x0507;          // control register of spi0 , the 5 is the clock divisor inside control register 0
    SPI1_SSPCR1 = SPI1_SSPCR1_SSE; // synchronous serial port enable
    delay_ms(100);
}

// remember to make cs of spi1 as gpio cause we need to manually make it low all the time

uint8_t spi1_transfer(uint8_t data)
{
    while (!(SPI1_SSPSR & SPI1_SSPSR_TNF)) // loop exits when tnf bit is 1 and enters when tnf bit is 0 ( 0 means full)
    {
        // wait
    }
    SPI1_SSPDR = data; // send data

    while (!(SPI1_SSPSR & SPI1_SSPSR_RNE)) // loop enters when rne bit is 0 means empty
    {
        // wait
    }
    return (uint8_t)(SPI1_SSPDR & 0xFF); // read reply fifo
}

void cs_init(void)
{
    GPIO13_CTRL = GPIO_FUNC_SIO;   // cs gpio func as sio not spi1
    PAD_GPIO13 &= ~(1u << 7);      // clear disable output bit although it is clear by default
    SIO_GPIO_OE_SET = (1u << 13);  // enable output
    SIO_GPIO_OUT_SET = (1u << 13); // idle cs = high means card ignored
}

void cs_select(void)
{
    SIO_GPIO_OUT_CLEAR = (1u << 13); // cs low is talking to the card
}

void cs_deselect(void)
{
    SIO_GPIO_OUT_SET = (1u << 13); // cs high = done talking
}

void sd_dummy_clocks(void) // dummy clocks at startup
{
    cs_deselect(); // high means sd not being addressed
    for (int i = 0; i <= 10; i++)
    {
        spi1_transfer(0xFF); // 10 bytes is 80 dummy clock pulses as 1 byte = 8 bits
        //  sending streams of 1's in the init even if cs high fails the commands wont be detected by sd as they start as 0
    }
}

// sending command
void sd_send_command(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    spi1_transfer(0x40 | cmd);         // start bit(0) + cmd number as 4 here 4 is 0100
    spi1_transfer((arg >> 24) & 0xFF); // MSB First big endian format
    spi1_transfer((arg >> 16) & 0xFF);
    spi1_transfer((arg >> 8) & 0xFF);
    spi1_transfer(arg & 0xFF); // LSB last
    spi1_transfer(crc);        // crc mostly ignored , only used in cmd0 and cmd8 , contains 7 bits + 1 bit of 1 as a trailing end marker
}

// Poll miso until the top bit is 0 with a timeout i.e. 2000
uint8_t sd_read_r1(void)
{
    uint8_t response;
    for (int i = 0; i < SD_R1_TIMEOUT; i++)
    {
        response = spi1_transfer(0xFF); // same 0xFF always i.e. streams of 1's mosi held high while listening all the time
        if (!(response & 0x80))         // top bit is 0 means a real response byte
        {
            return response;
        }
    }
    return 0xFF; // timeout card never answered
}
// a healthy response is usually 0x01 (just idle) during init, and 0x00 once ready

// cmd0 is go to ideal state , first real command , crc to be checked which is fixed constant 0x95
uint8_t sd_cmd0(void)
{
    uint8_t resp;
    for (int i = 0; i < 10; i++) // sending again & again as cmd0 may or may not register if sent only once
    {
        cs_select();                          // talking to card cs low
        sd_send_command(0, 0x00000000, 0x95); // cmd0 + crc which is 0x95 constant
        resp = sd_read_r1();                  // read reply after sending cmd0
        cs_deselect();                        // cs is high after exchange
        spi1_transfer(0xFF);                  // trailing bits just to register the cs high and complete whatever the internal housekeeping and release the miso line
        if (resp == 0x01)                     // 0x01 means idle state, card is awake on the starting line
        {
            return 1;
        }
    }
    return 0;
}