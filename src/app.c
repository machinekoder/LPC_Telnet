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

const static char user[] = "admin\r";
const static char pass[] = "pwd\r";
static ConnectionState connectionState;



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

static void commandProcess(char *data, uint16 dataLength); 
static void processCommand(char *data);


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
  EMAC_ConfigStruct.pbEMAC_Addr = eth_mac;

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
                            (INT16U *)commandOutBuffer, strlen(commandOutBuffer));// sends the same data back to sender (TCP echo)
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
            //(*taskFunctionPointer1)(taskBuffer1);
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
            printUnknownCommand();
        }
        else if (compareExtendedCommand("-c",dataPointer))
        {
        	xsnprintf(commandOutBuffer,EMAC_ETH_MAX_FLEN,"no Error cause no statistic\n");
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

void HardFault_Handler()
{
    /*GPIO_SetValue(1,(1<<29));
    //asm("ldr  r3, __cs3_reset");
    asm volatile("ldr  r0, main\n"
                 "bx   r0");*/
}
