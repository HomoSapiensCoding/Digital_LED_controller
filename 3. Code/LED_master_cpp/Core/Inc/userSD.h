#ifndef USERSD_H_
#define USERSD_H_

#include "diskio.h"
#include "fatfs.h"
#include "main.h"
#include "spi.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPI_TIMEOUT 100
#define FILE_END 1

/* Definitions for MMC/SDC command */
#define CMD0     (0x40+0)     /* GO_IDLE_STATE */
#define CMD1     (0x40+1)     /* SEND_OP_COND */
#define CMD8     (0x40+8)     /* SEND_IF_COND */
#define CMD9     (0x40+9)     /* SEND_CSD */
#define CMD10    (0x40+10)    /* SEND_CID */
#define CMD12    (0x40+12)    /* STOP_TRANSMISSION */
#define CMD16    (0x40+16)    /* SET_BLOCKLEN */
#define CMD17    (0x40+17)    /* READ_SINGLE_BLOCK */
#define CMD18    (0x40+18)    /* READ_MULTIPLE_BLOCK */
#define CMD23    (0x40+23)    /* SET_BLOCK_COUNT */
#define CMD24    (0x40+24)    /* WRITE_BLOCK */
#define CMD25    (0x40+25)    /* WRITE_MULTIPLE_BLOCK */
#define CMD41    (0x40+41)    /* SEND_OP_COND (ACMD) */
#define CMD55    (0x40+55)    /* APP_CMD */
#define CMD58    (0x40+58)    /* READ_OCR */

typedef struct {
	uint8_t protocol_number;
	uint8_t frame_rate;
	uint16_t pixels_quantity;
	uint32_t frames_quantity;
	uint32_t bytes_quantity_in_frames;
	uint32_t bytes_quantity_in_file;
} file_header_t;

typedef struct {
	FRESULT mount,
			open,
			close,
			read,
			write,
			compare,
			update,
			lseek;
		} file_result_t;

typedef struct {
	FATFS file_system;
	FIL file;
	char file_name[32];
	uint8_t file_num;
	uint8_t brightness;
	uint8_t contrast;
	uint8_t radio_channel_num;
	uint8_t status;

	UINT write_byte_counter;
	UINT read_byte_counter;
	file_result_t result;
	uint8_t repeate_mode;

	file_header_t file_head;
	uint8_t change_file_quantity;
	uint8_t ring_bufer[1024];
	uint16_t ring_bufer_size;
	uint16_t ring_bufer_half_size;
	volatile uint8_t update_flag;
	volatile uint8_t play_flag;
	volatile uint16_t write_address_pointer;
	volatile uint16_t read_address_pointer;
	volatile uint32_t total_read_byte_counter;;
} SD_t;

//driver function
DSTATUS SD_disk_initialize (BYTE pdrv);
DSTATUS SD_disk_status (BYTE pdrv);
DRESULT SD_disk_read (BYTE pdrv, BYTE* buff, DWORD sector, UINT count);
DRESULT SD_disk_write (BYTE pdrv, const BYTE* buff, DWORD sector, UINT count);
DRESULT SD_disk_ioctl (BYTE pdrv, BYTE cmd, void* buff);

//***********************user's***************************
typedef enum
{
	brightness_pos = 22,
	contrast_pos = brightness_pos+29,
	radio_channel_num_pos = contrast_pos+29,
	file_name_pos = radio_channel_num_pos+29,
	repeate_mode_pos = file_name_pos+29
} SD_settins_pos;

typedef enum
{
	repeate_off = 0,
	repeate_one,
	repeate_all
} repeate_modes;

uint8_t SD_test (SD_t* SD_data, const char* name);
uint8_t SD_read_settings (SD_t* SD_data);
uint8_t SD_change_file (SD_t* SD_data, int8_t direction, uint8_t trying_quantity);
uint8_t SD_prepare_file (SD_t* SD_data);
uint8_t SD_update_bufer (SD_t* SD_data);

//the user must call this function every millisecond
void 	SD_1msTick(void);

#endif /* USERUSART_H_ */
