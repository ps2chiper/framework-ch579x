/**************************************************************************//**
 * @file		FlashPrg.c
 * @brief	 Flash Programming Functions adapted for New Device Flash
 * @version	V1.0.0
 * @date		10. January 2018
 ******************************************************************************/
/*
 * Copyright (c) 2010-2018 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#include "..\FlashOS.H"			// FlashOS Structures

/* 
	Mandatory Flash Programming Functions (Called by FlashOS):
					 int Init			(unsigned long adr,	// Initialize Flash
											unsigned long clk,
											unsigned long fnc);
					 int UnInit		(unsigned long fnc);	// De-initialize Flash
					 int EraseSector (unsigned long adr);	// Erase Sector Function
					 int ProgramPage (unsigned long adr,	// Program Page Function
											unsigned long sz,
											unsigned char *buf);

	Optional	Flash Programming Functions (Called by FlashOS):
					 int BlankCheck	(unsigned long adr,	// Blank Check
											unsigned long sz,
											unsigned char pat);
					 int EraseChip	(void);					// Erase complete Device
		unsigned long Verify		(unsigned long adr,	// Verify Function
											unsigned long sz,
											unsigned char *buf);

		 - BlanckCheck	is necessary if Flash space is not mapped into CPU memory space
		 - Verify		 is necessary if Flash space is not mapped into CPU memory space
		 - if EraseChip is not provided than EraseSector for all sectors is called
*/


#ifndef UINT32
typedef unsigned long           UINT32;
#endif

#ifndef PUINT32
typedef unsigned long           *PUINT32;
#endif

#ifndef PUINT8V
typedef volatile unsigned char  *PUINT8V;
#endif

#ifndef PUINT32V
typedef volatile unsigned long  *PUINT32V;
#endif


/* System: Flash ROM control register */
#define R32_FLASH_DATA			(*((PUINT32V)0x40001800)) 	// RW, Flash ROM data
#define R32_FLASH_ADDR			(*((PUINT32V)0x40001804)) 	// RW, Flash ROM address
#define R8_FLASH_COMMAND		(*((PUINT8V)0x40001808))	// WO, Flash ROM operation command
#define ROM_CMD_PROG				0x9A					// WO: Flash ROM word program operation command, for changing some ROM bit of a word from 1 to 0
#define ROM_CMD_ERASE				0xA6					// WO: Flash ROM sector erase operation command, for changing all ROM bit of 512Bytes from 0 to 1
#define R8_FLASH_PROTECT		(*((PUINT8V)0x40001809))	// RW, Flash ROM protect control
#define	RB_ROM_DATA_WE				0x04						// RW, enable Flash ROM data area being erase/write: 0=writing protect, 1=enable program and erase
#define	RB_ROM_CODE_WE				0x08						// RW, enable Flash ROM code area being erase/write: 0=writing protect, 1=enable program and erase
#define	RB_ROM_WE_MUST_0			0x40						// RW, must write 0
#define	RB_ROM_WE_MUST_1			0x80						// RW, must write 1
#define R16_FLASH_STATUS		(*((PUINT8V)0x4000180A))	// RO, Flash ROM operation status
#define	RB_ROM_CMD_TOUT				0x01						// RO, Flash ROM operation result: 0=success, 1=operation time out
#define	RB_ROM_CMD_ERR				0x02						// RO, Flash ROM operation command error flag: 0=command accepted, 1=unknown command
#define	RB_ROM_ADDR_OK				0x40						// RO, Flash ROM erase/write operation address valid flag, can be reviewed before or after operation: 0=invalid parameter, 1=address valid
#define	RB_ROM_READ_FREE			0x100						// RO, indicate protected status of Flash ROM code and data: 0=reading protect, 1=enable read by external programmer





/*
 *	Initialize Flash Programming Functions
 *	 Parameter:		adr:	Device Base Address
 *							clk:	Clock Frequency (Hz)
 *							fnc:	Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *	 Return Value:	0 - OK,	1 - Failed
 */

int Init (unsigned long adr, unsigned long clk, unsigned long fnc) {
	
	R8_FLASH_PROTECT = 0x8c;	//配置CodeFlash和DataFlash 可擦写
	
	/* Add your Code */
	return (0);											 // Finished without Errors
}


/*
 *	De-Initialize Flash Programming Functions
 *	 Parameter:		fnc:	Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *	 Return Value:	0 - OK,	1 - Failed
 */

int UnInit (unsigned long fnc) {
	
	R8_FLASH_PROTECT = 0x80;	//配置CodeFlash和DataFlash 不可擦写
	
	/* Add your Code */
	return (0);											 // Finished without Errors
}


/*
 *	Erase complete Flash Memory
 *	 Return Value:	0 - OK,	1 - Failed
 */

int EraseChip (void) {
	
	UINT32 addr;
//	R8_FLASH_PROTECT = 0x8c;
	for (addr=0;addr<0x3F000;addr+=512) {
		R32_FLASH_ADDR = addr;
		R8_FLASH_COMMAND = ROM_CMD_ERASE;		//编程
		if ( R16_FLASH_STATUS&0x01 ) return (1);
	}
//	R8_FLASH_PROTECT = 0x80;
	
	/* Add your Code */
	return (0);											 // Finished without Errors
}


/*
 *	Erase Sector in Flash Memory
 *	 Parameter:		adr:	Sector Address
 *	 Return Value:	0 - OK,	1 - Failed
 */

int EraseSector (unsigned long adr) {
	
//	R8_FLASH_PROTECT = 0x8c;
	R32_FLASH_ADDR = adr;
	R8_FLASH_COMMAND = ROM_CMD_ERASE;		//编程
//	R8_FLASH_PROTECT = 0x80;
	if ( R16_FLASH_STATUS&0x01 ) return (1);
	
	/* Add your Code */
	return (0);											 // Finished without Errors
}


/*
 *	Program Page in Flash Memory
 *	 Parameter:		adr:	Page Start Address
 *							sz:	Page Size
 *							buf:	Page Data
 *	 Return Value:	0 - OK,	1 - Failed
 */

int ProgramPage (unsigned long adr, unsigned long sz, unsigned char *buf) {

	UINT32 i;
	PUINT32 pbuf = (PUINT32)buf;
	
//	R8_FLASH_PROTECT = 0x8c;
	for(i=0; i!=sz; i+=4)
	{
		R32_FLASH_ADDR = adr;
		R32_FLASH_DATA = *pbuf++;		
		R8_FLASH_COMMAND = ROM_CMD_PROG;		//编程
		adr += 4;
		if( R16_FLASH_STATUS&0x01 )	return (1);
	}
//	R8_FLASH_PROTECT = 0x80;
	
	/* Add your Code */
	return (0);											 // Finished without Errors
}
