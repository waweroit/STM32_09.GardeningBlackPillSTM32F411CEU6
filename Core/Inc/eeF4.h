#pragma once

#include "main.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_flash_ex.h"

/*
 * KONFIGURACJA:
 * Domyślnie: sektor 7 dla F411 512KB (0x08060000, 128KB).
 * Jeśli masz 256KB (F411xC), sensowny “ostatni” to sektor 6 (0x08040000, 128KB).
 */
#ifndef EE_FLASH_BASE
#define EE_FLASH_BASE   0x08060000UL
#endif

#ifndef EE_FLASH_SECTOR
#define EE_FLASH_SECTOR FLASH_SECTOR_7
#endif

#ifndef EE_FLASH_SECTOR_SIZE
#define EE_FLASH_SECTOR_SIZE (128UL * 1024UL)
#endif

#define FLASH_INIT   EE_FLASH_BASE

/* Zachowuję Twoją koncepcję “stron” 2KB jako offsety w ramach sektora */
#define PAGE_SECTOR  2048U
#define DATA_SPACE   8U

/* To nadal jest “numer strony 2KB w sektorze”, nie prawdziwy “page” Flash F4 */
#define DATA_PAGE    14

void flash_clear_all_flags(void);
uint32_t getHexAddressPage(int dataPage);
HAL_StatusTypeDef wait_flash_ready(uint32_t WaitForLastOperation_ms, uint32_t overall_ms);
uint32_t retrieveDataFromAddress(uint32_t hexAddress);
HAL_StatusTypeDef writeThreeData(uint32_t hexPage, int dataA, int dataB, int dataC);
HAL_StatusTypeDef memoryPageErase(uint32_t memoryPage);

/* (zostawiam, bo masz w pliku) */
uint32_t GetPage(uint32_t Addr);
