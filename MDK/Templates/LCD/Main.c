/********************************** (C) COPYRIGHT *******************************
* File Name          : Main.c
* Author             : WCH
* Version            : V1.0
* Date               : 2018/12/15
* Description 		 : LCD��ʾ���������������0-7
*******************************************************************************/

#include "CH57x_common.h"


//�����ʾС����+0x80.
unsigned char const lcd[10]={0x7d,0x60,0x3e,0x7a,0x63,0x5b,0x5f,0x70,0x7f,0x9b};
/*     4
     |----|
    0|    |5
     |-1--|
    2|    |6
     |----| . 7
       3
*/
/* ע�⣺ʹ�ô����ӣ�����ʱ��ر��ⲿ�ֶ���λ���� */

int main()
{    
    LCD_WriteData0( lcd[0] );
    LCD_WriteData1( lcd[1] );
    LCD_WriteData2( lcd[2] );
    LCD_WriteData3( lcd[3] );
    LCD_WriteData4( lcd[4] );
    LCD_WriteData5( lcd[5] );
    LCD_WriteData6( lcd[6] );
    LCD_WriteData7( lcd[7] );  
    
    LCD_DefInit();
    
    while(1);    
}






