/********************************** (C) COPYRIGHT ******************************
* File Name          : DNS_Client.c
* Author             : WCH
* Version            : V1.0
* Date               : 2018/12/01
* Description        : CH57xNET库演示文件
*                      (1)、CH579 Examples by KEIL;
*                      (2)、串口0输出监控信息,115200bps;
*                      (3)、本程序用于演示UDP 客户端通讯，单片机收到数据后，回传给目标IP上位机
*******************************************************************************/



/******************************************************************************/
/* 头文件包含*/
#include <stdio.h>
#include <string.h>
#include "CH57x_common.h"
#include "core_cm0.h"
#include "CH57xNET.H"

#define KEEPLIVE_ENABLE                      1                                  /* 开启KEEPLIVE功能 */

/* 下面的缓冲区和全局变量必须要定义，库中调用 */
__align(16)UINT8    CH57xMACRxDesBuf[(RX_QUEUE_ENTRIES )*16];                   /* MAC接收描述符缓冲区，16字节对齐 */
__align(4) UINT8    CH57xMACRxBuf[RX_QUEUE_ENTRIES*RX_BUF_SIZE];                /* MAC接收缓冲区，4字节对齐 */
__align(4) SOCK_INF SocketInf[CH57xNET_MAX_SOCKET_NUM];                         /* Socket信息表，4字节对齐 */

const UINT16 MemNum[8] = {CH57xNET_NUM_IPRAW,
                         CH57xNET_NUM_UDP,
                         CH57xNET_NUM_TCP,
                         CH57xNET_NUM_TCP_LISTEN,
                         CH57xNET_NUM_TCP_SEG,
                         CH57xNET_NUM_IP_REASSDATA,
                         CH57xNET_NUM_PBUF,
                         CH57xNET_NUM_POOL_BUF
                         };
const UINT16 MemSize[8] = {CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_IPRAW_PCB),
                          CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_UDP_PCB),
                          CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_TCP_PCB),
                          CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_TCP_PCB_LISTEN),
                          CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_TCP_SEG),
                          CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_IP_REASSDATA),
                          CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_PBUF) + CH57xNET_MEM_ALIGN_SIZE(0),
                          CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_PBUF) + CH57xNET_MEM_ALIGN_SIZE(CH57xNET_SIZE_POOL_BUF)
                         };
__align(4)UINT8 Memp_Memory[CH57xNET_MEMP_SIZE];
__align(4)UINT8 Mem_Heap_Memory[CH57xNET_RAM_HEAP_SIZE];
__align(4)UINT8 Mem_ArpTable[CH57xNET_RAM_ARP_TABLE_SIZE];
/******************************************************************************/
/* 本演示程序的相关宏 */
#define RECE_BUF_LEN                          536                               /* 接收缓冲区的大小 */
/* CH57xNET库TCP的MSS长度为536字节，即一个TCP包里的数据部分最长为536字节 */
/* TCP协议栈采用滑动窗口进行流控，窗口最大值为socket的接收缓冲区长度。在设定 */
/* RX_QUEUE_ENTRIES时要考虑MSS和窗口之间的关系，例如窗口值为4*MSS，则远端一次会发送 */
/* 4个TCP包，如果RX_QUEUE_ENTRIES小于4，则必然会导致数据包丢失，从而导致通讯效率降低 */
/* 建议RX_QUEUE_ENTRIES要大于( 窗口/MSS ),如果多个socket同时进行大批量发送数据，则 */ 
/* 建议RX_QUEUE_ENTRIES要大于(( 窗口/MSS )*socket个数) 在多个socket同时进行大批数据收发时 */
/* 为了节约RAM，请将接收缓冲区的长度设置为MSS */

/* CH579相关定义 */
UINT8 MACAddr[6] = {0x84,0xc2,0xe4,0x02,0x03,0x04};                             /* CH579MAC地址 */
UINT8 IPAddr[4] = {192,168,1,200};                                              /* CH579IP地址 */
UINT8 GWIPAddr[4] = {192,168,1,1};                                              /* CH579网关 */
UINT8 IPMask[4] = {255,255,255,0};                                              /* CH579子网掩码 */

UINT8 SocketId;                                                                 /* 保存socket索引，可以不用定义 */
UINT8 SocketRecvBuf[RECE_BUF_LEN];                                              /* socket接收缓冲区 */
UINT8 MyBuf[RECE_BUF_LEN];                                                      /* 定义一个临时缓冲区 */

extern UINT8 dns_buf[512];

UINT8 status;
#define   MAX_URL_SIZE 128

UINT8  url_dn1[MAX_URL_SIZE] = "www.baidu.com";        
UINT8  url_dn2[MAX_URL_SIZE] = "www.tencent.com";        
UINT8  url_dn3[MAX_URL_SIZE] = "www.wch.cn";        


//extern const UINT16 *memp_num;

/*******************************************************************************
* Function Name  : IRQ_Handler
* Description    : IRQ中断服务函数
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void ETH_IRQHandler( void )							/* 以太网中断 */
{
	CH57xNET_ETHIsr();								/* 以太网中断中断服务函数 */
}

void TMR0_IRQHandler( void ) 						/* 定时器中断 */
{
	CH57xNET_TimeIsr(CH57xNETTIMEPERIOD);           /* 定时器中断服务函数 */
	R8_TMR0_INT_FLAG |= 0xff;						/* 清除定时器中断标志 */
}



/*******************************************************************************
* Function Name  : mStopIfError
* Description    : 调试使用，显示错误代码
* Input          : iError 错误代码
* Output         : None
* Return         : None
*******************************************************************************/
void mStopIfError(UINT8 iError)
{
    if (iError == CH57xNET_ERR_SUCCESS) return;                                 /* 操作成功 */
    PRINT("mStopIfError: %02X\r\n", (UINT16)iError);                           /* 显示错误 */
}

/*******************************************************************************
* Function Name  : net_initkeeplive
* Description    : keeplive初始化
* Input          : None      
* Output         : None
* Return         : None
*******************************************************************************/
#ifdef  KEEPLIVE_ENABLE
void net_initkeeplive(void)
{
    struct _KEEP_CFG  klcfg;

    klcfg.KLIdle = 20000;                                                       /* 空闲 */
    klcfg.KLIntvl = 10000;                                                      /* 间隔 */
    klcfg.KLCount = 5;                                                          /* 次数 */
    CH57xNET_ConfigKeepLive(&klcfg);
}
#endif

/*******************************************************************************
* Function Name  : CH57xNET_LibInit
* Description    : 库初始化操作
* Input          : ip      ip地址指针
*                ：gwip    网关ip地址指针
*                : mask    掩码指针
*                : macaddr MAC地址指针 
* Output         : None
* Return         : 执行状态
*******************************************************************************/
UINT8 CH57xNET_LibInit(/*const*/ UINT8 *ip,/*const*/ UINT8 *gwip,/*const*/ UINT8 *mask,/*const*/ UINT8 *macaddr)
{
    UINT8 i;
    struct _CH57x_CFG cfg;

    if(CH57xNET_GetVer() != CH57xNET_LIB_VER)return 0xfc;                       /* 获取库的版本号，检查是否和头文件一致 */
    CH57xNETConfig = LIB_CFG_VALUE;                                             /* 将配置信息传递给库的配置变量 */
    cfg.RxBufSize = RX_BUF_SIZE; 
    cfg.TCPMss   = CH57xNET_TCP_MSS;
    cfg.HeapSize = CH57x_MEM_HEAP_SIZE;
    cfg.ARPTableNum = CH57xNET_NUM_ARP_TABLE;
    cfg.MiscConfig0 = CH57xNET_MISC_CONFIG0;
    CH57xNET_ConfigLIB(&cfg);
    i = CH57xNET_Init(ip,gwip,mask,macaddr);
#ifdef  KEEPLIVE_ENABLE
    net_initkeeplive( );
#endif
    return (i);                      /* 库初始化 */
}

/*******************************************************************************
* Function Name  : CH57xNET_HandleSockInt
* Description    : Socket中断处理函数
* Input          : sockeid  socket索引
*                ：initstat 中断状态
* Output         : None
* Return         : None
*******************************************************************************/
void CH57xNET_HandleSockInt(UINT8 sockeid,UINT8 initstat)
{
    UINT32 len;
    UINT8 i;

    if(initstat & SINT_STAT_RECV)                                               /* 接收中断 */
    {
        len = CH57xNET_SocketRecvLen(sockeid,NULL);                             /* 会将当前接收指针传递给precv*/
        PRINT("Receive Len = %02x\n",len);                           
            if (len > 0)
            {    
                if((sockeid==0)&&(status ==3))
                {
			    PRINT("enter rece int!STATUS=3\r\n");
                CH57xNET_SocketRecv(sockeid,dns_buf,&len);
                status = 4;
                i = CH57xNET_SocketClose( sockeid,TCP_CLOSE_NORMAL );
                mStopIfError(i);
                PRINT("status = 4!\n");
                }
                else
                  CH57xNET_SocketRecv(sockeid,MyBuf,&len);                      /* 将接收缓冲区的数据读到MyBuf中*/
            }
    }
    if(initstat & SINT_STAT_CONNECT)                                            /* TCP连接中断 */
    {                                                               /* 产生此中断表示TCP已经连接，可以进行收发数据 */
        PRINT("TCP Connect Success\n");                           
    }
    if(initstat & SINT_STAT_DISCONNECT)                                         /* TCP断开中断 */
    {                                                              /* 产生此中断，CH579库内部会将此socket清除，置为关闭*/
        PRINT("TCP Disconnect\n");                                             /* 应用曾需可以重新创建连接 */

    }
    if(initstat & SINT_STAT_TIM_OUT)                                            /* TCP超时中断 */
    {                                                              /* 产生此中断，CH579库内部会将此socket清除，置为关闭*/
        PRINT("TCP Timout\n");                                                 /* 应用曾需可以重新创建连接 */

    }
}


/*******************************************************************************
* Function Name  : CH57xNET_HandleGloableInt
* Description    : 全局中断处理函数
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void CH57xNET_HandleGlobalInt(void)
{
    UINT8 initstat;
    UINT8 i;
    UINT8 socketinit;
    initstat = CH57xNET_GetGlobalInt();                                         /* 读全局中断状态并清除 */
    if(initstat & GINT_STAT_UNREACH)                                            /* 不可达中断 */
    {
        PRINT("UnreachCode ：%d\n",CH57xInf.UnreachCode);                      /* 查看不可达代码 */
        PRINT("UnreachProto ：%d\n",CH57xInf.UnreachProto);                    /* 查看不可达协议类型 */
        PRINT("UnreachPort ：%d\n",CH57xInf.UnreachPort);                      /* 查询不可达端口 */     
    }
   if(initstat & GINT_STAT_IP_CONFLI)                                           /* IP冲突中断 */
   {
   
   }
   if(initstat & GINT_STAT_PHY_CHANGE)                                          /* PHY改变中断 */
   {
       i = CH57xNET_GetPHYStatus();                                             /* 获取PHY状态 */
       if(i>=0x02) status = 1;
       PRINT("status = 1!\n");
       PRINT("GINT_STAT_PHY_CHANGE %02x\n",i);
   }
   if(initstat & GINT_STAT_SOCKET)                                              /* Socket中断 */
   {
       for(i = 0; i < CH57xNET_MAX_SOCKET_NUM; i ++)                     
       {
           socketinit = CH57xNET_GetSocketInt(i);                               /* 读socket中断并清零 */
           if(socketinit)CH57xNET_HandleSockInt(i,socketinit);                  /* 如果有中断则清零 */
       }    
   }
}

/*******************************************************************************
* Function Name  : UDPSocketParamInit
* Description    : 创建UDP socket
* Input          : socket索引值，源端口，目的端口及目的IP
* Output         : None
* Return         : None
*******************************************************************************/
void UDPSocketParamInit(UINT8 S,UINT8 *addr,UINT16 SourPort,UINT16 DesPort )
{
   UINT8 i;                                                             
   SOCK_INF TmpSocketInf;                                                       /* 创建临时socket变量 */

   memset((void *)&TmpSocketInf,0,sizeof(SOCK_INF));                            /* 库内部会将此变量复制，所以最好将临时变量先全部清零 */
   memcpy((void *)TmpSocketInf.IPAddr,addr,4);                                  /* 设置目的IP地址 */
 
   TmpSocketInf.DesPort = DesPort;                                              /* 设置目的端口 */
   TmpSocketInf.SourPort = SourPort;                                            /* 设置源端口 */
   TmpSocketInf.ProtoType = PROTO_TYPE_UDP;                                     /* 设置socekt类型 */
   TmpSocketInf.RecvStartPoint = (UINT32)SocketRecvBuf;                         /* 设置接收缓冲区的接收缓冲区 */
   TmpSocketInf.RecvBufLen = RECE_BUF_LEN ;                                     /* 设置接收缓冲区的接收长度 */
   i = CH57xNET_SocketCreat(&S,&TmpSocketInf);                           /* 创建socket，将返回的socket索引保存在SocketId中 */
   mStopIfError(i);                                                             /* 检查错误 */
}


/*******************************************************************************
* Function Name  :  Timer0Init
* Description    : 定时器1初始化
* Input          : time 定时时间
* Output         : None
* Return         : None
*******************************************************************************/
void Timer0Init(UINT32 time)
{
	R8_TMR0_CTRL_MOD = RB_TMR_ALL_CLEAR;		                               /* 清除所有计数值 */
	R8_TMR0_CTRL_MOD = 0;						                               /* 设置定时器模式 */
	R32_TMR0_CNT_END = FREQ_SYS/1000000*time;	                               /* 设置定时时间 */
	R8_TMR0_INT_FLAG = R8_TMR0_INT_FLAG;		                               /* 清除标志 */
	R8_TMR0_INTER_EN = RB_TMR_IE_CYC_END;	                                   /* 定时中断 */
	R8_TMR0_CTRL_MOD |= RB_TMR_COUNT_EN;
	NVIC_EnableIRQ(TMR0_IRQn);	
}

/*******************************************************************************
* Function Name  : SystemClock_UART1_init
* Description    : 系统时钟和串口1初始化，串口1用于打印输出
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SystemClock_UART1_init(void)
{
    PWR_UnitModCfg(ENABLE, UNIT_SYS_PLL);                                      /* PLL上电 */
    DelsyMs(3); 
    SetSysClock(CLK_SOURCE_HSE_32MHz);                                          /* 外部晶振 PLL 输出32MHz */
    GPIOA_SetBits( GPIO_Pin_9 );
    GPIOA_ModeCfg( GPIO_Pin_9, GPIO_ModeOut_PP_5mA );                           /* 串口1的IO口设置 */
	UART1_DefInit( );                                                            /* 串口1初始化 */
}

/*******************************************************************************
* Function Name  : main
* Description    : 主函数
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
int main(void) 
{
    UINT8 ip[4],i = 0;
    UINT8 dns_flag = 0;
		
	SystemClock_UART1_init();	
    i = CH57xNET_LibInit(IPAddr,GWIPAddr,IPMask,MACAddr);                       /* 库初始化 */
    mStopIfError(i);                                                            /* 检查错误 */
    PRINT("CH57xNETLibInit Success\r\n");  
	Timer0Init( 10000 );		                                                /*初始化定时器:10ms*/
	NVIC_EnableIRQ(ETH_IRQn);
	while ( CH57xInf.PHYStat < 2 ) {
		DelsyMs(50);
	}
    PRINT("CH579 DNS client create！\r\n"); 
	
    while(1)
    {   	
        CH57xNET_MainTask();                                                    /* CH57xNET库主任务函数，需要在主循环中不断调用 */
        if(CH57xNET_QueryGlobalInt())CH57xNET_HandleGlobalInt();                /* 查询中断，如果有中断，则调用全局中断处理函数 */
        if( dns_flag == 0 ){
            i = DnsQuery(SocketId,url_dn3,ip);                                         /* 查询 */
             if(i){ 
                dns_flag = 1;
                if( i == 1 ){
                    PRINT("Domain name:%s \n",url_dn3);
                    PRINT(" HTTPs_IP=%d.%d.%d.%d\n\n",ip[0],ip[1],ip[2],ip[3]);
                }  
            }
        }     
	}
}

/*********************************** endfile **********************************/
