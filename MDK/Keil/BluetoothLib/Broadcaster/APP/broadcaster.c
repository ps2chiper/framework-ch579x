/********************************** (C) COPYRIGHT *******************************
* File Name          : broadcaster.c
* Author             : WCH
* Version            : V1.0
* Date               : 2018/12/10
* Description        : 广播应用程序，初始化广播连接参数，然后处于广播态一直广播
            
*******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "CONFIG.h"
#include "CH57x_common.h"
#include "CH57xBLE_LIB.h"
#include "battservice.h"
#include "devinfoservice.h"
#include "broadcaster.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// What is the advertising interval when device is discoverable (units of 625us, 80=50ms)
#define DEFAULT_ADVERTISING_INTERVAL          80

// Company Identifier: WCH 
#define WCH_COMPANY_ID                        0x07D7

// Length of bd addr as a string
#define B_ADDR_STR_LEN                        15

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint8 Broadcaster_TaskID;   // Task ID for internal task/event processing

// GAP - SCAN RSP data (max size = 31 bytes)
static uint8 scanRspData[] =
{
  // complete name
  0x0c,   // length of this data
  GAP_ADTYPE_LOCAL_NAME_COMPLETE,
  0x42,   // 'B'
  0x72,   // 'r'
  0x6f,   // 'o'
  0x61,   // 'a'
  0x64,   // 'd'
  0x63,   // 'c'
  0x61,   // 'a'
  0x73,   // 's'
  0x74,   // 't'
  0x65,   // 'e'
  0x72,   // 'r'

  // Tx power level
  0x02,   // length of this data
  GAP_ADTYPE_POWER_LEVEL,
  0       // 0dBm  
};

// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertisting)
static uint8 advertData[] = 
{ 
  // Flags; this sets the device to use limited discoverable
  // mode (advertises for 30 seconds at a time) instead of general
  // discoverable mode (advertises indefinitely)
  0x02,   // length of this data
  GAP_ADTYPE_FLAGS,
  GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

  // Broadcast of the data 
  0x04,   // length of this data including the data type byte
  GAP_ADTYPE_MANUFACTURER_SPECIFIC,      // manufacturer specific advertisement data type
  'b',
  'l',
  'e',
  0x04,
  GAP_ADTYPE_LOCAL_NAME_SHORT,
  'a',	
  'b',	
  'c'
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void Broadcaster_ProcessTMOSMsg( tmos_event_hdr_t *pMsg );
static void peripheralStateNotificationCB( gaprole_States_t newState );

/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static gapRolesCBs_t Broadcaster_BroadcasterCBs =
{
  peripheralStateNotificationCB,  // Profile State Change Callbacks
  NULL                            // When a valid RSSI is read from controller (not used by application)
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      Broadcaster_Init
 *
 * @brief   Initialization function for the Simple BLE Broadcaster App
 *          Task. This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by TMOS.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void Broadcaster_Init( uint8 task_id )
{
  Broadcaster_TaskID = task_id;

  // Setup the GAP Broadcaster Role Profile
  {
    // Device starts advertising upon initialization
    uint8 initial_advertising_enable = TRUE;

    // By setting this to zero, the device will go into the waiting state after
    // being discoverable, and will not being advertising again until the enabler 
    // is set back to TRUE
    uint16 gapRole_AdvertOffTime = 100;
      
    // Set the GAP Role Parameters
    GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &initial_advertising_enable );
    GAPRole_SetParameter( GAPROLE_ADVERT_OFF_TIME, sizeof( uint16 ), &gapRole_AdvertOffTime );    
    GAPRole_SetParameter( GAPROLE_SCAN_RSP_DATA, sizeof ( scanRspData ), scanRspData );
    GAPRole_SetParameter( GAPROLE_ADVERT_DATA, sizeof( advertData ), advertData );
  }

  // Set advertising interval
  {
    uint16 advInt = DEFAULT_ADVERTISING_INTERVAL;

    GAP_SetParamValue( TGAP_GEN_DISC_ADV_INT_MIN, advInt );
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_INT_MAX, advInt );
  }
  
  // Setup a delayed profile startup
  tmos_set_event( Broadcaster_TaskID, SBP_START_DEVICE_EVT );
}

/*********************************************************************
 * @fn      Broadcaster_ProcessEvent
 *
 * @brief   Simple BLE Broadcaster Application Task event processor. This
 *          function is called to process all events for the task. Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16 Broadcaster_ProcessEvent( uint8 task_id, uint16 events )
{
  
  //VOID task_id; // TMOS required parameter that isn't used in this function
  
  if ( events & SYS_EVENT_MSG ){
    uint8 *pMsg;

    if ( (pMsg = tmos_msg_receive( Broadcaster_TaskID )) != NULL ){
      Broadcaster_ProcessTMOSMsg( (tmos_event_hdr_t *)pMsg );

      // Release the TMOS message
      tmos_msg_deallocate( pMsg );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  if ( events & SBP_START_DEVICE_EVT ){
    // Start the Device
    GAPRole_BroadcasterStartDevice( &Broadcaster_BroadcasterCBs );
    
    return ( events ^ SBP_START_DEVICE_EVT );
  }
  
  // Discard unknown events
  return 0;
}

/*********************************************************************
 * @fn      Broadcaster_ProcessTMOSMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void Broadcaster_ProcessTMOSMsg( tmos_event_hdr_t *pMsg )
{
  switch ( pMsg->event )
  {
		default:
    break;
  }
}

/*********************************************************************
 * @fn      peripheralStateNotificationCB
 *
 * @brief   Notification from the profile of a state change.
 *
 * @param   newState - new state
 *
 * @return  none
 */
static void peripheralStateNotificationCB( gaprole_States_t newState )
{
  switch ( newState )
  {     
    default:
      break; 
  }
}
/*********************************************************************
*********************************************************************/
