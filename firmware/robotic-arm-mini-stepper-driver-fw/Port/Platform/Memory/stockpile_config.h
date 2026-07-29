/******
	************************************************************************
	******
	** @versions : 1.1.4
	** @time     : 2020/09/15
	******
	************************************************************************
	******
	** @project : XDrive_Step
	** @brief   : Stepper motor with multi-function interface and closed-loop function
	** @author  : unlir (ZhiBuZhiA)
	******
	** @address : https://github.com/unlir/XDrive
	******
	** @issuer  : IVES (IVES Laboratory) (QQ: 557214000)   (master)
	** @issuer  : REIN (ZhiYu Laboratory) (QQ: 857046846)   (master)
	******
	************************************************************************
	******
	** {Stepper motor with multi-function interface and closed Main function.}
	** Copyright (c) {2020}  {unlir(ZhiBuZhiA)}
	** 
	** This program is free software: you can redistribute it and/or modify
	** it under the terms of the GNU General Public License as published by
	** the Free Software Foundation, either version 3 of the License, or
	** (at your option) any later version.
	** 
	** This program is distributed in the hope that it will be useful,
	** but WITHOUT ANY WARRANTY; without even the implied warranty of
	** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	** GNU General Public License for more details.
	** 
	** You should have received a copy of the GNU General Public License
	** along with this program.  If not, see <http://www.gnu.org/licenses/>.
	******
	************************************************************************
******/

/*****
  ** @file     : stockpile_config.c/h
  ** @brief    : Storage configuration
  ** @versions : newest
  ** @time     : newest
  ** @reviser  : unli (HeFei China)
  ** @explain  : null
*****/

/*************************************************************** Stockpile_Start ***************************************************************/
/*************************************************************** Stockpile_Start ***************************************************************/
/*************************************************************** Stockpile_Start ***************************************************************/
/*********************STM32F103xx*************************/
//Main storage block capacity
//Flash Size(bytes)/RAM size(bytes)
// High capacity 1M / 96K                                     RG               VG           ZG
// High capacity 768K / 96K                                     RF               VF           ZF
// High capacity 512K / 64K                                     RE               VE           ZE
// High capacity 384K / 64K                                     RD               VD           ZD
// High capacity 256K / 48K                                     RC               VC           ZC
// Medium capacity 128K / 20K      TB           CB                RB               VB
// Medium capacity  64K / 20K      T8           C8                R8               V8
// Low capacity  32K / 10K      T6           C6                R6
// Low capacity  16K /  6K      T4           C4                R4
//        						 36pin-QFN	48pin-LQFP/QFN	64pin-BGA/CSP/LQFP  100pin-LQFP  144pin-BGA/LQFP  
/*************************************************************** Stockpile_End ***************************************************************/
/*************************************************************** Stockpile_End ***************************************************************/
/*************************************************************** Stockpile_End ***************************************************************/

#ifndef STOCKPILE_CONFIG_H
#define STOCKPILE_CONFIG_H

/* ROM sizes */
/* ROM sizes */
/* ROM sizes */

//DAPLINK_ROM_BL
#define		DAPLINK_ROM_BL_START						(0x08000000)		//Start address
#define		DAPLINK_ROM_BL_SIZE							(0x0000BC00)		//Flash capacity    47K		DAPLink_BL(DAPLINK_ROM_BL)
//DAPLINK_ROM_CONFIG_ADMIN
#define		DAPLINK_ROM_CONFIG_ADMIN_START	(0x0800BC00)		//Start address
#define		DAPLINK_ROM_CONFIG_ADMIN_SIZE		(0x00000400)		//Flash capacity     1K		DAPLink_BL(DAPLINK_ROM_CONFIG_ADMIN)
//APP_FIRMWARE
#define		STOCKPILE_APP_FIRMWARE_ADDR			(0x08000000) //(0x0800C000)		//Start address
#define		STOCKPILE_APP_FIRMWARE_SIZE			(0x0000BC00)		//Flash capacity    47K    XDrive(APP_FIRMWARE)
//APP_CALI
#define		STOCKPILE_APP_CALI_ADDR					(0x08017C00)		//Start address
#define		STOCKPILE_APP_CALI_SIZE					(0x00008000)		//Flash capacity    32K    XDrive(APP_CALI)(can hold 16K-2byte calibration data - max support for 14-bit encoder calibration data)
//APP_DATA
#define		STOCKPILE_APP_DATA_ADDR					(0x0801FC00)		//Start address
#define		STOCKPILE_APP_DATA_SIZE					(0x00000400)		//Flash capacity     1K    XDrive(APP_DATA)

/* RAM sizes */
/* RAM sizes */
/* RAM sizes */

#define STOCKPILE_RAM_APP_START           (0x20000000)
#define STOCKPILE_RAM_APP_SIZE            (0x00004F00)		//19K768 bytes

#define STOCKPILE_RAM_SHARED_START        (0x20004F00)
#define STOCKPILE_RAM_SHARED_SIZE         (0x00000100)		//256 bytes

#endif
