/********************************** (C) COPYRIGHT *******************************
* File Name          : central.c
* Author             : WCH
* Version            : V1.0
* Date               : 2018/11/12
* Description        : 主机例程，主动扫描周围设备，连接至给定的从机设备地址，
*                      寻找自定义服务及特征，执行读写命令，需与从机例程配合使用,
                       并将从机设备地址修改为该例程目标地址，默认为(84:C2:E4:03:02:02)
*******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "CONFIG.h"
#include "CH57x_common.h"
#include "CH57xBLE_LIB.H"
#include "gattprofile.h"
#include "central.h"

/*********************************************************************
 * MACROS
 */

// Length of bd addr as a string
#define B_ADDR_STR_LEN                        15

/*********************************************************************
 * CONSTANTS
 */
// Maximum number of scan responses
#define DEFAULT_MAX_SCAN_RES                  10

// Scan duration in 0.625ms
#define DEFAULT_SCAN_DURATION                 2400

// Connection interval in 1.25ms
#define DEFAULT_CONNECTION_INTERVAL           8

// Connection supervision timeout in 10ms
#define DEFAULT_CONNECTION_TIMEOUT            100

// Discovey mode (limited, general, all)
#define DEFAULT_DISCOVERY_MODE                DEVDISC_MODE_ALL

// TRUE to use active scan
#define DEFAULT_DISCOVERY_ACTIVE_SCAN         TRUE

// TRUE to use white list during discovery
#define DEFAULT_DISCOVERY_WHITE_LIST          FALSE

// TRUE to use high scan duty cycle when creating link
#define DEFAULT_LINK_HIGH_DUTY_CYCLE          FALSE

// TRUE to use white list when creating link
#define DEFAULT_LINK_WHITE_LIST               FALSE

// Default RSSI polling period in 0.625ms
#define DEFAULT_RSSI_PERIOD                   1000

// Minimum connection interval (units of 1.25ms) if parameter update request is enabled
#define DEFAULT_UPDATE_MIN_CONN_INTERVAL      6

// Maximum connection interval (units of 1.25ms) if parameter update request is enabled
#define DEFAULT_UPDATE_MAX_CONN_INTERVAL      6

// Slave latency to use if parameter update request is enabled
#define DEFAULT_UPDATE_SLAVE_LATENCY          0

// Supervision timeout value (units of 10ms) if parameter update request is enabled
#define DEFAULT_UPDATE_CONN_TIMEOUT           600

// Default passcode
#define DEFAULT_PASSCODE                      0

// Default GAP pairing mode
#define DEFAULT_PAIRING_MODE                  GAPBOND_PAIRING_MODE_WAIT_FOR_REQ

// Default MITM mode (TRUE to require passcode or OOB when pairing)
#define DEFAULT_MITM_MODE                     TRUE

// Default bonding mode, TRUE to bond
#define DEFAULT_BONDING_MODE                  TRUE

// Default GAP bonding I/O capabilities
#define DEFAULT_IO_CAPABILITIES               GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT

// Default service discovery timer delay in 0.625ms
#define DEFAULT_SVC_DISCOVERY_DELAY           1600

// Default parameter update delay in 0.625ms
#define DEFAULT_PARAM_UPDATE_DELAY            3200

// Default read or write timer delay in 0.625ms
#define DEFAULT_READ_OR_WRITE_DELAY           1600

// Application states
enum
{
  BLE_STATE_IDLE,
  BLE_STATE_CONNECTING,
  BLE_STATE_CONNECTED,
  BLE_STATE_DISCONNECTING
};

// Discovery states
enum
{
  BLE_DISC_STATE_IDLE,                // Idle
  BLE_DISC_STATE_SVC,                 // Service discovery
  BLE_DISC_STATE_CHAR                 // Characteristic discovery
};
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

// Task ID for internal task/event processing
static uint8 centralTaskId;

// GAP GATT Attributes
static const uint8 centralDeviceName[GAP_DEVICE_NAME_LEN] = "Central";

// Number of scan results
static uint8 centralScanRes;

// Scan result list
static gapDevRec_t centralDevList[DEFAULT_MAX_SCAN_RES];

// Peer device address
static uint8 PeerAddrDef[B_ADDR_LEN] = { 0x02,0x02,0x03,0xE4,0xC2,0x84 };

// RSSI polling state
static uint8 centralRssi = TRUE;

// Parameter update state
static uint8 centralParamUpdate = TRUE;

// Connection handle of current connection 
static uint16 centralConnHandle = GAP_CONNHANDLE_INIT;

// Application state
static uint8 centralState = BLE_STATE_IDLE;

// Discovery state
static uint8 centralDiscState = BLE_DISC_STATE_IDLE;

// Discovered service start and end handle
static uint16 centralSvcStartHdl = 0;
static uint16 centralSvcEndHdl = 0;

// Discovered characteristic handle
static uint16 centralCharHdl = 0;

// Value to write
static uint8 centralCharVal = 0x5A;

// Value read/write toggle
static uint8 centralDoWrite = TRUE;

// GATT read/write procedure state
static uint8 centralProcedureInProgress = FALSE;
/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void centralProcessGATTMsg( gattMsgEvent_t *pMsg );
static void centralRssiCB( uint16 connHandle, int8  rssi );
static void centralEventCB( gapCentralRoleEvent_t *pEvent );
static void centralHciMTUChangeCB( uint16 maxTxOctets, uint16 maxRxOctets );
static void centralPasscodeCB( uint8 *deviceAddr, uint16 connectionHandle,
                                        uint8 uiInputs, uint8 uiOutputs );
static void centralPairStateCB( uint16 connHandle, uint8 state, uint8 status );
static void central_ProcessTMOSMsg( tmos_event_hdr_t *pMsg );
static void centralGATTDiscoveryEvent( gattMsgEvent_t *pMsg );
static void centralStartDiscovery( void );
static void centralAddDeviceInfo( uint8 *pAddr, uint8 addrType );
/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static const gapCentralRoleCB_t centralRoleCB =
{
  centralRssiCB,       // RSSI callback
  centralEventCB,       // Event callback
  centralHciMTUChangeCB       // MTU change callback
};

// Bond Manager Callbacks
static const gapBondCBs_t centralBondCB =
{
  centralPasscodeCB,
  centralPairStateCB
};


/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      Central_Init
 *
 * @brief   Initialization function for the Central App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notification).
 *
 * @param   task_id - the ID assigned by TMOS.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void Central_Init( uint8 task_id )
{
  centralTaskId = task_id;

  // Setup Central Profile
  {
    uint8 scanRes = DEFAULT_MAX_SCAN_RES;
    GAPRole_SetParameter ( GAPROLE_MAX_SCAN_RES, sizeof( uint8 ), &scanRes );
  }
  
  // Setup GAP
  GAP_SetParamValue( TGAP_GEN_DISC_SCAN, DEFAULT_SCAN_DURATION );
  GAP_SetParamValue( TGAP_LIM_DISC_SCAN, DEFAULT_SCAN_DURATION );
  GAP_SetParamValue( TGAP_CONN_EST_INT_MIN, DEFAULT_CONNECTION_INTERVAL );
  GAP_SetParamValue( TGAP_CONN_EST_INT_MAX, DEFAULT_CONNECTION_INTERVAL );
  GAP_SetParamValue( TGAP_CONN_EST_SUPERV_TIMEOUT, DEFAULT_CONNECTION_TIMEOUT );
  
  GGS_SetParameter( GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, (uint8 *) centralDeviceName );
 
  // Setup the GAP Bond Manager
  {
    uint32 passkey = DEFAULT_PASSCODE;
    uint8 pairMode = DEFAULT_PAIRING_MODE;
    uint8 mitm = DEFAULT_MITM_MODE;
    uint8 ioCap = DEFAULT_IO_CAPABILITIES;
    uint8 bonding = DEFAULT_BONDING_MODE;
    
    GAPBondMgr_SetParameter( GAPBOND_DEFAULT_PASSCODE, sizeof( uint32 ), &passkey );
    GAPBondMgr_SetParameter( GAPBOND_PAIRING_MODE, sizeof( uint8 ), &pairMode );
    GAPBondMgr_SetParameter( GAPBOND_MITM_PROTECTION, sizeof( uint8 ), &mitm );
    GAPBondMgr_SetParameter( GAPBOND_IO_CAPABILITIES, sizeof( uint8 ), &ioCap );
    GAPBondMgr_SetParameter( GAPBOND_BONDING_ENABLED, sizeof( uint8 ), &bonding );
  }  
  // Initialize GATT Client
  GATT_InitClient();
  // Register to receive incoming ATT Indications/Notifications
  GATT_RegisterForInd( centralTaskId );
  // Initialize GATT attributes
  GGS_AddService( GATT_ALL_SERVICES );         // GAP
  GATTServApp_AddService( GATT_ALL_SERVICES ); // GATT attributes
  
  // Setup a delayed profile startup
  tmos_set_event( centralTaskId, START_DEVICE_EVT );
}

/*********************************************************************
 * @fn      Central_ProcessEvent
 *
 * @brief   Central Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16 Central_ProcessEvent( uint8 task_id, uint16 events )
{
  if ( events & SYS_EVENT_MSG )
  {
    uint8 *pMsg;

    if ( (pMsg = tmos_msg_receive( centralTaskId )) != NULL )
    {
      central_ProcessTMOSMsg( (tmos_event_hdr_t *)pMsg );
      // Release the TMOS message
       tmos_msg_deallocate( pMsg );
    }
    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  if ( events & START_DEVICE_EVT )
  {
    // Start the Device
    GAPRole_CentralStartDevice( (gapCentralRoleCB_t *) &centralRoleCB );

    // Register with bond manager after starting device
    GAPBondMgr_Register( (gapBondCBs_t *) &centralBondCB );
    return ( events ^ START_DEVICE_EVT );
  }
  
  if ( events & START_SVC_DISCOVERY_EVT )
  {
    // start service discovery
    centralStartDiscovery( );
    return ( events ^ START_SVC_DISCOVERY_EVT );
  }

  if ( events & START_PARAM_UPDATE_EVT )
  {
    // start connect parameter update
    GAPRole_UpdateLink( centralConnHandle,
                           DEFAULT_UPDATE_MIN_CONN_INTERVAL,
                           DEFAULT_UPDATE_MAX_CONN_INTERVAL,
                           DEFAULT_UPDATE_SLAVE_LATENCY,
                           DEFAULT_UPDATE_CONN_TIMEOUT );
    return ( events ^ START_PARAM_UPDATE_EVT );
  }
  
  if ( events & START_READ_OR_WRITE_EVT )
  {   
    if( centralProcedureInProgress == FALSE )
    {
      if( centralDoWrite )
      {
        // Do a write
        attWriteReq_t req;
        
        req.cmd = FALSE;
        req.sig = FALSE;
        req.handle = centralCharHdl;
        req.len = 1;
        req.pValue = GATT_bm_alloc(centralConnHandle,ATT_WRITE_REQ,req.len,NULL,0);
        if ( req.pValue != NULL )
        {
          *req.pValue = centralCharVal;
          
          if( GATT_WriteCharValue(centralConnHandle,&req,centralTaskId) == SUCCESS )
          {      
            centralProcedureInProgress = TRUE;
            centralDoWrite = !centralDoWrite;
            tmos_start_task( centralTaskId, START_READ_OR_WRITE_EVT, DEFAULT_READ_OR_WRITE_DELAY);
          }
          else
          {
            GATT_bm_free((gattMsg_t *)&req, ATT_WRITE_REQ);
          }
        }  
      }
      else
      {
        // Do a read
        attReadReq_t req;  
        
        req.handle = centralCharHdl;        
        if( GATT_ReadCharValue( centralConnHandle, &req, centralTaskId ) == SUCCESS )
        {
          centralProcedureInProgress = TRUE;
          centralDoWrite = !centralDoWrite;
        }
      }
    }
    return ( events ^ START_READ_OR_WRITE_EVT );
  }

  // Discard unknown events
  return 0;
}

/*********************************************************************
 * @fn      central_ProcessTMOSMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void central_ProcessTMOSMsg( tmos_event_hdr_t *pMsg )
{
  switch ( pMsg->event )
  {
    case GATT_MSG_EVENT:
      centralProcessGATTMsg( (gattMsgEvent_t *) pMsg );
      break;
  }
}

/*********************************************************************
 * @fn      centralProcessGATTMsg
 *
 * @brief   Process GATT messages
 *
 * @return  none
 */
static void centralProcessGATTMsg( gattMsgEvent_t *pMsg )
{
  if ( centralState != BLE_STATE_CONNECTED )
  {
    // In case a GATT message came after a connection has dropped,
    // ignore the message
    return;
  }
  
  if ( ( pMsg->method == ATT_EXCHANGE_MTU_RSP ) ||
       ( ( pMsg->method == ATT_ERROR_RSP ) &&
         ( pMsg->msg.errorRsp.reqOpcode == ATT_EXCHANGE_MTU_REQ ) ) )
  {
    if ( pMsg->method == ATT_ERROR_RSP )
    {
      uint8 status = pMsg->msg.errorRsp.errCode;
      
      PRINT( "Exchange MTU Error: %x\n", status );
    }
    centralProcedureInProgress = FALSE;
  }
  
  if ( pMsg->method == ATT_MTU_UPDATED_EVENT )
  {
    // After a successful read, display the read value
    PRINT("MTU: %x\n",pMsg->msg.mtuEvt.MTU);
  }    
  
  if ( ( pMsg->method == ATT_READ_RSP ) ||
       ( ( pMsg->method == ATT_ERROR_RSP ) &&
         ( pMsg->msg.errorRsp.reqOpcode == ATT_READ_REQ ) ) )
  {
    if ( pMsg->method == ATT_ERROR_RSP )
    {
      uint8 status = pMsg->msg.errorRsp.errCode;
      
      PRINT( "Read Error: %x\n", status );
    }
    else
    {
      // After a successful read, display the read value
      PRINT("Read rsp: %x\n",*pMsg->msg.readRsp.pValue);
    }
    centralProcedureInProgress = FALSE;
  }
  else if ( ( pMsg->method == ATT_WRITE_RSP ) ||
       ( ( pMsg->method == ATT_ERROR_RSP ) &&
         ( pMsg->msg.errorRsp.reqOpcode == ATT_WRITE_REQ ) ) )
  {
    
    if ( pMsg->method == ATT_ERROR_RSP == ATT_ERROR_RSP )
    {
      uint8 status = pMsg->msg.errorRsp.errCode;
      
      PRINT( "Write Error: %x\n", status );
    }
    else
    {
      // After a succesful write, display the value that was written and increment value
      PRINT( "Write sent: %x\n", centralCharVal);      
    }
    
    centralProcedureInProgress = FALSE;    

  }
  else if ( centralDiscState != BLE_DISC_STATE_IDLE )
  {
    centralGATTDiscoveryEvent( pMsg );
  }
  GATT_bm_free(&pMsg->msg, pMsg->method);
}

/*********************************************************************
 * @fn      centralRssiCB
 *
 * @brief   RSSI callback.
 *
 * @param   connHandle - connection handle
 * @param   rssi - RSSI
 *
 * @return  none
 */
static void centralRssiCB( uint16 connHandle, int8 rssi )
{
  PRINT( "RSSI : -%d dB \n", -rssi );
}

/*********************************************************************
 * @fn      centralHciMTUChangeCB
 *
 * @brief   MTU changed callback.
 *
 * @param   maxTxOctets - Max tx octets
 * @param   maxRxOctets - Max rx octets
 *
 * @return  none
 */
static void centralHciMTUChangeCB( uint16 maxTxOctets, uint16 maxRxOctets )
{
  attExchangeMTUReq_t req;
  
  req.clientRxMTU = maxRxOctets;
  GATT_ExchangeMTU(centralConnHandle,&req,centralTaskId);
  PRINT("exchange mtu:%d\n",maxRxOctets);
  centralProcedureInProgress = TRUE;
}

/*********************************************************************
 * @fn      centralEventCB
 *
 * @brief   Central event callback function.
 *
 * @param   pEvent - pointer to event structure
 *
 * @return  none
 */
static void centralEventCB( gapCentralRoleEvent_t *pEvent )
{
  switch ( pEvent->gap.opcode )
  {
    case GAP_DEVICE_INIT_DONE_EVENT:  
      {
        PRINT( "Discovering...\n" );
        GAPRole_CentralStartDiscovery( DEFAULT_DISCOVERY_MODE,
                               DEFAULT_DISCOVERY_ACTIVE_SCAN,
                               DEFAULT_DISCOVERY_WHITE_LIST );      

      }
      break;

    case GAP_DEVICE_INFO_EVENT:
      {
        // Add device to list
        centralAddDeviceInfo( pEvent->deviceInfo.addr, pEvent->deviceInfo.addrType );        
      }
      break;
      
      case GAP_DEVICE_DISCOVERY_EVENT:
      {
        uint8 i;        
        
        // See if peer device has been discovered 
        for ( i = 0; i < centralScanRes; i++ )
        {
          if ( tmos_memcmp( PeerAddrDef, centralDevList[i].addr , B_ADDR_LEN ) )
            break;
        }
        
        // Peer device not found
        if(i == centralScanRes)
        {
          PRINT( "Device not found...\n" );
          centralScanRes = 0;
          GAPRole_CentralStartDiscovery( DEFAULT_DISCOVERY_MODE,
                               DEFAULT_DISCOVERY_ACTIVE_SCAN,
                               DEFAULT_DISCOVERY_WHITE_LIST ); 
          PRINT( "Discovering...\n" );
        }          

        // Peer device found
        else
        {
          PRINT( "Device found...\n" );                 
          GAPRole_CentralEstablishLink( DEFAULT_LINK_HIGH_DUTY_CYCLE,
                                        DEFAULT_LINK_WHITE_LIST,
                                        centralDevList[i].addrType, 
                                        centralDevList[i].addr );
          PRINT( "Connecting...\n" );          
        }       
      }
      break;

    case GAP_LINK_ESTABLISHED_EVENT:
      {
        if ( pEvent->gap.hdr.status == SUCCESS )
        {
          centralState = BLE_STATE_CONNECTED;
          centralConnHandle = pEvent->linkCmpl.connectionHandle;
          centralProcedureInProgress = TRUE;             
          
          // Initiate service discovery
          tmos_start_task( centralTaskId, START_SVC_DISCOVERY_EVT, DEFAULT_SVC_DISCOVERY_DELAY);
          
          // See if initiate connect parameter update
          if ( centralParamUpdate )
          {
            tmos_start_task( centralTaskId, START_PARAM_UPDATE_EVT, DEFAULT_PARAM_UPDATE_DELAY);
          }
          // See if start RSSI polling
          if ( centralRssi )
          {
            GAPRole_CentralStartRssi( centralConnHandle, DEFAULT_RSSI_PERIOD );
          }

          PRINT( "Connected...\n" );
        }
        else
        {
          centralState = BLE_STATE_IDLE;
          centralConnHandle = GAP_CONNHANDLE_INIT;
          centralDiscState = BLE_DISC_STATE_IDLE;
          PRINT( "Connect Failed...Reason:%-8X\n",pEvent->gap.hdr.status );
        }
      }
      break;

    case GAP_LINK_TERMINATED_EVENT:
      {
        centralState = BLE_STATE_IDLE;
        centralConnHandle = GAP_CONNHANDLE_INIT;
        centralDiscState = BLE_DISC_STATE_IDLE;
        centralCharHdl = 0;
        centralScanRes = 0;
        centralProcedureInProgress = FALSE;
        PRINT( "Disconnected...Reason:%8x\n", pEvent->linkTerminate.reason );
        PRINT( "Discovering..." );
        GAPRole_CentralStartDiscovery( DEFAULT_DISCOVERY_MODE,
                               DEFAULT_DISCOVERY_ACTIVE_SCAN,
                               DEFAULT_DISCOVERY_WHITE_LIST ); 

      }
      break;

    case GAP_LINK_PARAM_UPDATE_EVENT:
      {
        PRINT( "Param Update...\n" );
      }
      break;
      
    default:
      break;
  }
}

/*********************************************************************
 * @fn      pairStateCB
 *
 * @brief   Pairing state callback.
 * 
 * @return  none
 */
static void centralPairStateCB( uint16 connHandle, uint8 state, uint8 status )
{
  if ( state == GAPBOND_PAIRING_STATE_STARTED )
  {
    PRINT( "Pairing started:%d\n", status);
  }
  else if ( state == GAPBOND_PAIRING_STATE_COMPLETE )
  {
    if ( status == SUCCESS )
    {
      PRINT( "Pairing success\n" );
    }
    else
    {
      PRINT( "Pairing fail\n" );
    }
  }
  else if ( state == GAPBOND_PAIRING_STATE_BONDED )
  {
    if ( status == SUCCESS )
    {
        PRINT( "Bonding success\n" );
    }
  }
  else if (state == GAPBOND_PAIRING_STATE_BOND_SAVED)
  {
    if (status == SUCCESS)
    {
      PRINT("Bond save success\n");
    }
    else
    {
      PRINT("Bond save failed: %d\n", status);
    }
  }
}

/*********************************************************************
 * @fn      centralPasscodeCB
 *
 * @brief   Passcode callback.
 *
 * @return  none
 */
static void centralPasscodeCB( uint8 *deviceAddr, uint16 connectionHandle,
                                        uint8 uiInputs, uint8 uiOutputs )
{
  uint32  passcode;

  // Create random passcode
  passcode = tmos_rand( );
  passcode %= 1000000;
  // Display passcode to user
  if ( uiOutputs != 0 )
  {
    PRINT("Passcode:%d\n",(int)passcode);
  }
  // Send passcode response
  GAPBondMgr_PasscodeRsp( connectionHandle, SUCCESS, passcode );
}

/*********************************************************************
 * @fn      centralStartDiscovery
 *
 * @brief   Start service discovery.
 *
 * @return  none
 */
static void centralStartDiscovery( void )
{
  uint8 uuid[ATT_BT_UUID_SIZE] = { LO_UINT16(SIMPLEPROFILE_SERV_UUID),
                                   HI_UINT16(SIMPLEPROFILE_SERV_UUID) };
  
  // Initialize cached handles
  centralSvcStartHdl = centralSvcEndHdl = centralCharHdl = 0;

  centralDiscState = BLE_DISC_STATE_SVC;
  
  // Discovery simple BLE service
  GATT_DiscPrimaryServiceByUUID( centralConnHandle,
                                 uuid,
                                 ATT_BT_UUID_SIZE,
                                 centralTaskId );
}

/*********************************************************************
 * @fn      centralGATTDiscoveryEvent
 *
 * @brief   Process GATT discovery event
 *
 * @return  none
 */
static void centralGATTDiscoveryEvent( gattMsgEvent_t *pMsg )
{
  attReadByTypeReq_t req;
  
  if ( centralDiscState == BLE_DISC_STATE_SVC )
  {
    // Service found, store handles
    if ( pMsg->method == ATT_FIND_BY_TYPE_VALUE_RSP &&
         pMsg->msg.findByTypeValueRsp.numInfo > 0 )
    {
      centralSvcStartHdl = ATT_ATTR_HANDLE(pMsg->msg.findByTypeValueRsp.pHandlesInfo,0);
      centralSvcEndHdl = ATT_GRP_END_HANDLE(pMsg->msg.findByTypeValueRsp.pHandlesInfo,0);
      
      // Display Profile Service handle range
      PRINT("Found Profile Service handle : %x ~ %x \n",centralSvcStartHdl,centralSvcEndHdl);

    }
    // If procedure complete
    if ( ( pMsg->method == ATT_FIND_BY_TYPE_VALUE_RSP  && 
           pMsg->hdr.status == bleProcedureComplete ) ||
         ( pMsg->method == ATT_ERROR_RSP ) )
    {
      if ( centralSvcStartHdl != 0 )
      {
        // Discover characteristic
        centralDiscState = BLE_DISC_STATE_CHAR;
        req.startHandle = centralSvcStartHdl;
        req.endHandle = centralSvcEndHdl;
        req.type.len = ATT_BT_UUID_SIZE;
        req.type.uuid[0] = LO_UINT16(SIMPLEPROFILE_CHAR1_UUID);
        req.type.uuid[1] = HI_UINT16(SIMPLEPROFILE_CHAR1_UUID);

        GATT_ReadUsingCharUUID( centralConnHandle, &req, centralTaskId );
      }
    }
  }
  else if ( centralDiscState == BLE_DISC_STATE_CHAR )
  {
    // Characteristic found, store handle
    if ( pMsg->method == ATT_READ_BY_TYPE_RSP && 
         pMsg->msg.readByTypeRsp.numPairs > 0 )
    {
      centralCharHdl = BUILD_UINT16( pMsg->msg.readByTypeRsp.pDataList[0],
                                       pMsg->msg.readByTypeRsp.pDataList[1] );
      centralProcedureInProgress = FALSE;
      
      // Start do read or write
      tmos_start_task( centralTaskId, START_READ_OR_WRITE_EVT, DEFAULT_READ_OR_WRITE_DELAY);
      
      // Display Characteristic 1 handle
      PRINT("Found Characteristic 1 handle : %x \n",centralCharHdl);      
    }
    centralDiscState = BLE_DISC_STATE_IDLE;
  }    
}

/*********************************************************************
 * @fn      centralAddDeviceInfo
 *
 * @brief   Add a device to the device discovery result list
 *
 * @return  none
 */
static void centralAddDeviceInfo( uint8 *pAddr, uint8 addrType )
{
  uint8 i;
  
  // If result count not at max
  if ( centralScanRes < DEFAULT_MAX_SCAN_RES )
  {
    // Check if device is already in scan results
    for ( i = 0; i < centralScanRes; i++ )
    {
      if ( tmos_memcmp( pAddr, centralDevList[i].addr , B_ADDR_LEN ) )
      {
        return;
      }
    }
    // Add addr to scan result list
    tmos_memcpy( centralDevList[centralScanRes].addr, pAddr, B_ADDR_LEN );
    centralDevList[centralScanRes].addrType = addrType;
    // Increment scan result count
    centralScanRes++;
    // Display device addr
    PRINT("Device %d - Addr %x %x %x %x %x %x \n",centralScanRes,
                                                  centralDevList[centralScanRes-1].addr[0],
                                                  centralDevList[centralScanRes-1].addr[1],
                                                  centralDevList[centralScanRes-1].addr[2],
                                                  centralDevList[centralScanRes-1].addr[3],
                                                  centralDevList[centralScanRes-1].addr[4],
                                                  centralDevList[centralScanRes-1].addr[5]);

  }
}

/************************ endfile @ central **************************/
