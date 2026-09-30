#include "bitmap.h"
#include "fat32.h"
#include "ili9341.h"
#include "sdcard.h"
#include "timer.h"
#include "uart.h"
#include <stdint.h>
#include <stdio.h>

#define IO_BANK0_BASE 0x40014000u

#define GPIO_FUNC_SIO 5u
#define GPIO25_CTRL (*(volatile uint32_t *)(IO_BANK0_BASE + 0xcc))
#define SIO_BASE 0xd0000000u
#define SIO_GPIO_OE (*(volatile uint32_t *)(SIO_BASE + 0x020))
#define SIO_GPIO_OUT (*(volatile uint32_t *)(SIO_BASE + 0x010))
#define SIO_GPIO_OUT_XOR (*(volatile uint32_t *)(SIO_BASE + 0x01c))
#define GPIO25 (1u << 25)

void led_init(void)
{
  // blink code just for testing
  GPIO25_CTRL = GPIO_FUNC_SIO;
  SIO_GPIO_OE |= GPIO25;
  SIO_GPIO_OUT |= GPIO25;
}

int main()
{
  uart0_init();
  spi1_init();
  sd_dummy_clocks();
  led_init();

  // cmd0
  uart0_puts("CMD0....\r\n");
  if (sd_cmd0())
  {
    uart0_puts("CMD0 Ok - card in idle\r\n");
  }
  else
  {
    uart0_puts("CMD0 FAIL\r\n");
  }

  // cmd8
  uart0_puts("CMD8....\r\n");
  uint8_t r2 = sd_cmd8();
  if (r2 == 1)
  {
    uart0_puts("CMD8 Ok - modern card, token echoed\r\n");
  }
  else if (r2 == 2)
  {
    uart0_puts("CMD8 - V1 card (old)\r\n");
  }
  else
  {
    uart0_puts("CMD8 FAIL\r\n");
  }

  // cmd55 + acmd41
  uart0_puts("CMD55 + ACMD41....\r\n");
  if (sd_acmd41() == 0)
  {
    uart0_puts("ACMD41 Ok - card ready\r\n");
  }
  else
  {
    uart0_puts("ACMD41 FAIL\r\n");
  }

  // cmd58
  uart0_puts("CMD58....\r\n");
  uint8_t r3 = sd_cmd58();

  if (r3 == 1)
  {
    uart0_puts("CMD58 Ok - block addressing (SDHC/SDXC)\r\n");
  }
  else if (r3 == 2)
  {
    uart0_puts("CMD58 - byte addressing (SDSC)\r\n");
  }
  else
  {
    uart0_puts("CMD58 FAIL\r\n");
  }

  sd_set_clk_fast();
  uart0_puts("SPI1 CLK is 12.5 MHz now\r\n");

  // cmd17 sd-read-block
  static uint8_t buf[512]; // static: no large buffers on the stack
  uart0_puts("CMD17 - reading block 0....\r\n");

  if (sd_read_block(0, buf) == 0)
  {
    if (buf[510] == 0x55 && buf[511] == 0xAA)
    {
      uart0_puts("CMD17 Ok - MBR signature 0x55AA found\r\n");
    }
    else
    {
      uart0_puts("CMD17 - read ok, no MBR signature\r\n");
    }
  }
  else
  {
    uart0_puts("CMD17 FAIL\r\n");
  }

  // read-twice integrity test
  uart0_puts("Integrity test....\r\n");
  if (sd_integrity_test(0, 300) == 0)
  {
    uart0_puts("Integrity Ok - 300 blocks read twice, all identical\r\n");
  }
  else
  {
    uart0_puts("Integrity FAIL - data path not clean at this clock\r\n");
  }

  // MBR Parser
  // block 0 is the partition table. the start LBA it hands back is already
  // a block number,
  static uint8_t mbr[512];
  uint32_t part_start = 0;
  uint32_t part_size = 0;

  uart0_puts("MBR - reading block 0....\r\n");
  if (sd_read_block(0, mbr) != 0)
  {
    uart0_puts("MBR FAIL - could not read block 0\r\n");
  }
  else if (sd_mbr_parse(mbr, &part_start, &part_size) == 0)
  {
    uart0_puts("MBR Ok - filesystem starts at block ");
    uart0_putnum(part_start);
    uart0_puts("\r\n");
  }
  else
  {
    uart0_puts("MBR FAIL - no usable partition table\r\n");
  }

  // BPB
  // block part_start is the VBR. its BPB gives the FAT32 geometry, which
  // every later read depends on. no point going on if this fails.
  static uint8_t vbr[512];
  fat_geom_t geom;

  uart0_puts("BPB - reading block ");
  uart0_putnum(part_start);
  uart0_puts("....\r\n");
  if (sd_read_block(part_start, vbr) != 0)
  {
    uart0_puts("BPB FAIL - could not read the VBR\r\n");
  }
  else if (fat32_parse_bpb(vbr, part_start, &geom) == 0)
  {
    uart0_puts("BPB Ok - FAT32 geometry read\r\n");
    uart0_puts("Root dir - listing....\r\n");
    if (fat32_list_root(&geom) == 0)
    {
      uart0_puts("Root dir Ok\r\n");
    }
    else
    {
      uart0_puts("Root dir FAIL\r\n");
    }
  }
  else
  {
    uart0_puts("BPB FAIL - not a usable FAT32 boot sector\r\n");
  }

  while (1)
  {
    SIO_GPIO_OUT_XOR = GPIO25;
    delay_ms(500);
  }
}

// ---- Opened the serial port COM14 ----
// CMD0....
// CMD0 Ok - card in idle
// CMD8....
// CMD8 Ok - modern card, token echoed
// CMD55 + ACMD41....
// ACMD41 Ok - card ready
// CMD58....
// CMD58 Ok - block addressing (SDHC/SDXC)
// SPI1 CLK is 12.5 MHz now
// CMD17 - reading block 0....
// CMD17 Ok - MBR signature 0x55AA found
// Integrity test....
// integrity test done - blocks 300, failures 0
// Integrity Ok - 300 blocks read twice, all identical
// MBR - reading block 0....
// Part 0: Type 0x0C  FAT32 (LBA)
// Start : 2048 Size : 62531584 Sectors / 30533 MiB
// Chosen partition: Start block 2048 (size 30533 MiB)
// FAT32 confirmed - its VBR is the sector above
// MBR Ok - filesystem starts at block 2048
// BPB - reading block 2048....
// BPB: bytes/sector 512, sectors/cluster 64 (32768 byte clusters)
// reserved 1120, FATs 2, sectors/FAT 7632
// total sectors 62531584 (30533 MiB)
// FAT starts at block 3168, data starts at block 18432
// clusters 976800, root cluster 2
// BPB Ok - FAT32 geometry read
// Root dir - listing....
// Root dir: cluster 2, first block 18432
// [DIR]  System Volume Information  cluster 3
// [FILE] Prince.rgb  cluster 6  size 153600
// [FILE] LongNameTesting.txt  cluster 0  size 0
// Root dir done, 3 entries
// Root dir Ok
// ---- Closed serial port COM14 due to disconnection from the machine ----