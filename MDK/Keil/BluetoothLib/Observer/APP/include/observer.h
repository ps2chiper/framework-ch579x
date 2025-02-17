/********************************** (C) COPYRIGHT *******************************
* File Name          : observer.h
* Author             : WCH
* Version            : V1.0
* Date               : 2018/11/12
* Description        : 观察应用主函数及任务系统初始化
*******************************************************************************/

#ifndef OBSERVER_H
#define OBSERVER_H

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


// Simple BLE Observer Task Events
#define START_DEVICE_EVT                              0x0001
#define START_DISCOVERY_EVT                           0x0002
#define START_SCAN_EVT                                0x0004

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the BLE Application
 */
extern void Observer_Init( uint8 task_id );

/*
 * Task Event Processor for the BLE Application
 */
extern uint16 Observer_ProcessEvent( uint8 task_id, uint16 events );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* OBSERVER_H */
