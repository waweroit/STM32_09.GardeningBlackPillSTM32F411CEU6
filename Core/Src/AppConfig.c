/*
 * AppConfig.c
 *
 * Application configuration stored in flash.
 */

#include "AppConfig.h"
#include <stddef.h>

void AppConfig_SetDefaults(DataFlash *config)
{
    if (config == NULL)
        return;

    config->magic = APP_CONFIG_FLASH_MAGIC;
    config->Sensor1DryValue = 2700u;
    config->Sensor1WetValue = 2020u;
    config->Sensor2DryValue = 2700u;
    config->Sensor2WetValue = 2020u;
    config->GetMoistureAvg_TimeLenghtDefault = 20000u;
    config->GetMoistureAvg_TimeLenghtFast = 1000u;
    config->StartPumpWhenMoistureLower = 40u;
    config->StopPumpWhenMoistureHigher = 75u;
    config->lightTime = 3600u * 1000u * 10u;
}

bool AppConfig_IsValid(const DataFlash *config)
{
    if (config == NULL)
        return false;

    if (config->magic != APP_CONFIG_FLASH_MAGIC)
        return false;

    if (config->Sensor1WetValue >= config->Sensor1DryValue || config->Sensor1DryValue > 4095u)
        return false;

    if (config->Sensor2WetValue >= config->Sensor2DryValue || config->Sensor2DryValue > 4095u)
        return false;

    if (config->GetMoistureAvg_TimeLenghtFast == 0u)
        return false;

    if (config->GetMoistureAvg_TimeLenghtDefault == 0u)
        return false;

    if (config->GetMoistureAvg_TimeLenghtFast > config->GetMoistureAvg_TimeLenghtDefault)
        return false;

    if (config->StartPumpWhenMoistureLower >= config->StopPumpWhenMoistureHigher)
        return false;

    if (config->StopPumpWhenMoistureHigher > 100u)
        return false;

    if (config->lightTime == 0u)
        return false;

    return true;
}
