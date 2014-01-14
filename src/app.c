/*
 *  uC/OS-III - Getting Started Demo Application
 *
 **/

/*
************************************************************************************************
*                                             INCLUDE FILES
************************************************************************************************
*/
#include <includes.h>
#include <string.h>
#include <shmemory.h>
#include <protocols.h>
#include <ethernet.h>
#include <type.h>
#include <types.h>
#include <xprintf.h>
#include <ssp.h>
#include <generic.h>

/*
************************************************************************************************
*                                             LOCAL DEFINES
************************************************************************************************
*/
#define APP_STACK_SIZE 128

/*
************************************************************************************************
*                                            LOCAL VARIABLES
************************************************************************************************
*/
static OS_TCB App_TaskStartTCB;               /* Application Startup Task Control Block (TCB) */
static OS_TCB App_TaskLEDTCB;
static OS_TCB ETH_RCVTCB;
static OS_TCB ETH_TRANSTCB;
static OS_TCB APP_NETTCB;

static CPU_STK App_TaskStartStk[APP_CFG_TASK_START_STK_SIZE];             /* Start Task Stack */
static CPU_STK App_TaskLEDStk[APP_STACK_SIZE];
static CPU_STK ETH_RCVTCBStk[APP_STACK_SIZE];
static CPU_STK ETH_TRANSTCBStk[APP_STACK_SIZE];
static CPU_STK APP_NETTCBStk[APP_STACK_SIZE];


/*
 * Network stack related structures
 */

const static uint8_t eth_mac[6] = {0x5a,0x01,0x02,0x03,0x04,0x05};

const static uint16 segmentValue[16u] = {
    0b11011011u,
    0b01010000u,
    0b00011111u,
    0b01011101u,
    0b11010100u,
    0b11001101u,
    0b11001111u,
    0b01011000u,
    0b11011111u,
    0b11011101u,
    0b11011110u,
    0b11000111u,
    0b10001011u,
    0b01010111u,
    0b10001111u,
    0b10001110u
};


OS_SEM RX_SEM;

OS_SEM  ECHOSem;         // semaphore used for signaling incoming CHAT segment

OS_Q TXNicQ;

OS_Q RX_Q;

INT16U tcp_ID;

typedef enum {
    ConnectionState_Offline = 0u,
    ConnectionState_User = 1u,
    ConnectionState_Pass = 2u,
    ConnectionState_Online = 3u
} ConnectionState;

char *commandInBuffer;
char *commandOutBuffer;
uint16 commandInBufferPos;
uint16 commandOutBufferPos;
uint8 messageReady;

const static char user[] = "admin";
const static char pass[] = "pwd";
static ConnectionState connectionState;

static uint8 segmentSelId;  // sel id for seven segment

uint32 replyCounter; // replyCoutner for pin


#define START_SRAM_BANK1 (0x20080000)
#define RX_BUF_START (START_SRAM_BANK1)
#define RX_BUF_LEN ((((EMAC_ETH_MAX_FLEN>>2)+2)*EMAC_NUM_RX_FRAG)+2/*+2 must be 32 bit aligned*/)

#define TX_BUF_START (RX_BUF_START+RX_BUF_LEN)
#define TX_BUF_LEN (((EMAC_ETH_MAX_FLEN>>2)+2)*EMAC_NUM_TX_FRAG)

OS_MEM PacketMemArea;


/*
************************************************************************************************
*                                         FUNCTION PROTOTYPES
************************************************************************************************
*/
static void App_TaskStart (void  *p_arg);
static void App_TaskLED (void *p_arg);
static void ETH_RECV(void *p_arg);
static void ETH_TRANS(void *p_arg);
static void APP_NET(void *p_arg);

static void PostEchoEvent(void);

uint8 pingHost(uint8 count, char* address);
static void commandProcess(char *data, uint16 dataLength); 
static void processCommand(char *data);
static void setSevenSegment(uint8 num, uint8 point);
uint8 doArp(char* broadcastAddress, char* clearAddress);


/*
************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code. It is assumed that your code will 
*               call main() once you have performed all necessary initialization.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : Startup Code.
*
* Note(s)     : none.
************************************************************************************************
*/

int 
main (void)
{
    tcp_ID = 0u;

    
     OS_ERR   os_err = OS_ERR_NONE;
#if (CPU_CFG_NAME_EN == DEF_ENABLED)
    CPU_ERR  cpu_err;
#endif

    BSP_PreInit();                                 /* initialize basic board support routines */

#if (CPU_CFG_NAME_EN == DEF_ENABLED)
    CPU_NameSet((CPU_CHAR *)CSP_DEV_NAME,
                (CPU_ERR  *)&cpu_err);
#endif

    /*right now there is no memory allocation from the heap possible*/
    //Mem_Init();                                       /* Initialize memory management  module */


    OSMemCreate((OS_MEM*)&PacketMemArea,
        	    		(CPU_CHAR *)"PacketMemArea",
        	    		(void*)(uint32_t *)START_SRAM_BANK1,
        	    		(OS_MEM_QTY ) (EMAC_NUM_TX_FRAG+EMAC_NUM_RX_FRAG),
        	    		(OS_MEM_SIZE)(((EMAC_ETH_MAX_FLEN+2)>>2)/*+4 byte framelength due to alignmnet*/),
        	    		(OS_ERR*)&os_err);
    if(os_err != OS_ERR_NONE)
    	for(;;);
        
    commandInBuffer = (char*)OSMemGet(&PacketMemArea, &os_err);
    commandOutBuffer = (char*)OSMemGet(&PacketMemArea, &os_err);
    commandInBufferPos = 0u;
    commandOutBufferPos = 0u;
    messageReady = 0u;
    connectionState = ConnectionState_Offline;
    
    // Initialize 433MHz module
    Pin_setFunction(0u, 7u, Pin_Function_SecondAlternate);  //SCK
    Pin_setMode(0u, 7u, Pin_Mode_NoPullUpDown);
    Pin_setFunction(0u, 8u, Pin_Function_SecondAlternate);
    Pin_setMode(0u, 8u, Pin_Mode_NoPullUpDown);
    Pin_setFunction(0u, 9u, Pin_Function_SecondAlternate);
    Pin_setMode(0u, 9u, Pin_Mode_NoPullUpDown);
    Ssp_initialize(Ssp1, 
                   1E6, 
                   Ssp_DataSize_8Bit,
                   Ssp_FrameFormat_Spi,
                   Ssp_Mode_Master,
                   Ssp_Loopback_Disabled,
                   Ssp_SlaveOutput_Enabled,
                   Ssp_ClockOutPolarity_Low,
                   Ssp_ClockOutPhase_First
                  );
    segmentSelId = Ssp_initializeSel(Ssp1, 1u, 31u);
    
    Ssp_write(Ssp1, segmentSelId, 0b01010101u);
    
    replyCounter = 0u;

    OSSemCreate(&ECHOSem, "ECHOSem", 0, &os_err);
    if(os_err != OS_ERR_NONE)
        	for(;;);

    OSQCreate (&TXNicQ,"TXNicQ", EMAC_NUM_TX_FRAG,&os_err);
    if(os_err != OS_ERR_NONE)
    	for(;;);

    OSQCreate (&RX_Q,"RX_Q", EMAC_NUM_RX_FRAG,&os_err);
    if(os_err != OS_ERR_NONE)
    	for(;;);



    OSInit(&os_err);                                                        /* Init uC/OS-III */
    if(os_err != OS_ERR_NONE)
      for(;;);



    OSTaskCreate((OS_TCB      *)&App_TaskStartTCB,                  /* Create the Start Task */
                 (CPU_CHAR    *)"Start",
                 (OS_TASK_PTR  )App_TaskStart,
                 (void        *)0,
                 (OS_PRIO      )4,                          
                 (CPU_STK     *)App_TaskStartStk,
                 (CPU_STK_SIZE )APP_CFG_TASK_START_STK_SIZE_LIMIT,
                 (CPU_STK_SIZE )APP_CFG_TASK_START_STK_SIZE,
                 (OS_MSG_QTY   )0u,
                 (OS_TICK      )0u,
                 (void        *)0,
                 (OS_OPT       )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR      *)&os_err);

    OSStart(&os_err);                                                   /* Start Multitasking */
    if(os_err != OS_ERR_NONE)                                         /* shall never get here */
          for(;;);
    return (0);
}

/*
************************************************************************************************
*                                          STARTUP TASK
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Arguments   : p_arg   is the argument passed to 'AppTaskStart()' by 'OSTaskCreate()'.
*
* Returns     : none
*
************************************************************************************************
*/

static  void  
App_TaskStart (void *p_arg)
{
  OS_ERR       err;
  EMAC_CFG_Type EMAC_ConfigStruct;


  EMAC_ConfigStruct.Mode = EMAC_MODE_100M_FULL;
  EMAC_ConfigStruct.pbEMAC_Addr = (uint8*)&(eth_mac[0u]);

  (void)p_arg;                                                    /* Prevent Compiler Warning */
  CPU_Init();                                               /* Initialize the uC/CPU Services */
  OS_CSP_TickInit();                                        /* Initialize the Tick Interrupt. */

#if (OS_CFG_STAT_TASK_EN > 0u)
  OSStatTaskCPUUsageInit(&err);
#endif


#ifdef CPU_CFG_INT_DIS_MEAS_EN
  CPU_IntDisMeasMaxCurReset();
#endif

  //EMAC Pin Configuration
  EMAC_PinCfg();

  if(OS_EMAC_Init(&RX_SEM) < 0)
	  for(;;);

  if(CSP_IntVectReg ( CSP_INT_CTRL_NBR_MAIN,
		  	  	  	  	  	  	  CSP_INT_SRC_NBR_ETHER_00,
                               (CPU_FNCT_PTR)    CSP_IntETH_Handler,
                               NULL) == DEF_FAIL) {
	  	  for(;;);
  }

  /*clear interrupts pending IN OS AND HW (NVIC)*/
  CSP_IntClr (CSP_INT_CTRL_NBR_MAIN, CSP_INT_SRC_NBR_ETHER_00);

  /*enable ETH interrupts IN OS and HW (NVIC)*/
  CSP_IntEn (CSP_INT_CTRL_NBR_MAIN, CSP_INT_SRC_NBR_ETHER_00);

   /*ENABLES EMAC and interrupts IN HW (NIC)*/
  (void) EMAC_Init(&EMAC_ConfigStruct);//will always return SUCCESS


  OSTaskCreate((OS_TCB     *)&ETH_RCVTCB,
                  (CPU_CHAR   *)"ETH_RECV",
                  (OS_TASK_PTR )ETH_RECV,
                  (void       *)0,
                  (OS_PRIO     )2,
                  (CPU_STK    *)ETH_RCVTCBStk,
                  (CPU_STK_SIZE)0,/*watermark*/
                  (CPU_STK_SIZE)APP_STACK_SIZE,
                  (OS_MSG_QTY  )0,
                  (OS_TICK     )0,
                  (void       *)0,
                  (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                  (OS_ERR     *)&err);
  if(err != OS_ERR_NONE)
	  for(;;);

#if 1
  OSTaskCreate((OS_TCB     *)&ETH_TRANSTCB,
                  (CPU_CHAR   *)"ETH_TRANS",
                  (OS_TASK_PTR )ETH_TRANS,
                  (void       *)0,
                  (OS_PRIO     )3,
                  (CPU_STK    *)ETH_TRANSTCBStk,
                  (CPU_STK_SIZE)0,/*watermark*/
                  (CPU_STK_SIZE)APP_STACK_SIZE,
                  (OS_MSG_QTY  )0,
                  (OS_TICK     )0,
                  (void       *)0,
                  (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                  (OS_ERR     *)&err);
  if(err != OS_ERR_NONE)
  	  for(;;);
#endif

  OSTaskCreate((OS_TCB     *)&APP_NETTCB,
                 (CPU_CHAR   *)"APP_NET",
                 (OS_TASK_PTR )APP_NET,
                 (void       *)0,
                 (OS_PRIO     )4,
                 (CPU_STK    *)APP_NETTCBStk,
                 (CPU_STK_SIZE)0,/*watermark*/
                 (CPU_STK_SIZE)APP_STACK_SIZE,
                 (OS_MSG_QTY  )0,
                 (OS_TICK     )0,
                 (void       *)0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);
  if(err != OS_ERR_NONE)
  	  for(;;);

#if 1
  OSTaskCreate((OS_TCB     *)&App_TaskLEDTCB,
                 (CPU_CHAR   *)"LED",
                 (OS_TASK_PTR )App_TaskLED,
                 (void       *)0,
                 (OS_PRIO     )5,
                 (CPU_STK    *)App_TaskLEDStk,
                 (CPU_STK_SIZE)0,/*watermark*/
                 (CPU_STK_SIZE)APP_STACK_SIZE,
                 (OS_MSG_QTY  )0,
                 (OS_TICK     )0,
                 (void       *)0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);
  if(err != OS_ERR_NONE)
  	  for(;;);
#endif


  while (DEF_TRUE) {


    OSTimeDlyHMSM(0u, 0u, 1u, 0u, OS_OPT_TIME_HMSM_STRICT, &err);

  }
}

/*receive frame task*/
static void ETH_RECV(void *p_arg) {


	OS_ERR os_err;
	CPU_TS ts;
	uint16_t len;
	EMAC_PACKETBUF_Type frame_p;
	LLC_HEADER *llc_p;
	ETHERNET_HEADER *pPacket;
    
    len = 0;

	for(;;) {
		OSSemPend(&RX_SEM, 0, OS_OPT_PEND_BLOCKING, &ts, &os_err);

	    llc_p = (LLC_HEADER *)OSMemGet(&PacketMemArea, &os_err);
	    if(os_err == OS_ERR_NONE) {

	    	len = (uint16_t)EMAC_GetReceiveDataSize();
	    	if(len > EMAC_ETH_MAX_FLEN) 
            {
                len = EMAC_ETH_MAX_FLEN;
            }

	    	llc_p->Bytes = len - 3;/*do not copy checksum*/

	    	frame_p.ulDataLen = (uint32_t)llc_p->Bytes;
	    	frame_p.pbDataBuf = (uint32_t *)llc_p->Data;

	    	/*copy to memory buffer*/
	    	EMAC_ReadPacketBuffer(&frame_p);

	    	/*finally free frame from HW*/
	    	EMAC_UpdateRxConsumeIndex();

	    	pPacket = (ETHERNET_HEADER *) llc_p->Data;

	    	ETHER_AllTypesProcess (pPacket);								// type?
	    	OSMemPut(&PacketMemArea, llc_p,&os_err);

	    } else { //simply free the frame again
	    	/*free frame anyhow*/
	    	EMAC_UpdateRxConsumeIndex();
	    	OSMemPut(&PacketMemArea, llc_p,&os_err);
	    }
	} /*end for*/

}


/*transmit frame task*/
static void ETH_TRANS(void *p_arg) {

	OS_ERR os_err;
	OS_MSG_SIZE msg_size = 0;
	CPU_TS ts;
	LLC_HEADER *llc_p;
	EMAC_PACKETBUF_Type frame_p;


	  for(;;) {
	    // wait on message in queue
	    llc_p = (LLC_HEADER *)OSQPend(&TXNicQ, 0, OS_OPT_PEND_BLOCKING, &msg_size, &ts, &os_err);
	    if(os_err != OS_ERR_NONE)
	    	for(;;);

	    frame_p.ulDataLen = llc_p->Bytes;
	    frame_p.pbDataBuf = (uint32_t *)(llc_p->Data);

	    /*write frame to dma location -> ready to send*/
	    EMAC_WritePacketBuffer(&frame_p);
	    EMAC_UpdateTxProduceIndex();
	    OSMemPut(&PacketMemArea, llc_p,&os_err);

	  }

}

void PostEchoEvent()
{
 OS_ERR os_err;
 OSSemPost(&ECHOSem, OS_OPT_POST_1, &os_err);
 if(os_err != OS_ERR_NONE)
	 for(;;);

}


/*net application*/
static void APP_NET(void *p_arg) {

	OS_ERR os_err;
	CPU_TS ts;
	int socketno;                        // store next socket number
	INT8U err;                           // local error variable
	INT16U BytesRcvd;                    // number of bytes to read
	INT16U *Dataptr;                     // local data buffer


	SOCKET_allinit();

	// -- socket setup --
	socketno = SOCKET_getsocket();       // get next socket number
	SOCKET_bind (socketno, 23);        // bind to 1024 port
	SOCKET_seteventproc (socketno,
	               &PostEchoEvent);       // set event procedure PostChatEvent
	SOCKET_listen (socketno);            // go to listen state


	 while (1) {

	  OSSemPend(&ECHOSem, 0, OS_OPT_PEND_BLOCKING, &ts, &os_err); // wait for TCP-Echo event
	  if(os_err != OS_ERR_NONE)
		  for(;;);
	  //OSSemPend(ECHOSem, 0, &err);         // wait for TCP-Echo event

	  if ( SOCKET_status (socketno) == TCP_STATE_CLOSED )
	  {
	   SOCKET_freesocket (socketno);       // give socket back
	   socketno = SOCKET_getsocket();      // get next free socket number
	   SOCKET_bind (socketno, 23);       // bind to 1024 port
	   SOCKET_seteventproc (socketno,
	                 &PostEchoEvent);      // set event procedure PostChatEvent
	   SOCKET_listen (socketno);           // go to listen state
	  }
	  BytesRcvd = SOCKET_RxBytes (socketno);     // get number of bytes being in RxBuffer
	  if (BytesRcvd > 0)                         // if payload has been received at socket 0
	  {
	    Dataptr = (INT16U *) OSMemGet(&PacketMemArea, &os_err);          // Dataptr is overlaid with LocalBuf
	    if(Dataptr!=NULL) {
	      SOCKET_read  (socketno,
	                  (INT16U *)Dataptr, BytesRcvd);// reads incoming data from socket read buffer
          commandProcess((char*)Dataptr, (uint16)BytesRcvd);
          if (messageReady == 1u)
          {
                SOCKET_write (socketno,
                            (INT16U *)commandOutBuffer, strlen(commandOutBuffer));
	                  messageReady = 0u;
          }
	      OSMemPut(&PacketMemArea, (void *)Dataptr,&os_err);
	      if(os_err != OS_ERR_NONE)
	    	  for(;;);
	    }

	  }
	 }

}

/*
************************************************************************************************
*                                          Application TASK
*
* Description : This is an example of a simple application task that blinks a single LED.
*
* Arguments   : p_arg   is the argument passed to 'AppTaskStart()' by 'OSTaskCreate()'.
*
* Returns     : none
*
************************************************************************************************
*/
static void 
App_TaskLED (void *p_arg) 
{
  OS_ERR       err;

  (void)p_arg;                  /* Prevent Compiler Warning */
  int toggle = 0;
  CLOCK_InitStandardTimers();
  CLOCK_Start();
  
  GPIO_SetValue(1,(1<<29));


  GPIO_SetDir(1,(1<<29),1);    /* Port, Mask, Direction 1 = Output*/
  while(DEF_TRUE) {

	CLOCK_UpdateTime();
	OSTimeDlyHMSM(0,0,0,250,OS_OPT_TIME_HMSM_STRICT,&err);
	if(toggle == 0) {
		GPIO_ClearValue(1,(1<<29));
		toggle = 1;
	} else {
		GPIO_SetValue(1,(1<<29));
				toggle = 0;
	}
  }

}
/*! EOF */
    
void errorCommand()
{
    xsnprintf(commandOutBuffer,EMAC_ETH_MAX_FLEN,"ERR: Command too long\n");
    messageReady = 1u;
}

void errorWiFly()
{
    xsnprintf(commandOutBuffer,EMAC_ETH_MAX_FLEN,"ERR: WiFly command too long\n");
    messageReady = 1u;
}

void printUnknownCommand(void)
{
    xsnprintf(commandOutBuffer,EMAC_ETH_MAX_FLEN,"CMD?\n");
    messageReady = 1u;
}

void printParameterMissing(void)
{
    xsnprintf(commandOutBuffer,EMAC_ETH_MAX_FLEN,"ERR: 2few Args\n");
    messageReady = 1u;
}

void printAcknowledgement(void)
{
    xsnprintf(commandOutBuffer,EMAC_ETH_MAX_FLEN,"ACK\n");
    messageReady = 1u;
}

void printError(char *message)
{
    xsnprintf(commandOutBuffer,EMAC_ETH_MAX_FLEN, "ERR: %s\n", message);
    messageReady = 1u;
}

bool compareBaseCommand(char *original, char *received)
{
    return (strcmp(original,received) == (int)(0));
}

bool compareExtendedCommand(char *original, char *received)
{
    return (((strlen(received) == 1u) 
            && (strncmp(original,received,1u) == (int)(0))) 
            || (strcmp(original,received) == (int)(0)));
}

void commandProcess(char *data, uint16 dataLength)
{
    static char receivedData;
    uint16 i;
    
    if (connectionState == ConnectionState_Offline)
    {
        xsnprintf(commandOutBuffer,EMAC_ETH_MAX_FLEN,"Enter user:\n");
        messageReady = 1u;
        connectionState = ConnectionState_User;
        return;
    }
    
    for (i = 0u; i < dataLength; i++)
    {
        receivedData = data[i];
        if ((receivedData != '\n')
        )
        {
            commandInBuffer[commandInBufferPos] = receivedData;
            if (commandInBufferPos < (EMAC_ETH_MAX_FLEN-1u))
            {
                commandInBufferPos++;
            }
            else
            {
                //(*errorFunctionPointer1)();
                commandInBufferPos = 0u;
            }
        }
        else
        {
        	commandInBufferPos--;   // -1 to delate /r from command
            commandInBuffer[commandInBufferPos] = '\0';
            
            if (connectionState == ConnectionState_User)
            {
                if (strcmp(commandInBuffer, user) == 0)
                {
                    xsnprintf(commandOutBuffer,EMAC_ETH_MAX_FLEN,"Enter pass:\n");
                    messageReady = 1u;
                    connectionState = ConnectionState_Pass;
                }
                else
                {
                    xsnprintf(commandOutBuffer,EMAC_ETH_MAX_FLEN,"Wrong user, enter again:\n");
                    messageReady = 1u;
                }
            }
            else if (connectionState == ConnectionState_Pass)
            {
                if (strcmp(commandInBuffer, pass) == (int)0)
                {
                    xsnprintf(commandOutBuffer,EMAC_ETH_MAX_FLEN,"You are logged in:\n");
                    messageReady = 1u;
                    connectionState = ConnectionState_Online;
                }
                else
                {
                    xsnprintf(commandOutBuffer,EMAC_ETH_MAX_FLEN,"Wrong pass, enter again:\n");
                    messageReady = 1u;
                }
            }
            else
            {
                processCommand(commandInBuffer);
            }
            commandInBufferPos = 0u;
        }
    }
}

void processCommand(char *buffer)
{
    char *dataPointer;
    char *savePointer;
    int64 value;
    
    if (strlen(buffer) == 0u)
    {
        return;
    }
    dataPointer = strtok_r(buffer, " ", &savePointer);
    
    if (compareBaseCommand("ping", dataPointer))
    {
        dataPointer = strtok_r(NULL, " ", &savePointer);
        if (dataPointer == NULL)
        {
            printParameterMissing();
        }
        else if (compareBaseCommand("-c",dataPointer))
        {
            dataPointer = strtok_r(NULL, " ", &savePointer);
            if (dataPointer == NULL)
            {
                printParameterMissing();
            }
            else
            {
                if (xatoi(&dataPointer, &value) == 1u)
                {
                    if (dataPointer == NULL)
                    {
                        printParameterMissing();
                    }
                    else
                    {
                        replyCounter = 0u;
                        if (pingHost((uint8)value, dataPointer) == 1u)
                        {
                            xsnprintf(commandOutBuffer,EMAC_ETH_MAX_FLEN,"Replies: \n", replyCounter);
                            messageReady = 1u;
                        }
                        else
                        {
                            printError("wrong address");
                        }
                    }
                }
                else
                {
                    printError("param wrong");
                }
            }
            xsnprintf(commandOutBuffer,EMAC_ETH_MAX_FLEN,"no Error cause no statistic\n");
            messageReady = 1u;
        }
        else
        {
            replyCounter = 0u;
            pingHost(4u, dataPointer);
            xsnprintf(commandOutBuffer,EMAC_ETH_MAX_FLEN,"Replies: \n", replyCounter);
            messageReady = 1u;
            //printUnknownCommand();
        }
    }
    else if (compareBaseCommand("7seg", dataPointer))
    {
        uint8 params;
        uint8 sevenValue;
        uint8 point;
        
        params = 0u;
        sevenValue = 0u;
        point = 0u;
        
        while(1u)
        {
            dataPointer = strtok_r(NULL, " ", &savePointer);
            if (dataPointer == NULL)
            {
                break;
            }
            else if (compareBaseCommand("-d",dataPointer))
            {
                dataPointer = strtok_r(NULL, " ", &savePointer);
                if (dataPointer == NULL)
                {
                    printParameterMissing();
                }
                else
                {
                    if (xatoi(&dataPointer, &value) == 1u)
                    {
                        sevenValue = (uint8)value;
                        params++;
                    }
                    else
                    {
                        printError("param wrong");
                        return;
                    }
                }
            }
            else if (compareBaseCommand("-p",dataPointer))
            {
                dataPointer = strtok_r(NULL, " ", &savePointer);
                if (dataPointer == NULL)
                {
                    printParameterMissing();
                }
                else
                {
                    if (xatoi(&dataPointer, &value) == 1u)
                    {
                        point = (uint8)value;
                        params++;
                    }
                    else
                    {
                        printError("param wrong");
                        return;
                    }
                }
            }
            else
            {
                 printError("wrong param");
                 return;
            }
        }
        
        if (params == 0u)
        {
            printParameterMissing();
        }
        else
        {
            setSevenSegment(sevenValue, point);
            printAcknowledgement();
        }
    }
    else if (compareBaseCommand("arp", dataPointer))
    {
        uint8 params;
        char *broadcastAddress;
        char *clearAddress;
        
        params = 0u;
        broadcastAddress = NULL;
        clearAddress = NULL;
        
        while(1u)
        {
            dataPointer = strtok_r(NULL, " ", &savePointer);
            if (dataPointer == NULL)
            {
                break;
            }
            else if (compareBaseCommand("-a",dataPointer))
            {
                dataPointer = strtok_r(NULL, " ", &savePointer);
                if (dataPointer == NULL)
                {
                    printParameterMissing();
                }
                else
                {
                    broadcastAddress = dataPointer;
                    params++;
                }
            }
            else if (compareBaseCommand("-d",dataPointer))
            {
                dataPointer = strtok_r(NULL, " ", &savePointer);
                if (dataPointer == NULL)
                {
                    printParameterMissing();
                }
                else
                {
                    clearAddress = dataPointer;
                    params++;
                }
            }
            else
            {
                 printError("wrong param");
                 return;
            }
        }
        
        if (params == 0u)
        {
            printParameterMissing();
        }
        else
        {
            if (doArp(broadcastAddress, clearAddress) == 1u)
            {
                printAcknowledgement();
            }
            else
            {
                printError("wrong Input");
            }
        }
    }
    else if (compareBaseCommand("logout", dataPointer))
    {
        dataPointer = strtok_r(NULL, " ", &savePointer);
        if (dataPointer == NULL)
        {
            xsnprintf(commandOutBuffer,EMAC_ETH_MAX_FLEN,"you are logged out\n");
            messageReady = 1u;
            //SOCKET_close(1);
            connectionState = ConnectionState_Offline;
        }
        else
        {
            printUnknownCommand();
        }
    }
    else
    {
        printUnknownCommand();
    }
}

uint8 doArp(char *broadcastAddress, char *clearAddress)
{
    int64 value;
    char *savePointer;
    char *dataPointer;
    uint8 pos;
    MAC_A address;
    IP_A  ipAddress;
    
    if (clearAddress != NULL)
    {
        pos = 0u;
        dataPointer = strtok_r(clearAddress, ":", &savePointer);
        while (dataPointer != NULL)
        {
            
            if (dataPointer != NULL)
            {
                if (pos < 6u)
                {
                    address.b[pos] = (uint8)Generic_hex2int(dataPointer,2u);;
                    pos++;
                }
                else
                {
                    return 0u;
                }
            }
            dataPointer = strtok_r(NULL, ":", &savePointer);
        }
        
        if (pos == 6u)
        {
            ARP_TableClearEntry(address);
        }
        else
        {
            return 0u;
        }
    }
    
    if (broadcastAddress != NULL)
    {
        pos = 0u;
        dataPointer = strtok_r(clearAddress, ".", &savePointer);
        while (dataPointer != NULL)
        {
            
            if (dataPointer != NULL)
            {
                if ((pos < 4u) || (xatoi(&dataPointer, &value) == 1u))
                {
                    ipAddress.b[pos] = (uint8)value;
                    pos++;
                }
                else
                {
                    return 0u;
                }
            }
            dataPointer = strtok_r(NULL, ".", &savePointer);
        }
        if (pos == 4u)
        {
            ARP_SendEchoRequest(ipAddress);
        }
        else
        {
            return 0u;
        }
    }
    
    
    ARP_TablePrint();
    
    return 1u;
}

uint8 pingHost(uint8 count, char *address)
{
    uint8 i;
    uint8 pos;
    int64 value;
    INT16U ICMPdestlen;
    ICMP_HEADER  *pICMPdest;
    OS_ERR ErrVar;
    IP_A targetAddress;
    char *dataPointer;
    char *savePointer;
    
    dataPointer = strtok_r(address, ".", &savePointer);
    while (dataPointer != NULL)
    {
        
        if (dataPointer != NULL)
        {
            if ((pos < 4u) || (xatoi(&dataPointer, &value) == 1u))
            {
                targetAddress.b[pos] = (uint8)value;
                pos++;
            }
            else
            {
                return 0u;
            }
        }
        dataPointer = strtok_r(NULL, ".", &savePointer);
    }
    if (pos != 4u)
    {
        return 0u;
    }
    
    for (i = 0u; i< count; i++)
    {
        pICMPdest = (ICMP_HEADER  *)OSMemGet(&PacketMemArea, &ErrVar);// get memory block from Lan transmit partition
        if ( pICMPdest == NULL )
        {
            return;
        }
        IP_ICMPPing((ICMP_HEADER  *)pICMPdest,// destination ICMP packet (ardy allocated)
                    &ICMPdestlen);
        IP_SendDatagram (targetAddress,                 // address where to send
                        1,                             // message type: ICMP Echo reply
                        123456u,                     // datagram identification
                        0,                             // do not fragment
                        (INT16U  *)pICMPdest,      // ICMP to be sent, allocated
                        ICMPdestlen,                   // ICMP total length
                        (void *)pICMPdest );           // pointer to dynamic memory block
        OSTimeDlyHMSM(0,0,0,500,OS_OPT_TIME_HMSM_STRICT,&ErrVar);
    }
    
    return 1u;
}

void setSevenSegment(uint8 num, uint8 point)
{
    uint8 segmentCode;
    
    if (num > 15u)
    {
        return;
    }
    
    segmentCode = segmentValue[num];
    if (point == 1u)
    {
        segmentCode |= 0b00100000;
    }
    
    Ssp_write(Ssp1, segmentSelId, ~segmentCode);    //negated
}

void HardFault_Handler()
{
    /*GPIO_SetValue(1,(1<<29));*/
    //asm("ldr  r3, __cs3_reset");
    asm volatile("ldr  r0, =__cs3_reset\n"
                 "bx   r0");
}
