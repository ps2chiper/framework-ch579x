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
#define START_SVC_DISCOVERY_EVT                       0x0008
#define START_PARAM_UPDATE_EVT                        0x0010
#define START_READ_OR_WRITE_EVT                       0x0020

  /*********************************************************************
 * MACROS
 */
typedef enum
{
	F_GATT_Connect = 0,
	F_GATT_Disconnect,
	F_GAP_Authenticate,
	F_GATT_ExchangeMTU,
	F_GATT_DiscAllPrimaryServices,
	F_GATT_DiscPrimaryServiceByUUID,
	F_GATT_FindIncludedServices,
	F_GATT_DiscAllChars,
	F_GATT_DiscCharsByUUID,
	F_GATT_DiscAllCharDescs,
	F_GATT_ReadCharValue,
	F_GATT_ReadUsingCharUUID,
	F_GATT_ReadLongCharValue,
	F_GATT_ReadMultiCharValues,
	F_GATT_WriteNoRsp,
	F_GATT_SignedWriteNoRsp,
	F_GATT_WriteCharValue,
	F_GATT_WriteLongCharValue,
	F_GATT_ReliableWrites,
	F_GATT_ReadCharDesc,
	F_GATT_ReadLongCharDesc,
	F_GATT_WriteCharDesc,
	F_GATT_WriteLongCharDesc,
	F_GATT_MAX
}FuncName_t;
/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the BLE Application
 */
extern void Central_Init( uint8 task_id );

/*
 * Task Event Processor for the BLE Application
 */
extern uint16 Central_ProcessEvent( uint8 task_id, uint16 events );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* OBSERVER_H */
