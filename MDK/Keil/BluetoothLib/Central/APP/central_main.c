/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0
* Date               : 2018/11/12
* Description        : 
*******************************************************************************/



/******************************************************************************/
/* 头文件包含 */
#include "CONFIG.h"
#include "CH57x_common.h"
#include "CH57xBLE_LIB.H"
#include "hal.h"
#include "central.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
pTaskEventHandlerFn tasksArr[] = {
	TMOS_CbTimerProcessEvent,
  HAL_ProcessEvent,
  LL_ProcessEvent,
	L2CAP_ProcessEvent,
	GAP_ProcessEvent,	
  GATT_ProcessEvent,                                                
  SM_ProcessEvent,                                                 
  GAPBondMgr_ProcessEvent,                                         
  GATTServApp_ProcessEvent,                                        
  GAPRole_CentralProcessEvent,
  Central_ProcessEvent,                                 
};
uint8 TASK_CNT =  sizeof( tasksArr ) / sizeof( tasksArr[0] );

/*********************************************************************
 * @fn      TMOS_InitTasks
 *
 * @brief   This function invokes the initialization function for each task.
 *
 * @param   void
 *
 * @return  none
 */
void TMOS_InitTasks( void )
{
  UINT8 taskID = 0;
  
  /* tmos Task */
	TMOS_Init( taskID++ );
  Hal_Init( taskID++ );
  /* LL Task */
  LL_Init( taskID++ );
  /* L2CAP Task */
  L2CAP_Init( taskID++ );
  /* GAP Task */
  GAP_Init( taskID++ );
  /* GATT Task */
  GATT_Init( taskID++ );
  /* SM Task */
  SM_Init( taskID++ );
  GAPBondMgr_Init( taskID++ );
  GATTServApp_Init( taskID++ );
   /* Profiles */
  GAPRole_CentralInit( taskID++ );
  /* Application */
  Central_Init( taskID++ );
	if( taskID > TASK_CNT ){
		PRINT("sys event error.\n");	
	}	
}

/*******************************************************************************
* Function Name  : main
* Description    : 主函数
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
int main( void ) 
{
#ifdef DEBUG
  GPIOA_SetBits(GPIO_Pin_9);
  GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
  UART1_DefInit( );
#endif   
  PRINT("%s\n",VER_LIB);
  CH57X_BLEInit( );
	TMOS_InitTasks( );
	while(1){
		TMOS_SystemProcess( );
	}
}

/******************************** endfile @ main ******************************/
