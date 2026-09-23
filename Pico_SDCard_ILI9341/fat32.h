#ifndef fat32_h
#define fat32_h

uint8_t sd_mbr_parse(uint8_t *mbr, uint32_t *part_start, uint32_t *part_size);
#endif