/*
 * userWS2812.h
 *
 *  Created on: May 24, 2020
 *      Author: Artem
 */

#ifndef USERWS2812_H_
#define USERWS2812_H_

#include "main.h"
#include "stm32f1xx_hal.h"
#include "tim.h"
#include <stdio.h>
#include <string.h>
#include "UserSD.h"

#define WS2812_1_code      65
#define WS2812_0_code      26

// Стандартные цвета
typedef enum{
	WHITE =		0xFFFFFF,	// белый
	SILVER =	0xC0C0C0,	// серебро
	GRAY =		0x808080,	// серый
	BLACK =		0x000000,	// чёрный
	RED =		0xFF0000,	// красный
	MAROON =	0x800000,	// бордовый
	ORANGE =	0xFF3000,	// оранжевый
	YELLOW =	0xFF8000,	// жёлтый
	OLIVE =		0x808000,	// олива
	LIME =		0x00FF00,	// лайм
	GREEN =		0x008000,	// зелёный
	AQUA =		0x00FFFF,	// аква
	TEAL =		0x008080,	// цвет головы утки чирка
	BLUE =		0x0000FF,	// голубой
	NAVY =		0x000080,	// тёмно-синий
	MAGENTA =	0xFF00FF,	// розовый
	PURPLE =	0x800080,	// пурпурный
} COLORS;

typedef enum
{
	to_play_file = 0,
	to_show_massage
} initialization_reason;

typedef struct {
	uint16_t pixels_quantity;
	uint16_t sended_pixels_counter;
	uint16_t frame_pheriod;//in us/30

	uint8_t contrast;
	uint8_t brightness;

	uint8_t ring_bufer[48];
	uint8_t ring_bufer_size;
	uint8_t ring_bufer_half_size;
	uint8_t write_address_pointer;
} WS2812_t;

uint8_t WS2812_start
		(WS2812_t* WS2812,
		SD_t* data,
		TIM_HandleTypeDef *htim,
		uint32_t tim_channel,
		uint8_t initialization_reason);

uint8_t WS2812_prepare_data (WS2812_t* WS2812, SD_t* data);

void  WS2812_setColor(SD_t* data, int LED_num, COLORS color);
void  WS2812_fill(SD_t* data, COLORS color);
void  WS2812_clear(SD_t* data);

#endif /* USERWS2812_H_ */
