/*
 * MoistureSensor.c
 *
 *  Created on: Jul 14, 2025
 *      Author: wawer
 */

#include "MoistureSensor.h"

int ScaleADC_Light_To_Percentage(const uint16_t *adc_buffer, size_t size, int adc_dark, int adc_bright)
{
    if (adc_buffer == NULL || size == 0u || adc_bright <= adc_dark)
        return 0;

    uint32_t sum = 0;
    for (size_t i = 0; i < size; i++) {
        sum += adc_buffer[i];
    }

    int avg = (int)(sum / size);

    if (avg < adc_dark) avg = adc_dark;
    if (avg > adc_bright) avg = adc_bright;

    int range = adc_bright - adc_dark;
    int delta = avg - adc_dark;

    int percent = (delta * 100) / range;

    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    return percent;
}

// dry - high ADC value, wet - low ADC value
int ScaleADC_To_Percent_Inverted_Ranged(const uint16_t *adc_buffer, size_t size, int dry_value, int wet_value)
{
    if (adc_buffer == NULL || size == 0u || dry_value <= wet_value)
        return 0;

    uint32_t sum = 0;
    for (size_t i = 0; i < size; i++) {
        sum += adc_buffer[i];
    }

    int avg = (int)(sum / size);

    if (avg > dry_value) avg = dry_value;
    if (avg < wet_value) avg = wet_value;

    int range = dry_value - wet_value;
    int delta = avg - wet_value;

    int percent = 100 - ((delta * 100) / range);

    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    return percent;
}
