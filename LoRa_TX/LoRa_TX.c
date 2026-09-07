#include <stdint.h>

#define IO_BANK0_BASE 0x40014000u
#define GPIO4_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x024))
#define GPIO5_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x02c))
#define GPIO6_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x034))
#define GPIO7_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x03c))
#define GPIO8_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x044))

#define SIO_BASE 0xd0000000u
#define SIO_FUNC 5u
#define SIO_GPIO_IN (*(volatile uint32_t *)(SIO_BASE + 0x004))
#define SIO_GPIO_OE (*(volatile uint32_t *)(SIO_BASE + 0x020))
#define SIO_GPIO_OE_SET (*(volatile uint32_t *)(SIO_BASE + 0x024))
#define SIO_GPIO_OE_CLEAR (*(volatile uint32_t *)(SIO_BASE + 0x028))
#define SIO_GPIO_OUT (*(volatile uint32_t *)(SIO_BASE + 0x010))
#define SIO_GPIO_OUT_SET (*(volatile uint32_t *)(SIO_BASE + 0x014))
#define SIO_GPIO_OUT_CLEAR (*(volatile uint32_t *)(SIO_BASE + 0x018))

#define PAD_BANK0_BASE 0x4001c000u
#define PAD_GPIO8_CTRL (*(volatile uint32_t *)(PAD_BANK0_BASE + 0x24))
#define PAD_GPIO_CTRL_IE (1u << 6)
#define PAD_GPIO_CTRL_PDE (1u << 2)
#define PAD_GPIO_CTRL_PUE (1u << 3)

#define GPIO0_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x004))
#define UART0_FUNC 2u
#define UART0_BASE 0x40034000u
#define UART0_DR (*(volatile uint32_t *)(UART0_BASE + 0x000))
#define UART0_FR (*(volatile uint32_t *)(UART0_BASE + 0x018))
#define UART0_FR_TXFF (1u << 5)
#define UART0_IBRD (*(volatile uint32_t *)(UART0_BASE + 0x024))
#define UART0_FBRD (*(volatile uint32_t *)(UART0_BASE + 0x028))
#define UART0_CR (*(volatile uint32_t *)(UART0_BASE + 0x030))
#define UART0_CR_UARTEN (1u << 0)
#define UART0_CR_TXE_EN (1u << 8)
#define UART0_LCR_H (*(volatile uint32_t *)(UART0_BASE + 0x02c))
#define UART0_LCR_H_FEN (1u << 4)
#define UART0_LCR_H_WLEN (3u << 5)

#define UART1_BASE 0x40038000u
#define UART1_FUNC 2u
#define UART1_DR (*(volatile uint32_t *)(UART1_BASE + 0x000))
#define UART1_FR (*(volatile uint32_t *)(UART1_BASE + 0x018))
#define UART1_FR_TXFF (1u << 5)
#define UART1_FR_RXFE (1u << 4)
#define UART1_IBRD (*(volatile uint32_t *)(UART1_BASE + 0x024))
#define UART1_FBRD (*(volatile uint32_t *)(UART1_BASE + 0x028))
#define UART1_CR (*(volatile uint32_t *)(UART1_BASE + 0x030))
#define UART1_CR_UARTEN (1u << 0)
#define UART1_CR_TXE_EN (1u << 8)
#define UART1_CR_RXE_EN (1u << 9)
#define UART1_LCR_H (*(volatile uint32_t *)(UART1_BASE + 0x02c))
#define UART1_LCR_H_FEN (1u << 4)
#define UART1_LCR_H_WLEN (3u << 5)

#define TIMER_BASE 0x40054000u
#define TIME_HR (*(volatile uint32_t *)(TIMER_BASE + 0x08))
#define TIME_LR (*(volatile uint32_t *)(TIMER_BASE + 0x0c))

void uart1_init(void);
void uart1_putc(char c);
void uart1_puts(const char *send);

void uart0_init(void);
void uart0_putc(char);
void uart0_puts(const char *);

void lora_tx_init(void);
int wait_aux_high(void);

void print_aux_state(void);

uint64_t read_timer(void);
void delay_ms(uint64_t milliseconds);

int main(void)
{
    lora_tx_init();
    delay_ms(1000);

    SIO_GPIO_OUT_SET = (1u << 6) | (1u << 7);
    delay_ms(50);
    uart1_init();
    uart0_init();
    uart0_puts("READY\r\n");

    // ask the module for its version info
    uart1_putc(0xC1);
    uart1_putc(0xC1);
    uart1_putc(0xC1);
    delay_ms(200);

    uart0_puts("RESPONSE: ");
    int got_any = 0;
    for (int i = 0; i < 100; i++)
    {
        if ((UART1_FR & UART1_FR_RXFE) == 0)
        {
            char c = (char)UART1_DR;
            got_any = 1;
            const char hex[] = "0123456789ABCDEF";
            uart0_putc(hex[(c >> 4) & 0xF]);
            uart0_putc(hex[c & 0xF]);
            uart0_putc(' ');
        }
        delay_ms(5);
    }
    if (!got_any)
    {
        uart0_puts("NONE");
    }
    uart0_puts("\r\n");

    while (1)
    {
    }
}

void lora_tx_init(void)
{
    GPIO6_CTRL = SIO_FUNC;
    GPIO7_CTRL = SIO_FUNC;
    GPIO8_CTRL = SIO_FUNC;

    SIO_GPIO_OE_SET = (1u << 6) | (1u << 7);
    SIO_GPIO_OUT_CLEAR = (1u << 6) | (1u << 7);

    SIO_GPIO_OE_CLEAR = (1u << 8); // input enable for gpio 8
    PAD_GPIO8_CTRL |= PAD_GPIO_CTRL_IE;
}

int wait_aux_high(void)
{
    if (SIO_GPIO_IN & (1 << 8))
    {
        return 1;
    }
    return 0;
}

void print_aux_state(void)
{
    if (SIO_GPIO_IN & (1u << 8))
    {
        uart0_puts("AUX HIGH\r\n");
    }
    else
    {
        uart0_puts("AUX LOW\r\n");
    }
}

void uart1_init(void)
{
    UART1_CR = 0;
    GPIO4_CTRL = UART1_FUNC;
    GPIO5_CTRL = UART1_FUNC;

    UART1_IBRD = 813; // for 9600 baud rate
    UART1_FBRD = 51;

    UART1_LCR_H = UART1_LCR_H_FEN | UART1_LCR_H_WLEN; // 8N1 8data, No parity, 1 stop bit, enable fifo

    UART1_CR = UART1_CR_UARTEN | UART1_CR_TXE_EN | UART1_CR_RXE_EN;
}

void uart1_putc(char c)
{
    while ((UART1_FR & UART1_FR_TXFF) != 0)
    {
        // wait while buffer full
    }
    UART1_DR = c;
}

void uart1_puts(const char *send)
{

    while (*send != '\0')
    {
        uart1_putc(*send);
        send++;
    }
}

void uart0_init(void)
{
    UART0_CR = 0;

    GPIO0_CTRL = UART0_FUNC;

    UART0_IBRD = 813;
    UART0_FBRD = 51;

    UART0_LCR_H = UART0_LCR_H_WLEN | UART0_LCR_H_FEN; // 8N1 8data, No parity, 1 stop bit, enable fifo

    UART0_CR = UART0_CR_UARTEN | UART0_CR_TXE_EN;
}

void uart0_putc(char c)
{
    while ((UART0_FR & UART0_FR_TXFF) != 0)
    {
        // wait buffer full
    }
    UART0_DR = c;
}

void uart0_puts(const char *send)
{

    while (*send != '\0')
    {
        uart0_putc(*send);
        send++;
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