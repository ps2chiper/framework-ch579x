/********************************** (C) COPYRIGHT *******************************
* File Name          : Main.c
* Author             : WCH
* Version            : V1.0
* Date               : 2019/4/28
* Description 		 : ��ʱ������
*******************************************************************************/

#include "CH57x_common.h"



int main()
{     

#if 1       /* �趨100ms��ʱ������IO�����ƣ� PB3-LED */
    
    GPIOB_SetBits( GPIO_Pin_3 );
    GPIOB_ModeCfg( GPIO_Pin_3, GPIO_ModeOut_PP_5mA );
    
    TMR0_TimerInit( FREQ_SYS/10 );                  // ���ö�ʱʱ�� 100ms
    TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END);          // �����ж�
    NVIC_EnableIRQ( TMR0_IRQn );
    
#endif    
    
    while(1);    
}




void TMR0_IRQHandler( void )        // TMR0 ��ʱ�ж�
{
    if( TMR0_GetITFlag( TMR0_3_IT_CYC_END ) )
    {
        TMR0_ClearITFlag( TMR0_3_IT_CYC_END );      // ����жϱ�־
        GPIOB_InverseBits( GPIO_Pin_3 );        
    }
}



