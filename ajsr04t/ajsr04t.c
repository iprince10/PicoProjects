#include <stdint.h>
#include "pico/stdlib.h"
#include "ajsr04t.pio.h"

#define GPIO2_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x014)) // trigger
#define GPIO3_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0x01c)) // echo

#define PIO_FUNC 6u

#define CTRL (*(volatile uint32_t *)(PIO0_BASE + 0x000))
#define CTRL_SM_ENABLE (1u << 0)

#define PIO_TXF0 (*(volatile uint32_t *)(PIO0_BASE + 0x010))
#define PIO_RXF0 (*(volatile uint32_t *)(PIO0_BASE + 0x020))

#define SM0_EXECCTRL (*(volatile uint32_t *)(PIO0_BASE + 0x0cc))
#define EXECCTRL_JMP_PIN (3u << 24)    // gpio 3 echo pin
#define EXECCTRL_WRAP_TOP (11u << 12)  // top wrap
#define EXECCTRL_WRAP_BOTTOM (1u << 7) // bottom wrap

#define SM0_PINCTRL (*(volatile uint32_t *)(PIO0_BASE + 0x0dc))
#define PINCTRL_SET_COUNT (1u << 26) // single pin
#define PINCTRL_SET_BASE (2u << 5)   // 1 base

#define SM0_CLKDIV (*(volatile uint32_t *)(PIO0_BASE + 0x0c8))
#define CLKDIV_INT ((62u) << 16) // 1us in 2 cycles
#define CLKDIV_FRAC ((128u) << 8)

#define INSTR_MEM(i) (*(volatile uint32_t *)(PIO0_BASE + 0x048 + ((i) * 4)))

void ajsr04t_pio_init(void)
{
    // Load PIO program
    for (int i = 0; i < ajsr04t_program.length; i++)
    {
        INSTR_MEM(i) = ajsr04t_program_instructions[i];
    }

    SM0_EXECCTRL = EXECCTRL_JMP_PIN | EXECCTRL_WRAP_TOP | EXECCTRL_WRAP_BOTTOM;
    SM0_PINCTRL = PINCTRL_SET_COUNT | PINCTRL_SET_BASE;
    SM0_CLKDIV = CLKDIV_INT | CLKDIV_FRAC;

    GPIO2_CTRL = PIO_FUNC;
    GPIO3_CTRL = PIO_FUNC;

    CTRL |= CTRL_SM_ENABLE; // state machine enable
}

void send_timeout_write_fifo(void)
{
    PIO_TXF0 = 40000;
}

uint32_t read_result(void)
{
    uint32_t remaining = PIO_RXF0;

    uint32_t duration_us = 40000 - remaining;
    return duration_us;
}

int main(void){
    ajsr04t_pio_init();
    send_timeout_write_fifo();

    while(1){
       uint32_t duration_us = read_result();
        
    }
}
