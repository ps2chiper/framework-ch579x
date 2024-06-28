/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0
* Date               : 2018/11/12
* Description        : ����ӻ�Ӧ��������������ϵͳ��ʼ��
*******************************************************************************/




/******************************************************************************/
/* ͷ�ļ����� */
#include "CONFIG.h"
#include "CH57x_common.h"
#include "CH57xBLE_LIB.h"
#include "HAL.h"
#include "GATTprofile.h"
#include "Peripheral.h"
#include "OTA.h"

/* ��¼��ǰ��Image */
unsigned char CurrImageFlag = 0xff;

__align(8) unsigned char flash_buf[FLASH_BLOCK_SIZE];

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

    GAPRole_PeripheralProcessEvent,                                            
    SimpleBLEPeripheral_ProcessEvent,                                 
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
    
    /* Hal Task */
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
    GAPRole_PeripheralInit( taskID++ );
    /* Application */
    SimpleBLEPeripheral_Init( taskID++ );
}

/* ע�⣺���ڳ���������flash�Ĳ���������ִ�У��������κ��жϣ���ֹ�����жϺ�ʧ�� */
/*******************************************************************************
* Function Name  : ReadImageFlag
* Description    : ��ȡ��ǰ�ĳ����Image��־��DataFlash���Ϊ�գ���Ĭ����ImageA
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void ReadImageFlag(void)
{
    UINT8 *p_image_flash;
    
    p_image_flash = (UINT8 *)OTA_DATAFLASH_ADD;
    CurrImageFlag = *p_image_flash;
    
    /* �����һ��ִ�У�����û�и��¹����Ժ���º��ڲ���DataFlash */
    if( CurrImageFlag != IMAGE_A_FLAG && CurrImageFlag != IMAGE_B_FLAG )
    {
        CurrImageFlag = IMAGE_A_FLAG;
    }
    
    PRINT("Image Flag %02x\n",CurrImageFlag);
}

/*******************************************************************************
* Function Name  : ImageVectorsCopy
* Description    : ִ���ж������İ��ƣ�����ImageA����ImageBʱִ��
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void ImageVectorsCopy(void)
{
	unsigned int vectors_entry;
	
	/* ��ȡ��ǰ�ĳ������ڵ�ַ */
	vectors_entry = *(unsigned int *)IMAGE_A_ENTRY_ADD;
	
    /* ImageA->ImageB����ǰ����ImageB�����������ImageA����Ҫ���� */
    if( (CurrImageFlag == IMAGE_B_FLAG) && (vectors_entry < IMAGE_B_START_ADD) )
    {
		unsigned int i;
		unsigned char *p_flash;
		
        /* ��ȡImageB�Ĵ����һ�� */
        p_flash = (UINT8 *)IMAGE_B_START_ADD;
        for(i=0; i<FLASH_BLOCK_SIZE; i++) flash_buf[i] = p_flash[i];
		
        /* ����ImageA����ĵ�һ�� */
        CodeFlash_BlockEarse(IMAGE_A_START_ADD);
		
        /* ��ImageB�ж���������ImageA�ж����� */
        CodeFlash_WriteBuf(IMAGE_A_START_ADD,(PUINT32)&flash_buf[0],FLASH_BLOCK_SIZE);
		
		PRINT("ImageB vectors entry copy complete %08x \n",vectors_entry);
    }
}

/*******************************************************************************
* Function Name  : main
* Description    : ������
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
    ReadImageFlag();
	ImageVectorsCopy();
    CH57X_BLEInit( );
    TMOS_InitTasks( );
    while(1){
        TMOS_SystemProcess( );
    }
}

/******************************** endfile @ main ******************************/
