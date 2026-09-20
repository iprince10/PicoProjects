#ifndef sdcard_h
#define sdcard_h

void spi1_init(void);
uint8_t spi1_transfer(uint8_t data);
void cs_init(void);
void cs_select(void);
void cs_deselect(void);
void sd_dummy_clocks(void);
void sd_send_command(uint8_t cmd, uint32_t arg, uint8_t crc);
uint8_t sd_read_r1(void);
uint8_t sd_cmd0(void);
uint8_t sd_cmd8(void);

#endif