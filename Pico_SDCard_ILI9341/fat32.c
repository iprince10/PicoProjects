#include <stdint.h>
#include "timer.h"
#include "sdcard.h"
#include "uart.h"

// little endian: lowest offset holds the least significant byte. this is the
// reverse of what sd_send_command puts on the wire

static uint32_t mbr_read_le32(const uint8_t *b)
{
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

static void mbr_show_type(uint8_t type)
{
    switch (type)
    {
    case 0x01:
        uart0_puts("FAT12");
        break;
    case 0x04:
        uart0_puts("FAT16 <32M");
        break;
    case 0x06:
        uart0_puts("FAT16");
        break;
    case 0x0B:
        uart0_puts("FAT32 (CHS)");
        break;
    case 0x0C:
        uart0_puts("FAT32 (LBA)");
        break;
    case 0x0E:
        uart0_puts("FAT16 (LBA)");
        break;
    case 0x07:
        uart0_puts("exFAT / NTFS");
        break;
    case 0x83:
        uart0_puts("Linux");
        break;
    case 0xEF:
        uart0_puts("EFI system");
        break;

    default:
        uart0_puts("unknown");
        break;
    }
}

// MBR Partition Table
#define SD_MBR_PART_OFFSET 446
#define SD_MBR_PART_SIZE 16
#define SD_MBR_PART_COUNT 4
#define SD_MBR_SIG_OFFSET 510

#define SD_PART_EMPTY 0x00
#define SD_PART_EXFAT 0x07
#define SD_PART_FAT32_CHS 0x0B
#define SD_PART_FAT32_LBA 0x0C

uint8_t sd_mbr_parse(uint8_t *mbr, uint32_t *part_start, uint32_t *part_size)
{
    uint8_t chosen_type = 0;
    uint8_t found = 0;

    if (mbr[SD_MBR_SIG_OFFSET] != 0x55 || mbr[SD_MBR_SIG_OFFSET + 1] != 0xAA)
    {
        uart0_puts("MBR: no 0x55AA signature, not a partition table\r\n");
        return 0xFF;
    }
    for (int i = 0; i < SD_MBR_PART_COUNT; i++)
    {
        // entry i sits 16 bytes further along each time
        uint8_t *entry = mbr + SD_MBR_PART_OFFSET + (i * SD_MBR_PART_SIZE); // entry is a pointer to the memory location inside the mbr block
        uint8_t type = entry[4];                                            // 4 bytes forward from the address stored in entry
        uint32_t start = mbr_read_le32(entry + 8);
        uint32_t size = mbr_read_le32(entry + 12);

        if (type == SD_PART_EMPTY || size == 0)
        {
            continue;
        }

        uart0_puts("Part ");
        uart0_putnum(i);
        uart0_puts(": Type 0x");
        uart0_puthex(type);
        uart0_puts(" ");
        mbr_show_type(type);
        uart0_puts("\r\nStart : ");
        uart0_putnum(start);
        uart0_puts(" Size : ");
        uart0_putnum(size);
        uart0_puts(" Sectors / ");
        // divide first: size * 512 overflows 32 bits on a 32 GB card.
        // 1 MB = 2048 sectors of 512, so size/2048 is MB directly.
        uart0_putnum(size / 2048u);
        uart0_puts(" MB\r\n");

        if (!found) // first usable entry wins, we have not looked for more
        {
            *part_start = start;
            *part_size = size;
            chosen_type = type;
            found = 1;
        }
    }
    if (!found)
    {
        uart0_puts("MBR: table valid but no usable partition entry\r\n");
        return 0xFF;
    }

    uart0_puts("Chosen partition: Start block ");
    uart0_putnum(*part_start);
    uart0_puts(" (size ");
    uart0_putnum(*part_size / 2048u);
    uart0_puts(" MB)\r\n");

    if (chosen_type == SD_PART_EXFAT)
    {
        uart0_puts("WARNING: type 0x07 is exFAT, not FAT32.\r\n");
        uart0_puts("FAT32 plan does not apply - reformat or change target.\r\n");
    }
    else if (chosen_type == SD_PART_FAT32_LBA || chosen_type == SD_PART_FAT32_CHS)
    {
        uart0_puts("FAT32 confirmed - its VBR is the sector above\r\n");
    }
    else
    {
        uart0_puts("not FAT32 - check the type above before going further\r\n");
    }

    return 0;
}