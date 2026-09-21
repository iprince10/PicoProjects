#include <stdint.h>
#include "timer.h"
#include "uart.h"

#define SIO_BASE 0xd0000000u
#define SIO_GPIO_OE (*(volatile uint32_t *)(SIO_BASE + 0x020))
#define SIO_GPIO_OE_SET (*(volatile uint32_t *)(SIO_BASE + 0x024))
#define SIO_GPIO_OE_CLEAR (*(volatile uint32_t *)(SIO_BASE + 0x028))
#define SIO_GPIO_OUT (*(volatile uint32_t *)(SIO_BASE + 0x010))
#define SIO_GPIO_OUT_SET (*(volatile uint32_t *)(SIO_BASE + 0x014))
#define SIO_GPIO_OUT_CLEAR (*(volatile uint32_t *)(SIO_BASE + 0x018))
#define SIO_GPIO_OUT_XOR (*(volatile uint32_t *)(SIO_BASE + 0x01c))
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

void spi1_init(void)
{

    PAD_GPIO10 = 0x23;            // SCK
    PAD_GPIO11 = 0x23;            // MOSI (TX)
    PAD_GPIO12 = 0x63;            // MISO (RX) - 0x63 = IE set to read the card
    GPIO10_CTRL = GPIO_FUNC_SPI1; // sck
    GPIO11_CTRL = GPIO_FUNC_SPI1; // tx mosi
    GPIO12_CTRL = GPIO_FUNC_SPI1; // rx miso

    CLK_PERI_CTRL |= CLK_PERI_CTRL_ENABLE; // enable clock , by default clock is gated for the spi1 peripheral
    RESETS_RESET &= ~(RESETS_RESET_SPI1);  // do a software reset and wait for reset done signal
    while (!(RESETS_RESET_DONE & RESETS_RESET_SPI1))
    {
        // wait
    };

    SPI1_SSPCPSR = 54;             // this is for spio clock register 386KHz for init slower pulse at the start
    SPI1_SSPCR0 = 0x0507;          // control register of spi0 , the 5 is the clock divisor inside control register 0
    SPI1_SSPCR1 = SPI1_SSPCR1_SSE; // synchronous serial port enable
    delay_ms(100);

    // cs init as plain sio
    GPIO13_CTRL = GPIO_FUNC_SIO;   // cs gpio func as sio not spi1
    PAD_GPIO13 &= ~(1u << 7);      // clear disable output bit although it is clear by default
    SIO_GPIO_OE_SET = (1u << 13);  // enable output
    SIO_GPIO_OUT_SET = (1u << 13); // idle cs = high means card ignored
}

// remember to make cs of spi1 as gpio cause we need to manually make it low all the time

// void cs_init(void)
// {
//     GPIO13_CTRL = GPIO_FUNC_SIO;   // cs gpio func as sio not spi1
//     PAD_GPIO13 &= ~(1u << 7);      // clear disable output bit although it is clear by default
//     SIO_GPIO_OE_SET = (1u << 13);  // enable output
//     SIO_GPIO_OUT_SET = (1u << 13); // idle cs = high means card ignored
// }

// makes cs low
void cs_select(void)
{
    SIO_GPIO_OUT_CLEAR = (1u << 13); // cs low is talking to the card
}
// makes cs high
void cs_deselect(void)
{
    SIO_GPIO_OUT_SET = (1u << 13); // cs high = done talking
}

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

void sd_dummy_clocks(void) // dummy clocks at startup
{
    cs_deselect(); // high means sd not being addressed
    for (int i = 0; i < 10; i++)
    {
        spi1_transfer(0xFF); // 10 bytes is 80 dummy clock pulses as 1 byte = 8 bits
        //  sending streams of 1's in the init even if cs high fails the commands wont be detected by sd as they start with 0
    }
}

// sending command , commands consists of 6 bytes:-
// first byte is start bit + cmd number
// next four bytes are argument bytes send in little endian , MSB first in lowest address
// sixth byte is 7 bit CRC + 1 bit of 1 at the end as a end marker
void sd_send_command(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    spi1_transfer(0x40 | cmd);         // start bit(0) + cmd number as 4 here 4 is 0100
    spi1_transfer((arg >> 24) & 0xFF); // MSB First big endian format
    spi1_transfer((arg >> 16) & 0xFF);
    spi1_transfer((arg >> 8) & 0xFF);
    spi1_transfer(arg & 0xFF); // LSB last
    spi1_transfer(crc);        // crc mostly ignored , only used in cmd0 and cmd8 , contains 7 bits + 1 bit of 1 as a trailing end marker
}

// the first command & repsonse defines
#define SD_R1_TIMEOUT 2000      // time- out value after sending init command
#define R1_IDLE_STATE (1u << 0) // 1 = card still in idle (not ready)
#define R1_ERASE_RESET (1u << 1)
#define R1_ILLEGAL_COMMAND (1u << 2) // 1 = card doesn't know that command
#define R1_CRC_ERROR (1u << 3)
#define R1_ERASE_SEQ_ERROR (1u << 4)
#define R1_ADDRESS_ERROR (1u << 5)
#define R1_PARAMETER_ERROR (1u << 6)
// bit 7 is always 0 in a valid R1 , that's how we detect the resp at all

// Poll miso until the top bit is 0 with a timeout i.e. 2000
uint8_t sd_read_r1(void)
{
    uint8_t r1;
    for (int i = 0; i < SD_R1_TIMEOUT; i++)
    {
        r1 = spi1_transfer(0xFF); // same 0xFF always i.e. streams of 1's mosi held high while listening all the time
        if (!(r1 & 0x80))         // top bit is 0 means a real r1 byte
        {
            return r1;
        }
    }
    return 0xFF; // timeout card never answered
}
// a healthy r1 is usually 0x01 (just idle) during init, and 0x00 once ready

// cmd0 is go to ideal state , first real command , crc to be checked which is fixed constant 0x95
uint8_t sd_cmd0(void)
{
    uint8_t r1;
    for (int i = 0; i < 10; i++) // sending again & again as cmd0 may or may not register if sent only once
    {
        cs_select();                          // talking to card cs low
        sd_send_command(0, 0x00000000, 0x95); // cmd0 + crc which is 0x95 constant for CMD 0 always it comes precomputed
        r1 = sd_read_r1();                    // read reply after sending cmd0
        cs_deselect();                        // cs is high after exchange
        spi1_transfer(0xFF);                  // trailing bits just to register the cs high and complete whatever the internal housekeeping and release the miso line
        if (r1 == 0x01)                       // 0x01 means idle state, card is awake on the starting line
        {
            return 1;
        }
    }
    return 0;
}

#define SD_CMD8_ARG 0x000001AAu // bits 11:8 = 2.7-3.6V, bits 7:0 = check pattern 0xAA
#define SD_CMD8_CRC 0x87        // const crc byte of cmd8
// the cmd8 byte responds in 1 status byte + 4 echo bytes
uint8_t sd_cmd8(void)
{
    uint8_t r1, echo[4];

    cs_select(); // cs must stay low during whole cmd8 convo
    sd_send_command(8, SD_CMD8_ARG, SD_CMD8_CRC);
    // 8 is the cmd number
    // arg is 0x000001AA in which 01 is the voltage range specified and AA is the random byte to be echoed back
    // cmd8_CRC is 0x87 , the constant crc for cmd8 except for cmd 0 and cmd 8 , every other cmd uses 0x01 i.e. dont care value
    r1 = sd_read_r1();
    echo[0] = spi1_transfer(0xFF); // the 4 echo bytes (this longer reply is called R7)
    echo[1] = spi1_transfer(0xFF);
    echo[2] = spi1_transfer(0xFF); // this contains the voltage ans if R7
    echo[3] = spi1_transfer(0xFF); // this contains the 0xAA if R7
    cs_deselect();                 // pull cs high
    spi1_transfer(0xFF);           // trailing bits to confirm cs high

    // Debugging HEX block of CMD8 Response
    // uart0_puts("r1=");
    // uart0_puthex(r1);
    // uart0_puts(" echo=");
    // for (int i = 0; i < 4; i++)
    // {
    //     uart0_puthex(echo[i]);
    // }
    // uart0_puts("\r\n");
    if (r1 == 0xFF) // no answer at all
    {
        return 0;
    }
    if (r1 & R1_ILLEGAL_COMMAND) // ancient card: doesn't know CMD8
    {
        return 2;
    }
    if (echo[2] == 0x01 && echo[3] == 0xAA) // modern card + token verified
    {
        return 1;
    }

    return 0; // answered, but token wrong
}

// ACMD41 - the command that actually brings the card to life
// CMD55 is the one-shot prefix: "the next command is an application
// command". It is consumed by that next command, so every ACMD41
// attempt needs its own CMD55. ACMD41 answers 0x01 (busy, idle) until
// the card's internal init finishes, then 0x00 (ready).
#define SD_ACMD41_ARG 0x40000000u // bit 30 = HCS : host supports high capacity cards
#define SD_ACMD41_RETRIES 200     // 200 * 10 ms = 2s ceiling
#define SD_DUMMY_CRC 0x01         // crc is only enforced for cmd0 and cmd8 ; 0x01 is all crc bits 0 _ end bit 1.

// application command 41
uint8_t sd_acmd41(void)
{
    uint8_t r1;
    for (int i = 0; i < SD_ACMD41_RETRIES; i++)
    {
        cs_select();                                   // must stay low for one exchange cmd55 + acmd41
        sd_send_command(55, 0x00000000, SD_DUMMY_CRC); // cmd number 55, all arg bits 0 and dummy crc bit
        r1 = sd_read_r1();

        if (r1 > 0x01) // 0x01 is the repsonse when card is in idle state, anything except idle is error
        {
            cs_deselect();       // cs high
            spi1_transfer(0xFF); // to register the cs high and clear out any trailing
            return 0xFF;
        }

        sd_send_command(41, 0x40000000, SD_DUMMY_CRC);
        r1 = sd_read_r1();
        cs_deselect();       // cs high
        spi1_transfer(0xFF); // to register the cs high and clear out any trailing

        if (r1 == 0x00)
        { // idle bit cleared -> card is ready
            return 0;
        }
        if (r1 > 0x01)
        { // any error bit set is  give up
            return 0xFF;
        }
        delay_ms(10);
    }
    return 0xFF; // never finished in time
}

// CMD58 - read the OCR (Operating Conditions Register)
// Reply is R3: R1 + 4 OCR bytes (MSB first).
//   OCR bit 31 = power-up busy : 1 = fully powered, 0 = still warming up
//   OCR bit 30 = CCS           : 1 = block addressing (SDHC/SDXC)
//   CCS : Card Capacity Status   0 = byte addressing (SDSC, old cards)
//   OCR bits 23:15 = accepted voltage window

uint8_t sd_cmd58(void)
{
    uint8_t r1, ocr[4];

    cs_select();                                   // cs low
    sd_send_command(58, 0x00000000, SD_DUMMY_CRC); // cmd number 55, no arg bits all zeroes, dont care or dummy crc
    r1 = sd_read_r1();
    ocr[0] = spi1_transfer(0xFF); // OCR bits 31:24: busy + CCS are here at 31 and 30 bit
    ocr[1] = spi1_transfer(0xFF); // OCR bits 23:16
    ocr[2] = spi1_transfer(0xFF); // OCR bits 15:8
    ocr[3] = spi1_transfer(0xFF); // OCR bits 7:0
    cs_deselect();                // cs high
    spi1_transfer(0xFF);          // confirm cs

    if (r1 != 0x00) // command itself failed
    {
        return 0;
    }

    if (!(ocr[0] & 0x80)) // bit 31 clear -> still powering up
    {
        return 0; 
    }

    if (ocr[0] & 0x40) // bit 30 set -> block addressing (SDHC)
    {
        return 1;
    }
    else   // bit 30 clear -> byte addressing (SDSC)
    {
        return 2; 
    }
}
