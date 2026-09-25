#include <stdint.h>
#include "timer.h"
#include "sdcard.h"
#include "uart.h"
#include "fat32.h"

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

// MBR partition table
// Block 0 is the Master Boot Record. Fixed layout, independent of any
// filesystem:
//   bytes 0..445   : bootstrap code, ignored
//   bytes 446..509 : four 16-byte partition entries
//   bytes 510..511 : signature 0x55 0xAA
// each 16-byte entry:
//   +0  boot flag  (0x80 bootable / 0x00 not)
//   +1  start CHS  (3 bytes) - obsolete, ignored
//   +4  partition TYPE  <- what filesystem
//   +5  end CHS    (3 bytes) - obsolete, ignored
//   +8  start LBA  (4 bytes, little endian)
//   +12 size        (4 bytes, little endian, in sectors)
// empty slots have type 0x00. there is no rule the partition sits in
// slot 0, so all four get scanned.
#define SD_MBR_PART_OFFSET 446
#define SD_MBR_PART_SIZE 16
#define SD_MBR_PART_COUNT 4
#define SD_MBR_SIG_OFFSET 510

#define SD_PART_EMPTY 0x00
#define SD_PART_EXFAT 0x07
#define SD_PART_FAT32_CHS 0x0B
#define SD_PART_FAT32_LBA 0x0C

// parse the MBR in mbr[512]. prints every non-empty entry found.
// on success fills *part_start with the first usable start LBA (this is
// already a block number, SDHC does the byte->block math) and
// *part_size with its size in sectors. returns 0 on success, 0xFF if the
// signature or the table is no good.
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
        uart0_puts(" MiB\r\n");

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
    uart0_puts(" MiB)\r\n");

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

// BPB (BIOS Parameter Block)
// The VBR sits at part_start (MBR gave 2048). Its first bytes are the BPB , all little Endian same as MBR entries
// offsets from the top of the VBR sector :
// 11  bytes per sector    (2) usually 512
// 13  sectors per cluster (1) power of 2 usually 64
// 14  reserved sectors    (2) sectors before FAT 1
// 16  number of FATs      (1) usually 2
// 17  root entry count    (2) 0 on FAT32
// 22  FAT size 16         (2) 0 on FAT 32
// 32  total sectors 32    (4)  sectors in the volume
// 36  FAT size 32         (4)  sectors per FAT  <- the real one
// 44  root cluster        (4)  usually 2
// 510 signature           (2)  0x55 0xAAF

#define FAT_BPB_BYTES_PER_SECT 11
#define FAT_BPB_SEC_PER_CLUS 13
#define FAT_BPB_RSVD_SEC 14
#define FAT_BPB_NUM_FATS 16
#define FAT_BPB_ROOT_ENT 17
#define FAT_BPB_FATSZ16 22
#define FAT_BPB_TOTSEC32 32
#define FAT_BPB_FATSZ32 36
#define FAT_BPB_ROOT_CLUS 44
#define FAT_BPB_SIG 510

// two-byte little endian read
static uint16_t bpb_read_le16(const uint8_t *b)
{
    return (uint16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8));
}

// parse the BPB in bpb[512] , the VBR read from block part_start.
// fills *g with the raw fields and the derived absolute block addresses
// return 0 on success , 0xFF if this is not a usable FAT32 boot sector
uint8_t fat32_parse_bpb(uint8_t *bpb, uint32_t part_start, fat_geom_t *g)
{
    // signature check first
    if (bpb[FAT_BPB_SIG] != 0x55 || bpb[FAT_BPB_SIG + 1] != 0xAA)
    {
        uart0_puts("BPB : no 0x55AA signature , not a boot sector\r\n");
        return 0xFF;
    }

    // raw fields
    g->part_start = part_start;
    g->bytes_per_sector = bpb_read_le16(bpb + FAT_BPB_BYTES_PER_SECT);
    g->sectors_per_cluster = bpb[FAT_BPB_SEC_PER_CLUS];
    g->reserved_sectors = bpb_read_le16(bpb + FAT_BPB_RSVD_SEC);
    g->num_fats = bpb[FAT_BPB_NUM_FATS];
    g->total_sectors = mbr_read_le32(bpb + FAT_BPB_TOTSEC32);
    g->sectors_per_fat = mbr_read_le32(bpb + FAT_BPB_FATSZ32);
    g->root_cluster = mbr_read_le32(bpb + FAT_BPB_ROOT_CLUS);
    // FAT32 marks itself by leaving FATSz16 and RootEntCnt zero. either one
    // nonzero means this is really FAT12/16 and the layout below is wrong.
    uint16_t fatsz16 = bpb_read_le16(bpb + FAT_BPB_FATSZ16);
    uint16_t rootent = bpb_read_le16(bpb + FAT_BPB_ROOT_ENT);

    if (fatsz16 != 0 || rootent != 0)
    {
        uart0_puts("BPB: FATSz16/RootEntCnt nonzero - this is FAT12/16\r\n");
        return 0xFF;
    }

    // sectors per cluster must be power of two; 0 means read junk
    if (g->sectors_per_cluster == 0 || (g->sectors_per_cluster & (g->sectors_per_cluster - 1)) != 0)
    {
        uart0_puts("BPB: sectors per cluster not a power of two\r\n");
        return 0xFF;
    }

    // derived addresses - add part_start to turn the BPB's partition relative
    // offsets into absolute blocks for sd_read_block()
    // FATs come first, right after the reserved area
    g->fat_start = g->part_start + g->reserved_sectors;
    // then the data region, right after every FAT copy
    g->data_start = g->part_start + g->reserved_sectors + (uint32_t)g->num_fats * g->sectors_per_fat;

    // how many clusters fit in the data region. clusters are counted from 2,
    // but the count is just data sectors / sectors per cluster
    uint32_t overhead = g->reserved_sectors + (uint32_t)g->num_fats * g->sectors_per_fat;
    uint32_t data_sectors = g->total_sectors - overhead;
    g->cluster_count = data_sectors / g->sectors_per_cluster;

    // ---- report ----
    uart0_puts("BPB: bytes/sector ");
    uart0_putnum(g->bytes_per_sector);
    uart0_puts(", sectors/cluster ");
    uart0_putnum(g->sectors_per_cluster);
    uart0_puts(" (");
    uart0_putnum((uint32_t)g->sectors_per_cluster * g->bytes_per_sector);
    uart0_puts(" byte clusters)\r\n");

    uart0_puts("reserved ");
    uart0_putnum(g->reserved_sectors);
    uart0_puts(", FATs ");
    uart0_putnum(g->num_fats);
    uart0_puts(", sectors/FAT ");
    uart0_putnum(g->sectors_per_fat);
    uart0_puts("\r\n");

    uart0_puts("total sectors ");
    uart0_putnum(g->total_sectors);
    uart0_puts(" (");
    uart0_putnum(g->total_sectors / 2048u); // divide first, same reason as MBR
    uart0_puts(" MiB)\r\n");

    uart0_puts("FAT starts at block ");
    uart0_putnum(g->fat_start);
    uart0_puts(", data starts at block ");
    uart0_putnum(g->data_start);
    uart0_puts("\r\n");

    uart0_puts("clusters ");
    uart0_putnum(g->cluster_count);
    uart0_puts(", root cluster ");
    uart0_putnum(g->root_cluster);
    uart0_puts("\r\n");

    // the whole block model leans on 512 byte sectors. warn if it is not.
    if (g->bytes_per_sector != 512)
    {
        uart0_puts("WARNING: bytes/sector is not 512, block model breaks\r\n");
    }

    // must fit under the 28 bit FAT32 entry ceiling
    if (g->cluster_count >= 268435456u)
    {
        uart0_puts("WARNING: cluster count over 2^28, not valid FAT32\r\n");
    }
    return 0;
}
