/*
 Battery.cpp - Battery library
 Copyright (c) 2014 Roberto Lo Giacco.

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as 
 published by the Free Software Foundation, either version 3 of the 
 License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Battery.h"

Battery::Battery(uint16_t minVoltage, uint16_t maxVoltage) {
	this->minVoltage = minVoltage;
	this->maxVoltage = maxVoltage;
}

void Battery::begin(uint16_t refVoltage, float dividerRatio, mapFn_t mapFunction) {
	this->refVoltage = refVoltage;
	this->dividerRatio = dividerRatio;
	this->mapFunction = mapFunction ? mapFunction : &linear;
}


uint8_t Battery::level() {
	return this->level(this->voltage());
}

uint8_t Battery::level(uint16_t voltage) {
	if (voltage <= minVoltage) {
		return 0;
	} else if (voltage >= maxVoltage) {
		return 100;
	} else {
		return (*mapFunction)(voltage, minVoltage, maxVoltage);
	}
}

uint16_t Battery::voltage() {
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1,100);
	uint16_t reading = HAL_ADC_GetValue(&hadc1) * dividerRatio * refVoltage / 4096;
	HAL_ADC_Stop(&hadc1);
	return reading;
}
