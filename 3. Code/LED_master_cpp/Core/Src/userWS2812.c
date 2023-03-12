/*
 * userWS2812.c
 *
 *  Created on: May 24, 2020
 *      Author: Artem
 */

#include "userWS2812.h"
uint8_t WS2812_start
	(WS2812_t* WS2812,
	SD_t* data,
	TIM_HandleTypeDef *htim,
	uint32_t tim_channel,
	uint8_t initialization_reason)
{
	HAL_TIM_PWM_Stop_DMA(htim, tim_channel);

	WS2812->ring_bufer_size=sizeof (WS2812->ring_bufer);
	WS2812->ring_bufer_half_size=WS2812->ring_bufer_size/2;
	WS2812->sended_pixels_counter=0;
	WS2812->write_address_pointer=0;

	if (initialization_reason == to_play_file) {
		WS2812->contrast = data->contrast;
		WS2812->brightness = WS2812->contrast*((float)data->brightness/100);
		WS2812->pixels_quantity=data->file_head.pixels_quantity;
		WS2812->frame_pheriod=(1000000/data->file_head.frame_rate)/30;/*(us/30)*/
	} else if (initialization_reason == to_show_massage){
		WS2812->contrast = 65;
		WS2812->brightness = WS2812->contrast*((float)15/100);
		WS2812->pixels_quantity = 256;
		WS2812->frame_pheriod = WS2812->pixels_quantity*2;/*( us/30)*/
		data->ring_bufer_size = 256*3;
		data->read_address_pointer = 3;/*for first WS2812_prepare_data*/
	}

	return HAL_TIM_PWM_Start_DMA(htim, tim_channel, (uint32_t*)WS2812->ring_bufer, WS2812->ring_bufer_size);
}

uint8_t WS2812_prepare_data (WS2812_t* WS2812, SD_t* data){

	WS2812->sended_pixels_counter++;

	//1:
	if (WS2812->sended_pixels_counter == WS2812->frame_pheriod) {
		WS2812->sended_pixels_counter=0;
	}

	//2:
	if (WS2812->sended_pixels_counter == WS2812->pixels_quantity || WS2812->sended_pixels_counter == WS2812->pixels_quantity+1){

		for (uint8_t i = 0; i < WS2812->ring_bufer_half_size; i++) {
			WS2812->ring_bufer[WS2812->write_address_pointer++] = 0;
		}
		if (WS2812->write_address_pointer == WS2812->ring_bufer_size) {
			WS2812->write_address_pointer = 0;
		}
		return HAL_OK;
	}

	//3:
	if (WS2812->sended_pixels_counter < WS2812->pixels_quantity){
		uint8_t RGB_buf[3];
		for (uint8_t i = 0; i < 3; i++) {
			RGB_buf[i] = data->ring_bufer[data->read_address_pointer];
			RGB_buf[i] /= WS2812->contrast;
			RGB_buf[i] *= WS2812->brightness;
			data->read_address_pointer++;
			if (data->read_address_pointer == data->ring_bufer_size) {
				data->read_address_pointer = 0;
				data->write_address_pointer = data->ring_bufer_half_size;
				data->total_read_byte_counter += data->ring_bufer_half_size;
				data->update_flag = 1;
			}
			else if (data->read_address_pointer == data->ring_bufer_half_size) {
				data->write_address_pointer = 0;
				data->total_read_byte_counter += data->ring_bufer_half_size;
				data->update_flag = 1;
			}
		}

		//green
		uint8_t mask = 0b10000000;
		for (uint8_t i = 0; i < 8; i++) {
			WS2812->ring_bufer[WS2812->write_address_pointer++] = RGB_buf[1]&mask ? WS2812_1_code:WS2812_0_code;
			mask >>= 1;
		}

		//red
		mask = 0b10000000;
		for (uint8_t i = 0; i < 8; i++) {
			WS2812->ring_bufer[WS2812->write_address_pointer++] = RGB_buf[0]&mask ? WS2812_1_code:WS2812_0_code;
			mask >>= 1;
		}

		//blue
		mask = 0b10000000;
		for (uint8_t i = 0; i < 8; i++) {
			WS2812->ring_bufer[WS2812->write_address_pointer++] = RGB_buf[2]&mask ? WS2812_1_code:WS2812_0_code;
			mask >>= 1;
		}

	}

	else WS2812->write_address_pointer += WS2812->ring_bufer_half_size;

	if (WS2812->write_address_pointer == WS2812->ring_bufer_size) {
		WS2812->write_address_pointer = 0;
	}

	return HAL_OK;
}

void  WS2812_setColor(SD_t* data, int LED_num, COLORS color) {
	uint16_t pos = LED_num*3;
	data->ring_bufer[pos++] = (color >> 16) & 0xFF;
	data->ring_bufer[pos++] = (color >>  8) & 0xFF;
	data->ring_bufer[pos] = (color >>  0) & 0xFF;
}

void  WS2812_fill(SD_t* data, COLORS color) {
	for (int LED_num = sizeof(data->ring_bufer)/3; LED_num >= 0; LED_num--) {
		WS2812_setColor(data, LED_num, color);
	}
}

void  WS2812_clear(SD_t* data) {
	memset(data->ring_bufer, 0x00, sizeof(data->ring_bufer));
}
