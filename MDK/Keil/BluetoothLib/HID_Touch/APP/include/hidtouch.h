/********************************** (C) COPYRIGHT *******************************
* File Name          : hidtouch.h
* Author             : WCH
* Version            : V1.0
* Date               : 2018/12/12
* Description        : 
*******************************************************************************/

#ifndef HIDTOUCH_H
#define HIDTOUCH_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */

// Task Events
#define START_DEVICE_EVT                              0x0001
#define START_REPORT_EVT                              0x0002

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * FUNCTIONS
 */
  
/*********************************************************************
 * GLOBAL VARIABLES
 */

/*
 * Task Initialization for the BLE Application
 */
extern void HidEmu_Init( uint8 task_id );

/*
 * Task Event Processor for the BLE Application
 */
extern uint16 HidEmu_ProcessEvent( uint8 task_id, uint16 events );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /*HIDEMUKBD_H */
