#ifndef sr04t_h
#define sr04t_h

extern void uart0_init(void);
extern void uart0_putc(char);
extern void uart0_puts(const char *);
extern void uart0_putnum(uint64_t num);

extern void gpio3_init_input(void);

extern void gpio2_init_output(void);
extern void gpio2_set_high(void);
extern void gpio2_set_low(void);
extern void sr04t_send_trigger(void);

extern void isr_irq13(void);

extern volatile uint8_t rise_captured;
extern volatile uint64_t rise_time;
extern volatile uint64_t fail_time;
extern volatile uint8_t ready_flag;

#endif
