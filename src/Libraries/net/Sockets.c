// --------------------------------------------------------------------------------------------- 
// PROJECT:     TCP Stack for Keil MCB167-NET
// UNIT:        SOCKETS.C
// PURPOSE:     Interface Unit to higher layers, provides standardized socket interface
// AUTHOR:      Martin Zauner, Matr.Nr.9911011114
// COPYRIGHT:   (c)2003, Fachhochschule Technikum Wien, Modul Telekom und 
//              Computer & Systemtechnik
// LAST UPDATE: 24-04-2003
// --------------------------------------------------------------------------------------------- 

// --------------------------------------------------------------------------------------------- 
// includes
// --------------------------------------------------------------------------------------------- 

#include "net_includes.h"

extern IP_A my_ip;
extern OS_MEM PacketMemArea;

// --------------------------------------------------------------------------------------------- 
// externals
// --------------------------------------------------------------------------------------------- 

// --------------------------------------------------------------------------------------------- 
//  socket variable
// --------------------------------------------------------------------------------------------- 

tcp_socket Socket[MAX_SOCKET_NUMBERS];     // maximum possible sockets

// --------------------------------------------------------------------------------------------- 
// function:    int SOCKET_bind (int socketno, int ownport) 
// parameters:  <socketno>    socket number
//              <ownport>     port of socket
// result:      int 0         socket available
//              < 0           bind to socket failed
// purpose:             binds a valid socket to own port/IP address. Socket must 
//                      be available (use SOCKET_NewSocket) prior to binding. 
//                      If OK, the new status is CLOSED.
// uses central:NO
// --------------------------------------------------------------------------------------------- 

int SOCKET_bind (int socketno, int ownport) {

if ((socketno < 0) | (socketno > MAX_SOCKET_NUMBERS))
{
 return(SOCKERR_PARAM_RANGE);                         // socket number out of range
}
if (Socket[socketno].SockState != TCP_STATE_UNBOUND)  // error, socket must be unbound
{
 return(SOCKERR_WRONG_STATE);
}
 // -- set socket parameters --
Socket[socketno].OwnPort= ownport;
memcpy(&Socket[socketno].OwnIP, (INT16U *)&my_ip.d,// copy own IP address into record
        IP_LEN);
Socket[socketno].SockState = TCP_STATE_CLOSED;                 // now ready-for-use state

return(SOCKERR_RESULT_OK);                                     // init successful
}

// --------------------------------------------------------------------------------------------- 
// function:    int SOCKET_close (int socketno)
// parameters:  <socketno>     number of socket
// result:      int 0          socket will now be closed
//              <0             cannot close from current state
// purpose:             closes the socket, state will change to CLOSED
// uses central:YES
// --------------------------------------------------------------------------------------------- 

int SOCKET_close (int socketno) {
int FuncResult;

if ((socketno < 0) | (socketno > MAX_SOCKET_NUMBERS))
{
 return(SOCKERR_PARAM_RANGE);                               // socket number out of range	
}
if ((Socket[socketno].SockState == TCP_STATE_UNAVAILABLE) ||// cannot close in this mode
  (Socket[socketno].SockState == TCP_STATE_UNBOUND)       ||// cannot close in this mode
  (Socket[socketno].SockState == TCP_STATE_CLOSED)        ||// cannot close in this mode
  (Socket[socketno].SockState == TCP_STATE_FINWAIT1)      ||// cannot close in this mode
  (Socket[socketno].SockState == TCP_STATE_FINWAIT2)      ||// cannot close in this mode
  (Socket[socketno].SockState == TCP_STATE_CLOSING)       ||// cannot close in this mode
  (Socket[socketno].SockState == TCP_STATE_TIMEDWAIT)     ||// cannot close in this mode
  (Socket[socketno].SockState == TCP_STATE_LASTACK))        // cannot close in this mode
{
 return (SOCKERR_WRONG_STATE);
}
 // -- call central procedure of state machine --
FuncResult = TCPSTATE_Central (socketno,
             Socket[socketno].ForeignIP.d,   // may be undefined !
             Socket[socketno].ForeignPort,   // may be undefined !
             Socket[socketno].OwnPort,       // may be undefined !
             Socket[socketno].SEQrcvd,       // SEQnum (ignore)
             Socket[socketno].ACKrcvd,       // ACKnum (ignore)
             TCP_SIG_NONE,                   // incoming flags
             TCP_CONN_CLOSE,                 // highlevel socket command
             -1,                             // incoming max. window size demanded
             -1,                             // max. segment size
             NULL,                           // data pointer
             0);                             // bytes to be read
return (FuncResult);
}

// --------------------------------------------------------------------------------------------- 
// function:    int SOCKET_connect (int socketno, INT32U foreignaddress, int foreignport) 
// parameters:  <socketno>        number of socket
//              <foreignaddress>  address where to send the message
//              <foreignport>     port where to send
// result:      int  0            successfully sent
//                  <0            invalid state to connect
// purpose:                 launches a connect request
// uses central:YES
// --------------------------------------------------------------------------------------------- 

int SOCKET_connect (int socketno, INT32U foreignaddress, int foreignport) {
int FuncResult;
int Command;

if ((socketno < 0) | (socketno > MAX_SOCKET_NUMBERS))
{
 return(SOCKERR_PARAM_RANGE);                         // socket number out of range
}
if ((Socket[socketno].SockState != TCP_STATE_CLOSED)  // error, socket must be closed or
 && (Socket[socketno].SockState != TCP_STATE_LISTEN)) // listening
{
 return (SOCKERR_WRONG_STATE);
}
 // -- set socket parameters --
Socket[socketno].ForeignPort = foreignport;           // target port
Socket[socketno].ForeignIP.d = foreignaddress;        // target IP to connect
 // -- call central procedure of state machine --
if (Socket[socketno].SockState != TCP_STATE_CLOSED)
    Command = TCP_CONN_SEND;                          // CONNECT from CLOSED stata		
else 
    Command = TCP_CONN_CONNECT;                       // OPEN from LISTEN state

FuncResult  = TCPSTATE_Central (socketno,
                Socket[socketno].ForeignIP.d,         // may be undefined !
                Socket[socketno].ForeignPort,         // may be undefined !
                Socket[socketno].OwnPort,             // may be undefined !
                0,                                    // SEQnum (ignore)
                0,                                    // ACKnum (ignore)
                TCP_SIG_NONE,                         // incoming flags
                Command,                              // highlevel socket command
                -1,                                   // incoming max. window size demanded
                -1,                                   // max. segment size
                NULL,                                 // data pointer
                0);                                   // bytes to be read
return (FuncResult);              // return TCPSTATE_Central() result
}


// --------------------------------------------------------------------------------------------- 
// function:    int SOCKET_listen (int socketno)
// parameters:  <socketno>    number of socket
// result:       0            successfully sent listen command
//              <0            invalid state to listen
// purpose:               moves to LISTEN state if currently in CLOSED state
// uses central:NO
// --------------------------------------------------------------------------------------------- 

int SOCKET_listen (int socketno) {

if ((socketno < 0) | (socketno > MAX_SOCKET_NUMBERS))
{
 return(SOCKERR_PARAM_RANGE);                       // socket number out of range
}
if (Socket[socketno].SockState != TCP_STATE_CLOSED) // error, socket must be closed
{
 return (SOCKERR_WRONG_STATE);
}
Socket[socketno].SockState = TCP_STATE_LISTEN;      // immediately change to listen
SOCKET_SetForeignIpandPortZero (socketno);

return(SOCKERR_RESULT_OK);                          // operation successful
}


// --------------------------------------------------------------------------------------------- 
// function:    int SOCKET_read (int socketno, byte  *readbuffer, int length)
// parameters:  <socketno>         socket ID
//              <*readbuffer>      pointer to returned data
//              <length>           nominal number of bytes to be read
//                                 (must be < RxLen, determined using <SOCKET_RxBytes>)
// result:       0                 successfully read
//              <0                 socket read error, eg. invalid state to read or requested 
//                                 block size too large
// purpose:                  reads socket read buffer (LIFO organized)
// uses central:NO
// --------------------------------------------------------------------------------------------- 

int SOCKET_read (int socketno, INT16U *readbuffer, int length) {
int BlockStart;

 // --- parameter check ---
if ((length <= 0) | (length > BUF_RxMAXWORDS)) 
 return (SOCKERR_PARAM_RANGE);                  // invalid number of bytes to read
if (readbuffer == NULL)
 return (SOCKERR_NULL_PTR);                     // cannot copy into NULL pointer

if (Socket[socketno].RxLen == 0)
{
 return (SOCKERR_BUFFER_EMPTY);                 // buffer is empty!
}
// determine blockread origin
if (length <= Socket[socketno].RxLen)           // read less or equal nr of bytes only
{
 BlockStart = Socket[socketno].RxLen - length;  // begin for reading
 memcpy((INT16U  *)readbuffer,              // copy to *readbuffer
         (INT16U  *)(Socket[socketno].RxBuffer+BlockStart),//[BlockStart],
                                                // from socket RxBuffer[BlockStart]/////
          length);                              // "length" bytes to be read 
 Socket[socketno].RxLen -= length;              // read out, decrement buffer length
}
else
{ 
 return (SOCKERR_READ_TOOMUCH);                 // requested amount of bytes is too large
}
return(SOCKERR_RESULT_OK);                      // operation successful
}


// --------------------------------------------------------------------------------------------- 
// function:    int SOCKET_newsocket (int socketno)
// parameters:  <socketno>       socketID
// result:       0               socket now ready for binding
//              <0               cannot create socket, socket is ardy in use
// purpose:               changes UNAVAILABLE state to UNBOUND state, there are no 
//                        special conditions to fulfil.
// uses central:NO
// --------------------------------------------------------------------------------------------- 

int SOCKET_newsocket (int socketno) {

if (Socket[socketno].SockState != TCP_STATE_UNAVAILABLE)// error, socket must be unavailable
{
 return (SOCKERR_WRONG_STATE);
}
 // -- set socket parameters --
Socket[socketno].SockState = TCP_STATE_UNBOUND;         // now ready for binding

return(SOCKERR_RESULT_OK);                              // init successful
}

// --------------------------------------------------------------------------------------------- 
// function:    int SOCKET_getsocket ()
// parameters:  <none>
// result:      >=0        socket now ready for binding
//              <0         cannot create socket, no socket is available
// purpose:         get a free socker number by using the function
//                  SOCKET_newsocket() and return it, if no socket is
//                  available return error code -20(SOCKERR_PORTS_EMPTY)
// uses central:NO
// --------------------------------------------------------------------------------------------- 

int SOCKET_getsocket()
{
  int socketno;                            // number of free socket
  OS_ERR Err;

  for (socketno = 0; socketno < MAX_SOCKET_NUMBERS; socketno++)
  {                                        // run throw socket buffer pool
    if(SOCKET_newsocket(socketno) == SOCKERR_RESULT_OK) 
    {                                      // see if and which socket is available
    	/*TODO: change PacketMemArea?*/
      Socket[socketno].RxBuffer = (INT8U *)OSMemGet(&PacketMemArea, &Err);
      /*TODO: change PacketMemArea?*/
      Socket[socketno].TxBuffer = (INT8U *)OSMemGet(&PacketMemArea, &Err);
      return socketno;                     // return available socket number
    }
  }
  return SOCKERR_PORTS_EMPTY;              // return error code if all socket are in use
}
// --------------------------------------------------------------------------------------------- 
// function:     void SOCKET_initonesock (int socketno) 
// parameters:   <socketno>       number of socket
// result:       <none>
// purpose:                sets default values for one single socket structure
// uses central:NO
// --------------------------------------------------------------------------------------------- 

void SOCKET_initonesock (int socketno) {

Socket[socketno].OwnPort          = 0;           // not specified
Socket[socketno].ForeignPort      = 0;           // unknown
Socket[socketno].ForeignIP.d      = 0;           // no partner determined
Socket[socketno].EventProcedure   = NULL;        // procedure not defined
Socket[socketno].SegmentMaxLength = 1000;        // default value in bytes	
Socket[socketno].SockState        = TCP_STATE_UNAVAILABLE; // socket not available
Socket[socketno].ACKsent          = 0;           // default
Socket[socketno].SEQsent          = 0;           // default
Socket[socketno].ACKrcvd          = 0;           // default
Socket[socketno].SEQrcvd          = 0;           // default
Socket[socketno].TxConfirmed      = TRUE;        // TRUE
Socket[socketno].TxLen            = 0;           // empty
Socket[socketno].RxLen            = 0;           // empty
Socket[socketno].FlagsReceived    = TCP_SIG_NONE;// no signal received
 // Timer init values
Socket[socketno].TimeToRetransmission = TIMERS_TIMETORETRANS;// time until last segment is re-sent
Socket[socketno].RetransCounter       = TIMERS_RETRYNUM;     // default value for retry send
Socket[socketno].TimeToRetransSyn     = TIMERS_TIMETORETRANSSYN;// time until SYN may be retransmitted
Socket[socketno].RetransSYNCounter    = TIMERS_SYNRETRYNUM;// default value for retry send SYN
}

// --------------------------------------------------------------------------------------------- 
// function:    int SOCKET_freesocket (int socketno)
// parameters:  <socketno>     socket ID
// result:       0             successfully removed
//              <0             cannot free a socket which is in operation (not CLOSED)
// purpose:               frees a closed socket and makes it unavailable
// uses central:NO
// --------------------------------------------------------------------------------------------- 

int SOCKET_freesocket (int socketno) {
	OS_ERR     ErrVar;

if ((socketno < 0) | (socketno > MAX_SOCKET_NUMBERS))
{
 return(SOCKERR_PARAM_RANGE);                       // socket number out of range		
} 
if (Socket[socketno].SockState != TCP_STATE_CLOSED) // error, socket must be closed first
{
 return (SOCKERR_WRONG_STATE);
}
 // -- set socket parameters --
OSMemPut(&PacketMemArea, Socket[socketno].RxBuffer,&ErrVar);
if(ErrVar != OS_ERR_NONE)
	for(;;);
OSMemPut(&PacketMemArea, Socket[socketno].TxBuffer,&ErrVar);
if(ErrVar != OS_ERR_NONE)
	for(;;);
Socket[socketno].SockState = TCP_STATE_UNAVAILABLE; // unbound and made unavailable
SOCKET_initonesock (socketno);                      // default settings to variables

return(SOCKERR_RESULT_OK);                          // success
}

// --------------------------------------------------------------------------------------------- 
// function:    int SOCKET_write (int socketno, byte  *readbuffer, int length)
// parameters:  <socketno>      socket ID
//              <*writebuffer>  pointer to data 
//              <length>        data bytes to be written
// result:       0              successfully written
//              <0              socket write error, eg. invalid state to write or buffer full
// purpose:                writes data into socket 
// uses central:YES
// --------------------------------------------------------------------------------------------- 

int SOCKET_write (int socketno, INT16U *writebuffer, int length) {
int FuncResult;
int RemainingBytes;
int BlockStart;

if ((socketno < 0) | (socketno > MAX_SOCKET_NUMBERS))
{
 return(SOCKERR_PARAM_RANGE);                             // socket number out of range
}
 // --- parameter check ---
RemainingBytes = BYTESTOSEND - Socket[socketno].TxLen;    // free space in transmit location

if (length <= 0) return (SOCKERR_PARAM_RANGE);            // invalid number of bytes to write
if (length > RemainingBytes) return (SOCKERR_BUFFER_FULL);// not enough buffer space
if (writebuffer == NULL) return (SOCKERR_NULL_PTR);       // can't read data from NULL pointer

if (Socket[socketno].SockState != TCP_STATE_ESTABLISHED)// cannot write in unestablished state
{
 return(SOCKERR_WRONG_STATE);                           // wrong state to send payload
}
// now append data to end of transmit stack
BlockStart = Socket[socketno].TxLen;                    // position to append data
 
memcpy((INT16U  *)(Socket[socketno].TxBuffer+BlockStart),//[BlockStart],// target: transmit buffer/////
        (INT16U  *)&writebuffer[0],                       // copy from *writebuffer
         length);                                            // "length" bytes to write
Socket[socketno].TxLen += length;                            // new length, increased

 // --- call state machine central procedure (SEND TxBUFFER) ---
FuncResult = TCPSTATE_Central (socketno,
               Socket[socketno].ForeignIP.d,        // may be undefined !
               Socket[socketno].ForeignPort,        // may be undefined !
               Socket[socketno].OwnPort,            // may be undefined !
               0,                                   // SEQnum (ignore)
               0,                                   // ACKnum (ignore)
               TCP_SIG_NONE,                        // incoming flags
               TCP_CONN_NONE,                       // highlevel socket command
               -1,                                  // incoming max. window size demanded
               -1,                                  // max. segment size
               NULL,                                // data pointer
               0);                                  // bytes to be read
                                                    // (retransmit timer uses the same method)
return (FuncResult);      // return TCPSTATE_Central() result
}

// --------------------------------------------------------------------------------------------- 
// function:     int SOCKET_status (int socketno)
// parameters:   <socketno>      socket ID (0..9)
// result:       int             socket status = [CLOSED, SYNSEND, LISTEN, SYNRECEIVED, ....]
// purpose:                 returns the status of the socket
// uses central:NO
// --------------------------------------------------------------------------------------------- 

int SOCKET_status (int socketno) {

if ((socketno < 0) | (socketno > MAX_SOCKET_NUMBERS))
{
 return(SOCKERR_PARAM_RANGE);         // socket number out of range
}
return (Socket[socketno].SockState);  // return state
}

// --------------------------------------------------------------------------------------------- 
// function:    int SOCKET_RxBytes (int socketno)
// parameters:  <socketno>      socket ID (0..9)
// result:      int             bytes in RxBuffer[socketno] for read
// purpose:               returns the data bytes available in RxBuffer
// uses central:NO
// --------------------------------------------------------------------------------------------- 

int SOCKET_RxBytes (int socketno) {

if ((socketno < 0) | (socketno > MAX_SOCKET_NUMBERS))
{
 return(SOCKERR_PARAM_RANGE);        // socket number out of range
}
return (Socket[socketno].RxLen);
}

// --------------------------------------------------------------------------------------------- 
// function:    int SOCKET_TxBytesFree (int socketno)
// parameters:  <socketno>      socket ID (0..9)
// result:      int             bytes in TxBuffer[socketno] free for sending
// purpose:                 returns free data bytes in TxBuffer
// uses central:NO
// --------------------------------------------------------------------------------------------- 

int SOCKET_TxBytesFree (int socketno) {
int Bytesfree;

if ((socketno < 0) | (socketno > MAX_SOCKET_NUMBERS))
{
 return(SOCKERR_PARAM_RANGE);                    // socket number out of range
}
Bytesfree = BYTESTOSEND - Socket[socketno].TxLen;// free bytes in transmit buffer

return (Bytesfree);                              // "0" means TxBuffer is full
}

// --------------------------------------------------------------------------------------------- 
// function:    int SOCKET_RxBytesFree (int socketno)
// parameters:  <socketno>     socket ID (0..9)
// result:      int            bytes in RxBuffer[socketno] free for receiving
// purpose:               returns free data bytes in RxBuffer
// uses central:NO
// --------------------------------------------------------------------------------------------- 

int SOCKET_RxBytesFree (int socketno) {
int Bytesfree;

if ((socketno < 0) | (socketno > MAX_SOCKET_NUMBERS))
{
 return(SOCKERR_PARAM_RANGE);                        // socket number out of range
}
Bytesfree = BUF_RxMAXBYTES - Socket[socketno].RxLen; // free bytes to move in

return (Bytesfree);                                  // "0" means RxBuffer is full
}

// --------------------------------------------------------------------------------------------- 
// function:     int SOCKET_seteventproc (int socketno, procref EventProc) 
// parameters:   <socketno>        socket ID
//               <EventProcedure>  address of procedure to be launched after receiving a data 
//                                 containing TCP segment.
// result:        0                procedure successfully linked
//               <0                unspecified error
// purpose:                 sets the procedure <EventProcedure> to be called after 
//                          receiving TCP payload
// uses central:NO
// --------------------------------------------------------------------------------------------- 

int SOCKET_seteventproc (int socketno, procref EventProc) {

if ((socketno < 0) | (socketno > MAX_SOCKET_NUMBERS))
{
 return(SOCKERR_PARAM_RANGE);                 // socket number out of range
}
Socket[socketno].EventProcedure = EventProc;  // set event procedure

return(SOCKERR_RESULT_OK);                    // init successful
}

// --------------------------------------------------------------------------------------------- 
// procedure:   void SOCKET_allinit (void)
// parameters:  <none>
// result:      <none>
// purpose:            init procedure, sets all sockets to unavailable.
// uses central:NO
// --------------------------------------------------------------------------------------------- 

void SOCKET_allinit (void) {
INT16U a;
  for (a=0; a<MAX_SOCKET_NUMBERS; a++) 
  {
    SOCKET_initonesock (a);         // defaults for all sockets
  }
}

// --------------------------------------------------------------------------------------------- 
// function:    int SOCKET_usedSocketno (INT16U portno, INT32U ipaddr, INT16U foreignport)
// parameters:  <portno>            number of port
//              <ipaddr>            foreign ip address
//              <foreignport>       forein port
// returns:     int     [0..9]      number of socket which uses portno
//                      <0          if failed
// purpose:               finds out, which socket is dedicated to the given port, 
//                        foreign IP and foreign port  
// uses central:NO
// --------------------------------------------------------------------------------------------- 

int SOCKET_usedSocketno (INT16U portno, INT32U ipaddr, INT16U foreignport) 
{
  INT16U socketno;

  for (socketno=0; socketno<MAX_SOCKET_NUMBERS; socketno++) 
  {  
    if ( (Socket[socketno].OwnPort     == portno) &&    // OK, this socket uses the port
       (Socket[socketno].ForeignIP.d == ipaddr) &&      // and the foreign IP address
       (Socket[socketno].ForeignPort == foreignport) )  // and the foreign port
    {  
     return socketno;                                   // result is number of socket 
    }
  }
  for (socketno=0; socketno<MAX_SOCKET_NUMBERS; socketno++) 
  {  
    if ( (Socket[socketno].OwnPort == portno)  &&       // which socket uses portno
         (Socket[socketno].ForeignIP.d == 0)   &&       // and the foreign IP address
         (Socket[socketno].ForeignPort == 0)   &&       // and the foreign port
     (Socket[socketno].SockState == TCP_STATE_LISTEN))  // current state listen?
    {
       return socketno;                                 // result is number of socket
    }
  }
  return (SOCKERR_PORT_UNUSED);                // return socket number or error valvue
}

// --------------------------------------------------------------------------------------------- 
// function:    int SOCKET_usesport (INT16U portno)
// parameters:  <portno>          number of port
// returns:     int    [0..9]     number of socket which uses portno
//                     <0         if failed
// purpose:                 finds out, which socket is dedicated to the given port 
// uses central:NO
// --------------------------------------------------------------------------------------------- 

int SOCKET_usesport (INT16U portno) {
INT16U a;

for (a=0; a<MAX_SOCKET_NUMBERS; a++) 
{  
if (Socket[a].OwnPort == portno)     // OK, this socket uses the port
    return (a);                      // result is number of socket
}
return (SOCKERR_PORT_UNUSED);        // no socket found
}

// --------------------------------------------------------------------------------------------- 
// function:    int SOCKET_receive (int socketno, INT16U  *Data, INT16U Datalen) 
// parameters:  <socketno>      number of socket
//              <*Data>         pointer to payload
//              <Datalen>       number of bytes to receive
// returns:     int    0        successful
//                    <0        socket error occured while reading 
// purpose:               main socket receive function, moves TCP payload into read 
//                        buffer, then calls the central function of state machine.
// uses central:YES
// --------------------------------------------------------------------------------------------- 

int SOCKET_receive (int socketno, INT16U *Data, INT16U Datalen) {

int WriteInPos,           // position where to write in
    Freespace;            // free space in RxBuffer

 // --- input parameter check ---
if  (Data == NULL) return(SOCKERR_NULL_PTR);     // cannot read from NULL pointer
if  (Datalen == 0) return(SOCKERR_NULL_PTR);     // cannot read 0 bytes
 // --- Socket state control ---
if (Socket[socketno].SockState != TCP_STATE_ESTABLISHED) // cannot read in unestablished state
{
 return(SOCKERR_WRONG_STATE);                    // wrong state to receive payload
}
 // -- check free space for receiving --
Freespace = SOCKET_RxBytesFree (socketno);       // free bytes to move in
if (Freespace < Datalen) return (SOCKERR_BUFFER_FULL);// cannot read segment, RxBuffer full.

WriteInPos 	= Socket[socketno].RxLen;            // Position to append
memcpy((INT16U  *)(Socket[socketno].RxBuffer+WriteInPos),//[WriteInPos],// to socket RxBuffer/////
        (INT16U  *)&Data[0],                 // from source pointer
         Datalen);                               // "length" bytes to be write
Socket[socketno].RxLen += Datalen;               // new length, increased

return(SOCKERR_RESULT_OK);                       // alright
}

// --------------------------------------------------------------------------------------------- 
// function:    int SOCKET_launcheventproc (INT16U socketno)
// parameters:  <socketno>     number of socket
// returns:     int   0        event procedure successfully launched
//                   <0        undefined error eg. socketno not in range
// purpose:                    launches socket event procedure
// uses central:NO
// --------------------------------------------------------------------------------------------- 

int SOCKET_launcheventproc (INT16U socketno) {

if ((socketno < 0) | (socketno > MAX_SOCKET_NUMBERS))
{
 return(SOCKERR_PARAM_RANGE);                 // socket number out of range		
}
// --- now call event procedure
if (Socket[socketno].EventProcedure != NULL)
{
 Socket[socketno].EventProcedure ();          // call user definable procedure if not NULL
}
else 
{
 return(SOCKERR_EVENTPROC_NULL);              // event proc is undefined
}
 // --- return OK code ---
return(SOCKERR_RESULT_OK);                    // alright, launched
}

// --------------------------------------------------------------------------------------------- 
// function:    void SOCKET_refresh (void) 
// parameters:  <none>
// returns:     <none>
// purpose:            frequently called for re-transmission of segments. The central
//                     state machine function will launch a new segment if the send 
//                     buffer contains some bytes to send or a control message must
//                     be transmitted.
// uses central:YES
// --------------------------------------------------------------------------------------------- 

void SOCKET_refresh (void) 
{
  int socketno;
  
  for (socketno=0; socketno<MAX_SOCKET_NUMBERS; socketno++) 
  {
    if (Socket[socketno].SockState > TCP_STATE_UNBOUND)    // check if valid state
    {
      if ( (Socket[socketno].TimeToRetransmission == 0) && // check if time to retransmit
           (Socket[socketno].RetransCounter > 0) )         // retransmission counter not zero
      {
        TCPSTATE_Central (socketno,
           Socket[socketno].ForeignIP.d,     // may be undefined !
           Socket[socketno].ForeignPort,     // may be undefined !
           Socket[socketno].OwnPort,         // may be undefined !
           0,                                // incoming SEQ value
           0,                                // incoming ACK value
           TCP_SIG_NONE,                     // incoming flags
           TCP_CONN_NONE,                    // highlevel socket command
           -1,                               // incoming max. window size demanded
           -1,                               // max. segment size
           NULL,                             // data pointer
           0);                               // bytes to be read
  
        Socket[socketno].RetransCounter -= 1;                          // decrement retransmission counter
        Socket[socketno].TimeToRetransmission = TIMERS_TIMETORETRANS;  // 1000ms until segment is re-sent
      }
    }
  }
}               

// --------------------------------------------------------------------------------------------- 
// function:    void SOCKET_DecrementAllTimers (INT16U socketno) 
// parameters:  <socketno>        number of port
// returns:     <none>
// purpose:             Decrements timer values of all sockets by one. (if >0)
// uses central:YES
// --------------------------------------------------------------------------------------------- 

void SOCKET_DecrementAllTimers (INT16U socketno) 
{
 if (Socket[socketno].TimeToRetransmission > CLOCK_CYCLETIME)
 {
   Socket[socketno].TimeToRetransmission -= CLOCK_CYCLETIME; // decrement time to zero
 }
 else
 {
   Socket[socketno].TimeToRetransmission = 0;        // ran out of time for this timer
 }
 if (Socket[socketno].TimeToRetransSyn > CLOCK_CYCLETIME)
 {
   Socket[socketno].TimeToRetransSyn  -= CLOCK_CYCLETIME;    // decrement time to zero
 }
 else
 {
   Socket[socketno].TimeToRetransSyn   = 0;          // ran out of time for this timer
 }
}

// --------------------------------------------------------------------------------------------- 
// function:    void SOCKET_SetForeignIpandPortZero (INT16U socketno)
// parameters:  <socketno>   number of port
// returns:     int   0      successful
//                   <0      socket error occured while reading 
// purpose:              Set foreignIP and foreignPort to zero
// uses central:NO
// --------------------------------------------------------------------------------------------- 

int SOCKET_SetForeignIpandPortZero (INT16U socketno) 
{
 if ((socketno < 0) | (socketno > MAX_SOCKET_NUMBERS))
 {
  return(SOCKERR_PARAM_RANGE);                  // socket number out of range		
 }
 Socket[socketno].ForeignIP.d = 0;              // set foreignIP zero
 Socket[socketno].ForeignPort = 0;              // set foreignPort zero
 return(SOCKERR_RESULT_OK);                     // alright, launched
}

// --------------------------------------------------------------------------------------------- 
// END OF UNIT
// --------------------------------------------------------------------------------------------- 

