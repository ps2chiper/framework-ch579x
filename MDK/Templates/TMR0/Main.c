/********************************** (C) COPYRIGHT *******************************
* File Name          : Main.c
* Author             : WCH
* Version            : V1.0
* Date               : 2019/4/28
* Description 		 : 定时器例子
*******************************************************************************/

#include "CH57x_common.h"



int main()
{     

#if 1       /* 设定100ms定时器进行IO口闪灯， PB3-LED */
    
    GPIOB_SetBits( GPIO_Pin_3 );
    GPIOB_ModeCfg( GPIO_Pin_3, GPIO_ModeOut_PP_5mA );
    
    TMR0_TimerInit( FREQ_SYS/10 );                  // 设置定时时间 100ms
    TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END);          // 开启中断
    NVIC_EnableIRQ( TMR0_IRQn );
    
#endif    
    
    while(1);    
}




void TMR0_IRQHandler( void )        // TMR0 定时中断
{
    if( TMR0_GetITFlag( TMR0_3_IT_CYC_END ) )
    {
        TMR0_ClearITFlag( TMR0_3_IT_CYC_END );      // 清除中断标志
        GPIOB_InverseBits( GPIO_Pin_3 );        
    }
}



