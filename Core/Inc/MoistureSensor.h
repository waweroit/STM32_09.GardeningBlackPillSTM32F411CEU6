/*
 * MoistureSensor.h
 *
 *  Created on: Jul 14, 2025
 *      Author: wawer
 */

#ifndef INC_MOISTURESENSOR_H_
#define INC_MOISTURESENSOR_H_

#include <stddef.h>
#include <stdint.h>

int ScaleADC_Light_To_Percentage(const uint16_t *adc_buffer, size_t size, int adc_dark, int adc_bright);
int ScaleADC_To_Percent_Inverted_Ranged(const uint16_t *adc_buffer, size_t size, int dry_value, int wet_value);

#endif /* INC_MOISTURESENSOR_H_ */
