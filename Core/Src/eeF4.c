#include "eeF4.h"
#include "stm32f4xx_hal.h"

/* =========================================================
 *  FLAGI FLASH / WAIT
 * ========================================================= */

void flash_clear_all_flags(void)
{
    /* F4: standardowe flagi + ewentualnie PGSERR/PGAERR itd. (zależnie od HAL) */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
}

HAL_StatusTypeDef wait_flash_ready(uint32_t WaitForLastOperation_ms, uint32_t overall_ms)
{
    uint32_t t0 = HAL_GetTick();
    HAL_StatusTypeDef st;

    do {
        st = FLASH_WaitForLastOperation(WaitForLastOperation_ms);
        if (st == HAL_OK)    return HAL_OK;
        if (st == HAL_ERROR) return HAL_ERROR;
        /* st == HAL_TIMEOUT -> próbuj do overall_ms */
    } while ((HAL_GetTick() - t0) < overall_ms);

    return HAL_TIMEOUT;
}

/* =========================================================
 *  ADRESOWANIE “2KB STRON” w obrębie sektora
 * ========================================================= */

uint32_t getHexAddressPage(int dataPage)
{
    uint32_t offs = (uint32_t)PAGE_SECTOR * (uint32_t)dataPage;
    return FLASH_INIT + offs;
}

uint32_t retrieveDataFromAddress(uint32_t hexAddress)
{
    return *(uint32_t*)hexAddress;
}

/* =========================================================
 *  MAPOWANIE ADRESU NA SEKTOR (F411)
 *  F411: S0..S3 = 16KB, S4 = 64KB, S5..S7 = 128KB (dla 512KB)
 * ========================================================= */

static uint32_t GetSector(uint32_t Address)
{
    /* Tabela dla układu 512KB:
       0x0800 0000 S0  16K
       0x0800 4000 S1  16K
       0x0800 8000 S2  16K
       0x0800 C000 S3  16K
       0x0801 0000 S4  64K
       0x0802 0000 S5 128K
       0x0804 0000 S6 128K
       0x0806 0000 S7 128K
    */
    if (Address < 0x08004000UL) return FLASH_SECTOR_0;
    if (Address < 0x08008000UL) return FLASH_SECTOR_1;
    if (Address < 0x0800C000UL) return FLASH_SECTOR_2;
    if (Address < 0x08010000UL) return FLASH_SECTOR_3;
    if (Address < 0x08020000UL) return FLASH_SECTOR_4;
    if (Address < 0x08040000UL) return FLASH_SECTOR_5;
    if (Address < 0x08060000UL) return FLASH_SECTOR_6;
    return FLASH_SECTOR_7;
}

/* Zostawiam Twoją funkcję – na F4 “page” nie ma sensu jak w G0,
   ale jeśli gdzieś używasz, to zwracam “numer 2KB strony w obrębie sektora bazowego”. */
uint32_t GetPage(uint32_t Addr)
{
    if (Addr < FLASH_INIT) return 0;
    return (Addr - FLASH_INIT) / PAGE_SECTOR;
}

/* =========================================================
 *  ERASE: na F4 kasujemy sektor, nie 2KB page
 * ========================================================= */

HAL_StatusTypeDef memoryPageErase(uint32_t memoryPage)
{
    (void)memoryPage;

    HAL_StatusTypeDef st = HAL_FLASH_Unlock();
    if (st != HAL_OK) return st;

    flash_clear_all_flags();

    st = wait_flash_ready(50, 1000);
    if (st != HAL_OK) {
        /* jeśli flash wciąż zajęty, nie rób Lock przy BSY */
        return st;
    }

    /* Kasujemy sektor, w którym jest nasza baza obszaru danych. */
    uint32_t sector = GetSector(FLASH_INIT);

    FLASH_EraseInitTypeDef erase = {0};
    uint32_t sector_error = 0;

    erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3; /* typowe dla 3.3V */
    erase.Sector       = sector;
    erase.NbSectors    = 1;

    __disable_irq();
    st = HAL_FLASHEx_Erase(&erase, &sector_error);
    __enable_irq();

    if (st != HAL_OK) {
        (void)wait_flash_ready(50, 1000);
        /* po błędzie lock ok */
        (void)HAL_FLASH_Lock();
        return st;
    }

    st = wait_flash_ready(50, 2000);
    if (st == HAL_OK) {
        (void)HAL_FLASH_Lock();
    }
    /* Jeśli timeout: nie lockuj na siłę */

    return st;
}

/* =========================================================
 *  WRITE: na F4 najbezpieczniej programować WORD (32-bit)
 *  Zachowuję Twoje odstępy 8 bajtów: +0, +8, +16
 * ========================================================= */

HAL_StatusTypeDef writeThreeData(uint32_t hexPage, int dataA, int dataB, int dataC)
{
    /* Zanim zapiszesz – kasujesz (u Ciebie: kasowanie “page”; tu: kasowanie sektora) */
    HAL_StatusTypeDef st = memoryPageErase(GetPage(hexPage));
    if (st != HAL_OK) return st;

    /* WORD na F4 wymaga wyrównania do 4 bajtów */
    if ((hexPage & 0x3u) != 0u) return HAL_ERROR;

    st = HAL_FLASH_Unlock();
    if (st != HAL_OK) return st;

    flash_clear_all_flags();
    (void)wait_flash_ready(1, 50);

    st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, hexPage,      (uint32_t)dataA);
    if (st != HAL_OK) { (void)HAL_FLASH_Lock(); return st; }
    (void)wait_flash_ready(1, 50);

    st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, hexPage + 8u,  (uint32_t)dataB);
    if (st != HAL_OK) { (void)HAL_FLASH_Lock(); return st; }
    (void)wait_flash_ready(1, 50);

    st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, hexPage + 16u, (uint32_t)dataC);
    (void)wait_flash_ready(1, 50);

    (void)HAL_FLASH_Lock();
    return st;
}
