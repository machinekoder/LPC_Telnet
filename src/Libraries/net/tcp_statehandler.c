// --------------------------------------------------------------------------------------------- 
// PROJECT:     TCP Stack for Keil MCB167-NET
// UNIT:        TCP_StateHandler.C
// PURPOSE:     simple handler for TCP states
// AUTHOR:      Martin Zauner, Matr.Nr.9911011114
// COPYRIGHT:   (c)2003, Fachhochschule Technikum Wien, Modul Telekom und 
//              Computer & Systemtechnik
// LAST UPDATE: 24-04-2003
// --------------------------------------------------------------------------------------------- 

// -------------------------------------------------------------------------------------------- 
// include files
// -------------------------------------------------------------------------------------------- 

#include "net_includes.h"

// -------------------------------------------------------------------------------------------- 
// externals
// -------------------------------------------------------------------------------------------- 

extern tcp_socket Socket[MAX_SOCKET_NUMBERS]; // maximum possible sockets

// -------------------------------------------------------------------------------------------- 
// state procedure type
// -------------------------------------------------------------------------------------------- 

typedef int (*stateproc) (int socketno,   // socket number (may have access to structure)
                int  command,
                INT32U foreignAddress,    // address (used for sending via TCP_Send)
                int foreignport,          // foreign port (for sending via TCP_Send)
                INT32U incoming_SEQ,      // current SEQ/ACK values
                INT32U incoming_ACK,
                int incoming_Flags,       // flags and command for this state
                int incoming_RxLen,       // length of incoming (!) payload
                int incoming_WinSize,     // limit for window size requested
                int incoming_SegSize,     // limit for segment size requested
                int  *SendBytes,      // send xxx bytes from TxBuffer
                int  *CanReceive);    // = RxLen, to receive all, else do nothing (= 0).


// --------------------------------------------------------------------------------------------- 
// function:      int Closed_State (....)
// parameters:    see  typedef of stateproc at the top of this unit
// result:        int  TCPERR_NONE       new state accepted
//                     TCPERR_NOCHANGE   state not changed
// purpose:              closed state handler
// --------------------------------------------------------------------------------------------- 

int Closed_State (int socketno, int command, INT32U foreignAddress, int foreignport,
                  INT32U incoming_SEQ, INT32U incoming_ACK, int incoming_Flags, 
                  int incoming_RxLen, int incoming_WinSize, int incoming_SegSize, 
                  int  *SendBytes, int  *CanReceive) {

*CanReceive =  0;     // nothing to receive
*SendBytes  = -1;     // do not send

// -- CONNECT COMMAND --
if (command == TCP_CONN_CONNECT)                     // CONNECT command
{
 Socket[socketno].ForeignPort       = foreignport;           // connect to external port
 Socket[socketno].ForeignIP.d       = foreignAddress;        // connect to external address
 Socket[socketno].SockState         = TCP_STATE_SYNSEND;     // change to SYNSEND
 Socket[socketno].FlagsSent         = TCP_SIG_SYN;           // send SYN
 Socket[socketno].SEQsent           = CLOCK_GetISN ();       // ISN
 Socket[socketno].ACKsent           = 0;              // (no ACK, will be ignored by target)
 Socket[socketno].TimeToRetransSyn	 = TIMERS_TIMETORETRANSSYN;// start time to retrans SYN
 Socket[socketno].RetransSYNCounter = TIMERS_SYNRETRYNUM;     // full number of retry cycles
 *SendBytes                         = 0;                      // send control segment only
 return (TCPERR_NONE);                                        // alright
}

// -- LISTEN COMMAND --
if (command == TCP_CONN_LISTEN)                    // LISTEN command
{
 Socket[socketno].SockState = TCP_STATE_LISTEN;    // change to LISTEN
 *SendBytes                 = -1;                  // do not send
 return (TCPERR_NONE);                             // alright
}

// -- signals coming in --
if (command == TCP_CONN_NONE)
if (incoming_Flags & TCP_SIG_SYN)                       // SYN signal received ..
{
 Socket[socketno].SEQrcvd   = incoming_SEQ;             // note incoming numbers
 Socket[socketno].ACKrcvd   = incoming_ACK;
 Socket[socketno].FlagsSent = TCP_SIG_RST | TCP_SIG_ACK;// send RST+ACK, refuse to connect!
 Socket[socketno].SEQsent   = incoming_ACK;             // SEQnum is incoming ACKnum
 Socket[socketno].ACKsent   = incoming_SEQ;             // ACK incoming SEQ number
 *SendBytes                 = 0;                        // send control segment only
 return (TCPERR_NONE);                                  // alright
}
return (TCPERR_NOCHANGE);                               // default, leave this state
}

// --------------------------------------------------------------------------------------------- 
// function:     int Synsend_State (....)
// parameters:   see typedef of stateproc at the top of this unit
// result:       int   TCPERR_NONE       new state accepted
//                     TCPERR_NOCHANGE   state not changed
// purpose:                Synsend state handler
// --------------------------------------------------------------------------------------------- 

int Synsend_State(int socketno, int command, INT32U foreignAddress, int foreignport,
                  INT32U incoming_SEQ, INT32U incoming_ACK, int incoming_Flags, 
                  int incoming_RxLen, int incoming_WinSize, int incoming_SegSize, 
                  int  *SendBytes, int  *CanReceive) {

*CanReceive =  0;       // nothing to receive
*SendBytes  = -1;       // do not send

// -- CLOSE COMMAND --
if (command == TCP_CONN_CLOSE)                      // CLOSE command
{
 Socket[socketno].SockState = TCP_STATE_CLOSED;     // change to CLOSED
 *SendBytes                 = -1;                   // send nothing
 return (TCPERR_NONE);                              // alright
}

// -- INCOMING SEGMENT, ACCEPT SYN or ACK OF SYN --
if (command == TCP_CONN_NONE)                 // no command, so watch the FLAGS
{
 if (incoming_Flags & TCP_SIG_SYN)            // SYN signal received..
 {
  // RECEIVE ACK OF SYN
  if (incoming_Flags & TCP_SIG_ACK)           // SYN plus ACK signal received
  {
   Socket[socketno].SegmentMaxLength = incoming_SegSize;
   Socket[socketno].SockState = TCP_STATE_ESTABLISHED; // connection now established
   Socket[socketno].FlagsSent = TCP_SIG_ACK;           // send ACK
   Socket[socketno].SEQsent   = incoming_ACK;          // sequence number
   Socket[socketno].ACKsent   = incoming_SEQ + 1;      // next SEQ nr to wait for
   Socket[socketno].SEQrcvd   = incoming_SEQ;
   Socket[socketno].ACKrcvd   = incoming_ACK;
   *SendBytes                 = 0;                     // send 0 data segment
   return (TCPERR_NONE);                               // alright
  }
   // RECEIVE SYN ONLY (synchronous open)
  else                                                 // no ACK, only SYN received
  {
   Socket[socketno].SegmentMaxLength = incoming_SegSize;
   Socket[socketno].SockState = TCP_STATE_SYNRECEIVED;    // SYN received
   Socket[socketno].FlagsSent = TCP_SIG_ACK | TCP_SIG_SYN;// send ACK + SYN
   Socket[socketno].SEQsent   = incoming_ACK;             // sequence number
   Socket[socketno].ACKsent   = incoming_SEQ + 1;         // next SEQ nr to wait for
   Socket[socketno].SEQrcvd   = incoming_SEQ;
   Socket[socketno].ACKrcvd   = incoming_ACK;
   *SendBytes                 = 0;                    // send 0 data segment
   return (TCPERR_NONE);                              // alright
  }
 }
 if ((incoming_Flags == TCP_SIG_NONE) &&              // no flags, retransmit
     (Socket[socketno].TimeToRetransSyn == 0) &&      // time to retransmit SYN
     (Socket[socketno].RetransSYNCounter > 0))        // cycle left to retransmit
 {
  Socket[socketno].FlagsSent = TCP_SIG_SYN;           // send SYN again, same numbers
  Socket[socketno].RetransSYNCounter--;
  Socket[socketno].TimeToRetransSyn = TIMERS_TIMETORETRANSSYN;// reset time to retrans SYN
  *SendBytes = 0;                                             // send 0 data segment
  return (TCPERR_NONE);                                       // alright
 }
}
return (TCPERR_NOCHANGE);                             // default, leave this state
}


// --------------------------------------------------------------------------------------------- 
// function:     int Listen_State (....)
// parameters:   see typedef of stateproc at the top of this unit
// result:       int  TCPERR_NONE      new state accepted
//                    TCPERR_NOCHANGE  state not changed
// purpose:                Listen state handler
// --------------------------------------------------------------------------------------------- 

int Listen_State(int socketno, int command, INT32U foreignAddress, int foreignport,
                 INT32U incoming_SEQ, INT32U incoming_ACK, int incoming_Flags, 
                 int incoming_RxLen, int incoming_WinSize, int incoming_SegSize, 
                 int  *SendBytes, int  *CanReceive) {

*CanReceive =  0;       // nothing to receive
*SendBytes  = -1;       // do not send

// -- CLOSE COMMAND --
if (command == TCP_CONN_CLOSE)                     // CLOSE command
{
 Socket[socketno].SockState = TCP_STATE_CLOSED;    // change to CLOSED
 *SendBytes                 = -1;                  // send nothing
 return (TCPERR_NONE);                             // alright
}

// -- SEND COMMAND --
if (command == TCP_CONN_SEND)                      // CONNECT command
{
 Socket[socketno].ForeignPort = foreignport;       // connect to external port
 Socket[socketno].ForeignIP.d = foreignAddress;    // connect to external address
 Socket[socketno].SockState   = TCP_STATE_SYNSEND; // change to SYNSEND
 Socket[socketno].FlagsSent   = TCP_SIG_SYN;       // send SYN
 Socket[socketno].SEQsent     = incoming_ACK;      // sequence number
 Socket[socketno].ACKsent     = incoming_SEQ + 1;  // next SEQ nr to wait for
 *SendBytes                   = 0;                 // send SIGNAL segment only
 return (TCPERR_NONE);                             // alright
}

// -- INCOMING SEGMENT, ACCEPT SYN --
if (command == TCP_CONN_NONE)                      // no command, so watch the FLAGS
{
 if (incoming_Flags == TCP_SIG_SYN)                // SYN signal received only
 {
  Socket[socketno].ForeignPort = foreignport;
  Socket[socketno].ForeignIP.d = foreignAddress;
  Socket[socketno].SegmentMaxLength = incoming_SegSize;
  Socket[socketno].SockState   = TCP_STATE_SYNRECEIVED;    // SYN received
  Socket[socketno].FlagsSent   = TCP_SIG_ACK | TCP_SIG_SYN;// send ACK + SYN
  Socket[socketno].SEQsent     = incoming_ACK;             // sequence number
  Socket[socketno].ACKsent     = incoming_SEQ + 1;         // next SEQ nr to wait for
  Socket[socketno].SEQrcvd     = incoming_SEQ;             // update incoming numbers
  Socket[socketno].ACKrcvd     = incoming_ACK;
  *SendBytes                   = 0;                        // send 0 data segment
  return (TCPERR_NONE);                                    // alright
 }
}
return (TCPERR_NOCHANGE);                                  // default, leave this state
}

// --------------------------------------------------------------------------------------------- 
// function:     int Synreceived_State (....)
// parameters:   see typedef of stateproc at the top of this unit
// result:       int   TCPERR_NONE      new state accepted
//                     TCPERR_NOCHANGE  state not changed
// purpose:                  Synreceived state handler
// --------------------------------------------------------------------------------------------- 

int Synreceived_State(int socketno, int command, INT32U foreignAddress, int foreignport,
                      INT32U incoming_SEQ, INT32U incoming_ACK, int incoming_Flags, 
                      int incoming_RxLen, int incoming_WinSize, int incoming_SegSize, 
                      int  *SendBytes, int  *CanReceive) {

*CanReceive =  0;   // nothing to receive
*SendBytes  = -1;   // do not send

// -- CLOSE COMMAND --
if (command == TCP_CONN_CLOSE)                    // CLOSE command
{
 Socket[socketno].SockState = TCP_STATE_FINWAIT1; // change to FINWAIT1
 Socket[socketno].FlagsSent = TCP_SIG_FIN;
 Socket[socketno].SEQsent   = incoming_ACK;       // sequence number
 Socket[socketno].ACKsent   = incoming_SEQ + 1;   // next SEQ nr to wait for
 Socket[socketno].SEQrcvd   = incoming_SEQ;       // update incoming numbers
 Socket[socketno].ACKrcvd   = incoming_ACK;
 *SendBytes                 = 0;                  // send control segment
 return (TCPERR_NONE);                            // alright
}

// -- INCOMING SEGMENT, ACCEPT ACK --
if (command == TCP_CONN_NONE)                     // no command, so watch the FLAGS
{
 if (incoming_Flags == TCP_SIG_ACK)               // ACK signal received only
 {
  Socket[socketno].SegmentMaxLength = incoming_SegSize;
  Socket[socketno].SockState = TCP_STATE_ESTABLISHED;// SYN received
  Socket[socketno].FlagsSent = TCP_SIG_NONE;
  Socket[socketno].SEQrcvd   = incoming_SEQ;         // update incoming numbers
  Socket[socketno].ACKrcvd   = incoming_ACK;
  *SendBytes                 = -1;                   // nothing to send, ESTABLISHED
  return (TCPERR_NONE);                              // alright
 }
 if (incoming_Flags & TCP_SIG_RST)                   // RST read
 {
  Socket[socketno].SegmentMaxLength = incoming_SegSize;
  Socket[socketno].SockState = TCP_STATE_LISTEN;
  Socket[socketno].FlagsSent = TCP_SIG_RST | TCP_SIG_ACK;
  Socket[socketno].SEQsent   = incoming_ACK;         // sequence number
  Socket[socketno].ACKsent   = incoming_SEQ + 1;     // next SEQ nr to wait for
  Socket[socketno].SEQrcvd   = incoming_SEQ;         // update incoming numbers
  Socket[socketno].ACKrcvd   = incoming_ACK;
  *SendBytes                 = 0;                    // nothing to send, ESTABLISHED
  return (TCPERR_NONE);                              // alright
 }
}
return (TCPERR_NOCHANGE);                            // default, leave this state
}

// --------------------------------------------------------------------------------------------- 
// function:    int Established_State (....)
// parameters:  see typedef of stateproc at the top of this unit
// result:      int   TCPERR_NONE      new state accepted
//                    TCPERR_NOCHANGE  state not changed
// purpose:                Established state handler
// --------------------------------------------------------------------------------------------- 

int Established_State(int socketno, int command, INT32U foreignAddress, int foreignport,
                      INT32U incoming_SEQ, INT32U incoming_ACK, int incoming_Flags, 
                      int incoming_RxLen, int incoming_WinSize, int incoming_SegSize, 
                      int  *SendBytes, int  *CanReceive) {
int txdiff,
    diff;              // difference of sent ACK and recvd SEQnum = data size!

*CanReceive =  0;      // nothing to receive
*SendBytes  = -1;      // do not send

// -- CLOSE COMMAND --
if (command == TCP_CONN_CLOSE)                          // CLOSE command
{
 Socket[socketno].SockState = TCP_STATE_FINWAIT1;       // change to FINWAIT1
 Socket[socketno].FlagsSent = TCP_SIG_FIN | TCP_SIG_ACK;
 Socket[socketno].SEQsent   = incoming_ACK;             // sequence number
 Socket[socketno].ACKsent   = incoming_SEQ;             // next SEQ nr to wait for
 Socket[socketno].SEQrcvd   = incoming_SEQ;             // update incoming numbers
 Socket[socketno].ACKrcvd   = incoming_ACK;
 *SendBytes                 = 0;                        // send control segment
 return (TCPERR_NONE);                                  // alright
}

// --- NO COMMAND, SO WATCH THE FLAGS ---
if (command == TCP_CONN_NONE)                  // no command, so watch FLAGS and DATA
{
 diff = incoming_RxLen;                        // incoming data length
 if (diff>0)                                   // not zero?
 {
  *CanReceive              = diff;             // nr. of bytes to receive!
  Socket[socketno].SEQsent = incoming_ACK;
  Socket[socketno].ACKsent = incoming_SEQ+diff;
  Socket[socketno].SEQrcvd = incoming_SEQ+diff;// update incoming numbers
  Socket[socketno].ACKrcvd = incoming_ACK;
  Socket[socketno].SegmentMaxLength = incoming_SegSize;
  Socket[socketno].FlagsSent     = TCP_SIG_ACK;// SEND ACK
  Socket[socketno].FlagsReceived = TCP_SIG_ACK;
  if ( incoming_Flags & TCP_SIG_ACK )
      *SendBytes =  0;                         // send control segment    
  if (diff <= (BUF_RxMAXBYTES - Socket[socketno].RxLen))
  if (diff < BUF_RxMAXWORDS)
      *CanReceive	= diff;
 }
 else                                          // no data...
 {
  if ((incoming_Flags == TCP_SIG_NONE) && 
     (Socket[socketno].TxLen > 0))             // if something to send
  {
   Socket[socketno].TimeToRetransmission = TIMERS_TIMETORETRANS;// 1000ms until segment is re-sent 
   Socket[socketno].RetransCounter       = TIMERS_RETRYNUM;     // retransmission number
   Socket[socketno].FlagsSent = TCP_SIG_PSH | TCP_SIG_ACK;      // send ACK and PSH
   *SendBytes = Socket[socketno].TxLen;                         // send all, keep all numbers!    
  }
  if (incoming_Flags == TCP_SIG_ACK)                            // CONSUME ACK
  {
   Socket[socketno].ACKrcvd = incoming_ACK;
   Socket[socketno].SEQrcvd = incoming_SEQ;                     // update incoming numbers
   if (incoming_SEQ == Socket[socketno].ACKsent)
   {
    txdiff = incoming_ACK - Socket[socketno].SEQsent;
    Socket[socketno].SEQsent = incoming_ACK;
        /* Because of multitasking and unpredictable ISR occurence it is possible
           that a segment could be retransmitted due to fact that an acknowlegde segment 
           arrives after a retransmition (ISR) already took place. To deal with that
           problem a statement has to be included to check if the transmit socket 
           data length exceeds the maximum possible data lenght specified in BUF_TxMAXBYTES */
    if ( Socket[socketno].TxLen != 0 )
    { 
     Socket[socketno].TxLen -= txdiff;                        // segment ACKnowledged
    }
   }
   Socket[socketno].SegmentMaxLength = incoming_SegSize;
   *SendBytes                        = -1;                    // send control segment
 }
}

if (incoming_Flags & TCP_SIG_FIN)                             // FIN signal received?
{
 Socket[socketno].SegmentMaxLength = incoming_SegSize;
 Socket[socketno].SockState = TCP_STATE_CLOSEWAIT;
 Socket[socketno].FlagsSent = TCP_SIG_ACK;                    // send ACK
 Socket[socketno].SEQrcvd   = incoming_SEQ;                   // update incoming numbers
 Socket[socketno].ACKrcvd   = incoming_ACK;
 Socket[socketno].SEQsent   = incoming_ACK;                   // sequence number
 Socket[socketno].ACKsent   = incoming_SEQ+1;
 Socket[socketno].TimeToRetransmission = TIMERS_TIMETORETRANS;// 1000ms until segment is re-sent 
 Socket[socketno].RetransCounter       = TIMERS_RETRYNUM;     // retransmission number	
 *SendBytes                            = 0;                   // send control segment
 return (TCPERR_NONE);                                        // alright
}
}
return (TCPERR_NOCHANGE);                                     // default, leave this state
}

// --------------------------------------------------------------------------------------------- 
// function:     int Finwait1_State (....)
// parameters:   see typedef of stateproc at the top of this unit
// result:       int   TCPERR_NONE      new state accepted
//                     TCPERR_NOCHANGE  state not changed
// purpose:                 Finwait1 state handler
// --------------------------------------------------------------------------------------------- 

int Finwait1_State(int socketno, int command, INT32U foreignAddress, int foreignport,
                   INT32U incoming_SEQ, INT32U incoming_ACK, int incoming_Flags, 
                   int incoming_RxLen, int incoming_WinSize, int incoming_SegSize, 
                   int  *SendBytes, int  *CanReceive) {

*CanReceive =  0;       // nothing to receive
*SendBytes  = -1;       // do not send

if (command == TCP_CONN_NONE)             // no command, so watch the FLAGS
{
 if (incoming_Flags == TCP_SIG_ACK)       // ACK signal received only
 {
  Socket[socketno].SegmentMaxLength = incoming_SegSize;
  Socket[socketno].SockState = TCP_STATE_FINWAIT2;// goto finwait2
  Socket[socketno].FlagsSent = TCP_SIG_NONE;
  Socket[socketno].SEQsent   = incoming_ACK;      // sequence number
  Socket[socketno].ACKsent   = incoming_SEQ + 1;  // next SEQ nr to wait for
  Socket[socketno].SEQrcvd   = incoming_SEQ;      // update incoming numbers
  Socket[socketno].ACKrcvd   = incoming_ACK;
  *SendBytes                 = -1;                // nothing to send
  return (TCPERR_NONE);                           // alright
 }
 if (incoming_Flags == TCP_SIG_FIN)               // FIN signal received only
 {
  Socket[socketno].SegmentMaxLength = incoming_SegSize;
  Socket[socketno].SockState = TCP_STATE_CLOSING; // connection closing
  Socket[socketno].FlagsSent = TCP_SIG_ACK;       // send ACK
  Socket[socketno].SEQsent   = incoming_ACK;      // sequence number
  Socket[socketno].ACKsent   = incoming_SEQ + 1;  // next SEQ nr to wait for
  Socket[socketno].SEQrcvd   = incoming_SEQ;
  Socket[socketno].ACKrcvd   = incoming_ACK;
  *SendBytes                 = 0;                 // send 0 data segment
  return (TCPERR_NONE);                           // alright
 }
 if (incoming_Flags == (TCP_SIG_FIN || TCP_SIG_ACK ))// both FIN+ACK signal received
 {
  Socket[socketno].SegmentMaxLength = incoming_SegSize;
  Socket[socketno].SockState = TCP_STATE_TIMEDWAIT;// wait 2 SSL
  Socket[socketno].FlagsSent = TCP_SIG_ACK;        // send ACK
  Socket[socketno].SEQsent   = incoming_ACK;       // sequence number
  Socket[socketno].ACKsent   = incoming_SEQ + 1;   // next SEQ nr to wait for
  Socket[socketno].SEQrcvd   = incoming_SEQ;
  Socket[socketno].ACKrcvd   = incoming_ACK;
  *SendBytes                 = 0;                  // send 0 data segment
  return (TCPERR_NONE);                            // alright
 }
 if (incoming_Flags & TCP_SIG_RST)                 // RST read
 {
  Socket[socketno].SegmentMaxLength = incoming_SegSize;
  Socket[socketno].SockState = TCP_STATE_LISTEN;
  Socket[socketno].FlagsSent = TCP_SIG_RST | TCP_SIG_ACK;
  Socket[socketno].SEQsent   = incoming_ACK;       // sequence number
  Socket[socketno].ACKsent   = incoming_SEQ + 1;   // next SEQ nr to wait for
  Socket[socketno].SEQrcvd   = incoming_SEQ;       // update incoming numbers
  Socket[socketno].ACKrcvd   = incoming_ACK;
  *SendBytes                 = 0;                  // nothing to send, ESTABLISHED
  return (TCPERR_NONE);                            // alright
 }
}
return (TCPERR_NOCHANGE);                          // default, leave this state
}

// --------------------------------------------------------------------------------------------- 
// function:    int Finwait2_State (....)
// parameters:  see typedef of stateproc at the top of this unit
// result:      int   TCPERR_NONE      new state accepted
//                    TCPERR_NOCHANGE  state not changed
// purpose:                 Finwait2 state handler
// --------------------------------------------------------------------------------------------- 

int Finwait2_State(int socketno, int command, INT32U foreignAddress, int foreignport,
                   INT32U incoming_SEQ, INT32U incoming_ACK, int incoming_Flags, 
                   int incoming_RxLen, int incoming_WinSize, int incoming_SegSize, 
                   int  *SendBytes, int  *CanReceive) {

*CanReceive =  0;      // nothing to receive
*SendBytes  = -1;      // do not send

// -- INCOMING SEGMENT, ACCEPT FIN --
if (command == TCP_CONN_NONE)           // no command, so watch the FLAGS
{
 if (incoming_Flags == TCP_SIG_FIN)     // FIN signal received
 {
  Socket[socketno].SegmentMaxLength = incoming_SegSize;
  Socket[socketno].SockState = TCP_STATE_TIMEDWAIT;// wait 2 SSL
  Socket[socketno].FlagsSent = TCP_SIG_ACK;        // send ACK
  Socket[socketno].SEQsent   = incoming_ACK;       // sequence number
  Socket[socketno].ACKsent   = incoming_SEQ + 1;   // next SEQ nr to wait for
  Socket[socketno].SEQrcvd   = incoming_SEQ;
  Socket[socketno].ACKrcvd   = incoming_ACK;
  *SendBytes                 = 0;                  // send 0 data segment
  return (TCPERR_NONE);                            // alright
 }
 if ( (incoming_Flags & TCP_SIG_FIN) &&
      (incoming_Flags & TCP_SIG_ACK) )              // FIN and ACK signal received
 {
  Socket[socketno].SegmentMaxLength = incoming_SegSize;
  Socket[socketno].SockState = TCP_STATE_CLOSED;            // connection closed
  Socket[socketno].FlagsSent = TCP_SIG_ACK;                 // send ACK
  Socket[socketno].SEQsent   = Socket[socketno].ACKrcvd;    // sequence number
  Socket[socketno].ACKsent   = Socket[socketno].SEQrcvd + 1;// next SEQ nr to wait for
  *SendBytes                 = 0;                           // send 0 data segment
  return (TCPERR_NONE);                                     // alright
 }
 if (incoming_Flags & TCP_SIG_RST)                  // RST read
 {
  Socket[socketno].SegmentMaxLength = incoming_SegSize;
  Socket[socketno].SockState = TCP_STATE_LISTEN;
  Socket[socketno].FlagsSent = TCP_SIG_RST | TCP_SIG_ACK;
  Socket[socketno].SEQsent   = incoming_ACK;        // sequence number
  Socket[socketno].ACKsent   = incoming_SEQ + 1;    // next SEQ nr to wait for
  Socket[socketno].SEQrcvd   = incoming_SEQ;        // update incoming numbers
  Socket[socketno].ACKrcvd   = incoming_ACK;
  *SendBytes                 = 0;                   // nothing to send, ESTABLISHED
  return (TCPERR_NONE);                             // alright
 }
}
return (TCPERR_NOCHANGE);                           // default, leave this state
}

// --------------------------------------------------------------------------------------------- 
// function:      int Closing_State (....)
// parameters:    see typedef of stateproc at the top of this unit
// result:        int   TCPERR_NONE      new state accepted
//                      TCPERR_NOCHANGE  state not changed
// purpose:                  Closing state handler
// --------------------------------------------------------------------------------------------- 

int Closing_State(int socketno, int command, INT32U foreignAddress, int foreignport,
                  INT32U incoming_SEQ, INT32U incoming_ACK, int incoming_Flags, 
                  int incoming_RxLen, int incoming_WinSize, int incoming_SegSize, 
                  int  *SendBytes, int  *CanReceive) {

*CanReceive =  0;         // nothing to receive
*SendBytes  = -1;         // do not send

if (command == TCP_CONN_NONE)         // no command, so watch the FLAGS
{
 if (incoming_Flags == TCP_SIG_ACK)   // ACK signal received
 {
  Socket[socketno].SegmentMaxLength = incoming_SegSize;
  Socket[socketno].SockState        = TCP_STATE_TIMEDWAIT;// wait 2 SSL
  Socket[socketno].FlagsSent        = TCP_SIG_NONE;
  Socket[socketno].SEQrcvd          = incoming_SEQ;       // update incoming numbers
  Socket[socketno].ACKrcvd          = incoming_ACK;
  *SendBytes                        = -1;                 // nothing to send
  return (TCPERR_NONE);                                   // alright
 }
}
return (TCPERR_NOCHANGE);                                 // default, leave this state
}

// --------------------------------------------------------------------------------------------- 
// function:     int Timedwait_State (....)
// parameters:   see typedef of stateproc at the top of this unit
// result:       int   TCPERR_NONE      new state accepted
//                     TCPERR_NOCHANGE  state not changed
// purpose:               Timedwait state handler
// note:                  quiet timer not implemented to prevent the state
//                        machine from blocking the connection for 2 segment
//                        lifetimes. The <closed> state will now immediately
//                        follow <timedwait>
// --------------------------------------------------------------------------------------------- 

int Timedwait_State(int socketno, int command, INT32U foreignAddress, int foreignport,
                    INT32U incoming_SEQ, INT32U incoming_ACK, int incoming_Flags, 
                    int incoming_RxLen, int incoming_WinSize, int incoming_SegSize, 
                    int  *SendBytes, int  *CanReceive) {

*CanReceive =  0;        // nothing to receive
*SendBytes  = -1;        // do not send

// -- INCOMING SEGMENT, ACCEPT FIN --
if (command == TCP_CONN_NONE)          // no command, so watch the FLAGS
{
 if (incoming_Flags == TCP_SIG_ACK)    // ACK signal received
 {
  Socket[socketno].SegmentMaxLength = incoming_SegSize;
  // NOTE: SHOULD WAIT 2 MSL HERE ...
  Socket[socketno].SockState        = TCP_STATE_CLOSED;// connection closed
  Socket[socketno].FlagsSent        = TCP_SIG_NONE;
  Socket[socketno].SEQrcvd          = incoming_SEQ;    // update incoming numbers
  Socket[socketno].ACKrcvd          = incoming_ACK;
  *SendBytes                        = -1;              // nothing to send
  return (TCPERR_NONE);                                // alright
 }
}
return (TCPERR_NOCHANGE);                              // default, leave this state
}

// --------------------------------------------------------------------------------------------- 
// function:     int Closewait_State (....)
// parameters:   see typedef of stateproc at the top of this unit
// result:       int   TCPERR_NONE      new state accepted
//                     TCPERR_NOCHANGE  state not changed
// purpose:                 Closewait state handler
// note:                    Do not wait for manual CLOSE command now
// --------------------------------------------------------------------------------------------- 

int Closewait_State(int socketno, int command, INT32U foreignAddress, int foreignport,
                    INT32U incoming_SEQ, INT32U incoming_ACK, int incoming_Flags, 
                    int incoming_RxLen, int incoming_WinSize, int incoming_SegSize, 
                    int  *SendBytes, int  *CanReceive) {

*CanReceive =  0;       // nothing to receive
*SendBytes  = -1;       // do not send

if (command == TCP_CONN_NONE)                             // now auto goto LASTACK (SKIP!)
{
 Socket[socketno].SegmentMaxLength = incoming_SegSize;
 Socket[socketno].SockState   = TCP_STATE_LASTACK;        // wait for last ACK
 Socket[socketno].FlagsSent   = TCP_SIG_FIN | TCP_SIG_ACK;// send ACK+FIN
 Socket[socketno].SEQrcvd     = incoming_SEQ;
 Socket[socketno].ACKrcvd     = incoming_ACK;
 *SendBytes                   = 0;                         // send 0 data segment
 return (TCPERR_NONE);                                     // alright
}
return (TCPERR_NOCHANGE);                                  // default, leave this state
}

// --------------------------------------------------------------------------------------------- 
// function:     int Lastack_State (....)
// parameters:   see typedef of stateproc at the top of this unit
// result:       int   TCPERR_NONE      new state accepted
//                     TCPERR_NOCHANGE  state not changed
// purpose:                 Lastack state handler
// --------------------------------------------------------------------------------------------- 

int Lastack_State(int socketno, int command, INT32U foreignAddress, int foreignport,
                  INT32U incoming_SEQ, INT32U incoming_ACK, int incoming_Flags, 
                  int incoming_RxLen, int incoming_WinSize, int incoming_SegSize, 
                  int  *SendBytes, int  *CanReceive) {

*CanReceive =  0;        // nothing to receive
*SendBytes  = -1;        // do not send

if (command == TCP_CONN_NONE)                         // no command, so watch the FLAGS
{
 if (incoming_Flags == TCP_SIG_ACK)                   // FIN signal received
 {
  Socket[socketno].SegmentMaxLength = incoming_SegSize;
  Socket[socketno].SockState    = TCP_STATE_CLOSED;   // now closed
  Socket[socketno].FlagsSent    = TCP_SIG_NONE;
  Socket[socketno].SEQrcvd      = incoming_SEQ;       // update incoming numbers
  Socket[socketno].ACKrcvd      = incoming_ACK;
  *SendBytes                    = -1;                 // nothing to send
  return (TCPERR_NONE);                               // alright
 }
}
return (TCPERR_NOCHANGE);                             // default, leave this state
}

// --------------------------------------------------------------------------------------------- 
// function:    int TCPSTATE_Central(int socketno, INT32U foreignAddress, int foreignport,
//                                   int ownport, INT32U SEQnum, INT32U ACKnum, int Flags,
//                                   int Command, int Window, int SegmentMaxSize,
//                                   INT16U  *Data, INT16U Datalen) 
// parameters:  <socketno>        socket number (redundant) 
//              <foreignAddress>  foreign address
//              <foreignport>     foreign port
//              <ownport>         own port number (obligatory!)
//              <SEQnum>          incoming SEQ number
//              <ACKnum>          incoming ACK number
//              <Flags>           incoming Flags (none, if command is to be recognized)
//              <Command>         manual command to execute
//              <Window>          requested window size limit
//              <SegmentMaxSize>  desired max seg size
//              <*Data>           pointer to incoming data
//              <Datalen>         length of incoming data
// result:      int  0    OK
//                  -1    undefined error of the state machine
// purpose:                    simple TCP state machine, calls state handling 
//                             procedures.
// --------------------------------------------------------------------------------------------- 

int TCPSTATE_Central(int socketno,          // Sock number
                     INT32U foreignAddress, // address (used for sending via TCP_Send)
                     int foreignport,       // foreign port (for sending via TCP_Send)
                     int ownport,           // own port (not needed if ardy BOUND)
                     INT32U SEQnum,         // incoming SEQ value
                     INT32U ACKnum,         // incoming ACK value
                     int Flags,             // incoming flags
                     int Command,
                     int Window,            // incoming window size limit
                     int SegmentMaxSize,    // max segment size desired
                     INT16U  *Data,     // pointer to incoming data
                     INT16U Datalen) {      // data	size in bytes

int SendMaxSeg,
    Current_TCPState;            // current state (of <socketno> socket)

stateproc TCPstate_handler;      // current state handler

int ToSendBytes ;                // nr. of bytes to send
int ReceiveBytes ;               // grant to receive xx bytes

if (socketno == SOCKERR_PORT_UNUSED) return(-1);    // port is not BOUND!
if  ((socketno < 0) | (socketno > 9)) return(-1);   // socket number out of range
 // -- OK, socket number found, state is not UNAVAILABLE or UNBOUND --
 // SOCKET STATE
Current_TCPState = Socket[socketno].SockState;      // SOCKET STATE
 // ignore false port?
if (Current_TCPState > TCP_STATE_LISTEN)            // foreign port must be set here ..
 if  (Socket[socketno].ForeignPort != foreignport) 
      return(-1);                                   // ignore, accept only one foreign port
  // --- main switch ---
  switch (Current_TCPState) { 
    case TCP_STATE_CLOSED:      TCPstate_handler = (stateproc)Closed_State;     break;
    case TCP_STATE_SYNSEND:     TCPstate_handler = (stateproc)Synsend_State;    break;
    case TCP_STATE_LISTEN:      TCPstate_handler = (stateproc)Listen_State;     break;
    case TCP_STATE_SYNRECEIVED: TCPstate_handler = (stateproc)Synreceived_State;break;
    case TCP_STATE_ESTABLISHED: TCPstate_handler = (stateproc)Established_State;break;
    case TCP_STATE_FINWAIT1:    TCPstate_handler = (stateproc)Finwait1_State;   break;
    case TCP_STATE_FINWAIT2:    TCPstate_handler = (stateproc)Finwait2_State;   break;
    case TCP_STATE_CLOSING:     TCPstate_handler = (stateproc)Closing_State;    break;
    case TCP_STATE_TIMEDWAIT:   TCPstate_handler = (stateproc)Timedwait_State;  break;
    case TCP_STATE_CLOSEWAIT:   TCPstate_handler = (stateproc)Closewait_State;  break;
    case TCP_STATE_LASTACK:     TCPstate_handler = (stateproc)Lastack_State;    break;
    default: TCPstate_handler = NULL; break;
  }
if (TCPstate_handler == NULL) return(-1);     // can't execute NULL procedure

 // -- now call this handler and get a new state and the permission to send/receive --
TCPstate_handler (socketno,                   // socket ID
                  Command,                    // TCP_CONN_xx command
                  foreignAddress,
                  foreignport,
                  SEQnum,                     // current SEQ/ACK values
                  ACKnum,
                  Flags,
                  Datalen,                    // data length (>0 if incoming data avail)
                  Window,
                  SegmentMaxSize,             // max segment size requested
                  (int  *)&ToSendBytes,   // send from TxBuffer via TCP_Send
                  (int  *)&ReceiveBytes); // receive from *Data into RxBuffer.. 
                                              // ..and launch event proc

if (ReceiveBytes >0)
{                                             // udate RxLen for Window sending!
 SOCKET_receive (socketno, (INT16U  *)&Data[0], Datalen);
}
if (ToSendBytes >= 0) 
{
 if (Socket[socketno].FlagsSent & TCP_SIG_SYN)
 { 
  SendMaxSeg = 256;
 }
 else
 {  
  SendMaxSeg = -1;
 }

TCP_SendSegment (Socket[socketno].ForeignIP,
                 Socket[socketno].OwnPort,          // Source Port, eg. Telnet (23)
                 Socket[socketno].ForeignPort,
                 Socket[socketno].SEQsent,
                 Socket[socketno].ACKsent,
                 Socket[socketno].FlagsSent,
                (2*BUF_RxMAXWORDS) - Socket[socketno].RxLen,
                 SendMaxSeg,
                (INT16U  *)(Socket[socketno].TxBuffer+(Socket[socketno].TxLen - ToSendBytes)),//[Socket[socketno].TxLen - ToSendBytes]), 
                 ToSendBytes);
}                                   // send 0-byte segment (flags only) or TxBuffer info (>0)
SOCKET_launcheventproc (socketno);  // launch event if available
return(0);
}

// --------------------------------------------------------------------------------------------- 
// END OF UNIT
// --------------------------------------------------------------------------------------------- 

