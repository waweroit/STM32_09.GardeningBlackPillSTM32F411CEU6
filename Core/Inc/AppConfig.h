/*
 * AppConfig.h
 *
 * Application configuration stored in flash.
 */

#ifndef INC_APPCONFIG_H_
#define INC_APPCONFIG_H_

#include <stdbool.h>
#include <stdint.h>

#define APP_CONFIG_FLASH_MAGIC 0xA55A1235u

typedef struct {
    uint32_t magic;
    uint32_t Sensor1DryValue;
    uint32_t Sensor1WetValue;
    uint32_t Sensor2DryValue;
    uint32_t Sensor2WetValue;
    uint32_t GetMoistureAvg_TimeLenghtDefault;
    uint32_t GetMoistureAvg_TimeLenghtFast;
    uint32_t StartPumpWhenMoistureLower;
    uint32_t StopPumpWhenMoistureHigher;
    uint32_t lightTime;
} DataFlash;

void AppConfig_SetDefaults(DataFlash *config);
bool AppConfig_IsValid(const DataFlash *config);

#endif /* INC_APPCONFIG_H_ */
