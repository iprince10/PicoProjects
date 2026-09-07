#include <stdint.h>
#include <timer.h>

#define IO_BANK0_BASE 0x40014000u
#define PAD_BANK0_BASE 0x4001c000u

#define SIO_BASE 0xd0000000u
#define SIO_OE (*(volatile uint32_t *)(SIO_BASE + 0x20))
#define SIO_OUT (*(volatile uint32_t *)(SIO_BASE + 0x10))

#define LED_PIN_25 25u
#define GPIO_FUNC_SIO 5u
#define GPIO25_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x0cc))

#define GPIO2_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x014))
#define PAD_GPIO2_CTRL (*(volatile uint32_t *)(PAD_BANK0_BASE + 0x0c))
#define GPIO3_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x01c))
#define PAD_GPIO3_CTRL (*(volatile uint32_t *)(PAD_BANK0_BASE + 0x10))
#define PAD_GPIO_CTRL_IE (1u << 6)
#define PAD_GPIO_CTRL_PDE (1u << 2)
#define PAD_GPIO_CTRL_SCHMITT (1u << 1)

#define PROC0_INTR0 (*(volatile uint32_t *)(IO_BANK0_BASE + 0x0f0))
#define PROC0_INTE0 (*(volatile uint32_t *)(IO_BANK0_BASE + 0x100))
#define PROC0_INTS0 (*(volatile uint32_t *)(IO_BANK0_BASE + 0x120))
#define PROC0_INTE0_GPIO3_EDGE_HIGH_IRQ_EN (1u << 15)
#define PROC0_INTR0_GPIO3_HIGH_CLEAR (1u << 15)
#define PROC0_INTE0_GPIO3_EDGE_LOW_IRQ_EN (1u << 14)
#define PROC0_INTR0_GPIO3_LOW_CLEAR (1u << 14)

#define ARM_CORTEX_NVIC_BASE (0xe0000000u)
#define NVIC_ISER (*(volatile uint32_t *)(ARM_CORTEX_NVIC_BASE + 0xe100))
#define NVIC_IO_IRQ_BANK0_EN (1u << 13)

void gpio3_init_input(void);

void gpio2_init_output(void);
void gpio2_set_high(void);
void gpio2_set_low(void);
void sr04t_send_trigger(void);

void isr_irq13(void);

volatile uint8_t rise_captured = 0;
volatile uint64_t rise_time = 0;
volatile uint64_t fail_time = 0;
volatile uint8_t ready_flag = 0;

void gpio2_init_output(void)
{
    GPIO2_CTRL = GPIO_FUNC_SIO;
    SIO_OE |= (1u << 2);
}

void gpio2_set_high(void)
{
    SIO_OUT |= (1u << 2);
}

void gpio2_set_low(void)
{
    SIO_OUT &= ~(1u << 2);
}

void sr04t_send_trigger(void)
{
    gpio2_set_high();
    delay_us(10);
    gpio2_set_low();
}

void gpio3_init_input(void)
{
    GPIO3_CTRL = GPIO_FUNC_SIO;
    SIO_OE &= ~(1u << 3); // input enable for gpio 3
    PAD_GPIO3_CTRL |= PAD_GPIO_CTRL_IE | PAD_GPIO_CTRL_PDE | PAD_GPIO_CTRL_SCHMITT;
    PROC0_INTE0 |= PROC0_INTE0_GPIO3_EDGE_HIGH_IRQ_EN | PROC0_INTE0_GPIO3_EDGE_LOW_IRQ_EN;

    NVIC_ISER |= NVIC_IO_IRQ_BANK0_EN;
}

void isr_irq13(void)
{
    if (PROC0_INTS0 & (1u << 15))
    {
        PROC0_INTR0 = PROC0_INTR0_GPIO3_HIGH_CLEAR;
        rise_time = read_timer();
        rise_captured = 1;
    }
    else if (PROC0_INTS0 & (1u << 14))
    {
        PROC0_INTR0 = PROC0_INTR0_GPIO3_LOW_CLEAR;
        if (rise_captured)
        {
            fail_time = read_timer();
            ready_flag = 1;
            rise_captured = 0;
        }
    }
    else
    {
    }
}