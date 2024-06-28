/********************************** (C) COPYRIGHT *******************************
* File Name          : Main.c
* Author             : WCH
* Version            : V1.0
* Date               : 2018/12/15
* Description 		 : ϵͳ˯��ģʽ��������ʾ��GPIOA_6��Ϊ����Դ����6��˯�ߵȼ�
*******************************************************************************/

#include "CH57x_common.h"

void DebugInit(void)		
{
    GPIOA_SetBits(GPIO_Pin_9);
    GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
}

int main()
{
/* ���ô��ڵ��� */   
    DebugInit();
    PRINT( "Start @ChipID=%02X\n", R8_CHIP_ID );
    DelsyMs(500);
    
#if 1    
    /* ���û���ԴΪ GPIO - PA6 */
    GPIOA_ModeCfg( GPIO_Pin_6, GPIO_ModeIN_PU );
    GPIOA_ITModeCfg( GPIO_Pin_6, GPIO_ITMode_FallEdge );        // �½��ػ���
    NVIC_EnableIRQ( GPIO_IRQn );
    PWR_PeriphWakeUpCfg( ENABLE, RB_SLP_GPIO_WAKE );
#endif    
    
#if 1
    PRINT( "IDLE mode sleep \n");   
    DelsyMs(1);
    LowPower_Idle();
    PRINT( "wake.. \n"); 
    DelsyMs(500);
#endif    
    
#if 1
    PRINT( "Halt_1 mode sleep \n");   
    DelsyMs(1);
    LowPower_Halt_1();
    PRINT( "wake.. \n"); 
    DelsyMs(500);    
#endif    
    
#if 1
    PRINT( "Halt_2 mode sleep \n");   
    DelsyMs(1);
    LowPower_Halt_2();
    PRINT( "wake.. \n"); 
    DelsyMs(500);    
#endif    

#if 1
    PRINT( "sleep mode sleep \n");   
    DelsyMs(1);
    LowPower_Sleep( RB_PWR_RAM14K|RB_PWR_RAM2K );       //ֻ����14+2K SRAM ����
    SetSysClock( CLK_SOURCE_HSI_32MHz );                // �˵͹���ģʽ���Ѻ���Ҫ�л���ԭʱ��
    PRINT( "wake.. \n");      
    DelsyMs(500);
#endif

#if 1
    PRINT( "shut down mode sleep \n");   
    DelsyMs(1);
    LowPower_Shutdown( NULL );                          //ȫ���ϵ磬���Ѻ�λ
    SetSysClock( CLK_SOURCE_HSI_32MHz );
    PRINT( "wake.. \n");
    DelsyMs(500);
#endif

    while(1);    
}


void GPIO_IRQHandler(void)
{
    GPIOA_ClearITFlagBit( GPIO_Pin_6 );
}




