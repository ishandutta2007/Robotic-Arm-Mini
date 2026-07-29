#ifndef STOCKPILE_F103CB_H
#define STOCKPILE_F103CB_H

#ifdef __cplusplus
extern "C" {
#endif

// Include hardware ports definitions
#include "main.h"
#include "gpio.h"
#include "spi.h"
#include "tim.h"
// Include application storage configuration
#include "stockpile_config.h"

/*************************************************************** FLASH_Start ***************************************************************/
/*************************************************************** FLASH_Start ***************************************************************/
/*************************************************************** FLASH_Start ***************************************************************/
/****************** Page configuration (Must be modified when changing chips) ***********************/
#define Stockpile_Page_Size		0x400U		// Sector size (default 1024 bytes)
#if (Stockpile_Page_Size != FLASH_PAGE_SIZE)	// Compare with FLASH page size obtained from HAL library to check if config is valid
	#error "Stockpile_Page_Size Error !!!"		
#endif

/**
* Flash partition table structure
**/
typedef struct{
	// Configuration
	uint32_t	begin_add;			// Start address
	uint32_t	area_size;			// Area size
	uint32_t	page_num;				// Number of physical pages on the chip
	// Process variable
	uint32_t	asce_write_add;	// Write address
}Stockpile_FLASH_Typedef;

/********** Flash partition table instances **********/
extern Stockpile_FLASH_Typedef stockpile_app_firmware;
extern Stockpile_FLASH_Typedef stockpile_quick_cali;
extern Stockpile_FLASH_Typedef stockpile_data;

void Stockpile_Flash_Data_Empty(Stockpile_FLASH_Typedef *stockpile);			// Clear Flash data
void Stockpile_Flash_Data_Begin(Stockpile_FLASH_Typedef *stockpile);			// Begin Flash data write
void Stockpile_Flash_Data_End(Stockpile_FLASH_Typedef *stockpile);				// End Flash data write
void Stockpile_Flash_Data_Set_Write_Add(Stockpile_FLASH_Typedef *stockpile, uint32_t write_add);					// Set Flash write address
void Stockpile_Flash_Data_Write_Data16(Stockpile_FLASH_Typedef *stockpile, uint16_t *data, uint32_t num);	// Write 16-bit Flash data
void Stockpile_Flash_Data_Write_Data32(Stockpile_FLASH_Typedef *stockpile, uint32_t *data, uint32_t num);	// Write 32-bit Flash data
void Stockpile_Flash_Data_Write_Data64(Stockpile_FLASH_Typedef *stockpile, uint64_t *data, uint32_t num);	// Write 64-bit Flash data

/*************************************************************** FLASH_End ***************************************************************/
/*************************************************************** FLASH_End ***************************************************************/
/*************************************************************** FLASH_End ***************************************************************/

#ifdef __cplusplus
}
#endif

#endif
