// ---------------------------------------------------------------------------------------------
// PROJECT:     TCP Stack for Keil MCB167-NET
// UNIT:        SOCKETS.H
// PURPOSE:     Interface Unit to higher layers, provides standardized socket interface
// AUTHOR:      Martin Zauner, Matr.Nr.9911011114
// COPYRIGHT:   (c)2003, Fachhochschule Technikum Wien, Modul Telekom und
//              Computer & Systemtechnik
// LAST UPDATE: 24-04-2003
// ---------------------------------------------------------------------------------------------

#ifndef SOCKETPROCS
 #define SOCKETPROCS

#include "tcp_statehandler.h"    // TCP State Machine
#include "utility_procs.h"       // prototyes definition swap byte order

// --------------------------------------------------------------------------------------------
// socket only constants
// --------------------------------------------------------------------------------------------

 // standard values for retransmission cycles
#define  TIMERS_RETRYNUM           3      // retransmission number
#define  TIMERS_SYNRETRYNUM        3      // SYN retransmission number
 // default timer values for reload
#define  TIMERS_TIMETORETRANS      1000   // 1000ms until segment is re-sent
#define  TIMERS_TIMETORECONN       1000   // 1000ms until conn may be re-established
#define  TIMERS_TIMETORETRANSSYN   1000   // 1000ms until SYN is re-sent
#define  TIMERS_TIMETOGIVEUP       60000  // 60sec until "dead" conn is shut down
#define  TIMERS_TIMETOBEPERSISTENT 10000  // 10sec until test segment is sent
 // timer value for unused
#define  TIMERS_NEVERENDINGTIME    -1     // unused

// --------------------------------------------------------------------------------------------
// Layer 4 error codes, must be <0 not to overlap with TCP_STATE and socket numbers!
// --------------------------------------------------------------------------------------------

#define MAX_SOCKET_NUMBERS  1

#define  SOCKERR_RESULT_OK      -10       // everything is OK, no error
#define  SOCKERR_WRONG_STATE    -11       // state is unsuitable for current operation
#define  SOCKERR_READ_RANGE     -12       // cannot read over range
#define  SOCKERR_NULL_PTR       -13       // NULL pointer must not be used
#define  SOCKERR_BUFFER_EMPTY   -14       // buffer is empty
#define  SOCKERR_BUFFER_FULL    -15       // transmit/receive buffer is full
#define  SOCKERR_PARAM_RANGE    -16       // input parameter is invalid
#define  SOCKERR_PORT_UNUSED    -17       // port not used
#define  SOCKERR_EVENTPROC_NULL -18       // event procedure is undefined
#define	 SOCKERR_READ_TOOMUCH   -19       // buffer holds not enough data to read
#define  SOCKERR_PORTS_EMPTY    -20       // no more port available
#define  SOCKERR_MIS_READ       -30       // general error when moving payload to RxBuffer


// --------------------------------------------------------------------------------------------
// TCP socket definition
// --------------------------------------------------------------------------------------------

typedef struct _tcp_socket {              // socket structure
// -- GENERAL --
 INT16U     OwnPort;                      // my own port
 INT16U     ForeignPort;                  // partner's port
 IP_A      OwnIP;                         // just for comfort
 IP_A      ForeignIP;                     // IP address of my target
 procref   EventProcedure;                // user definable event procedure
 INT16U     SegmentMaxLength;             // maximum length to send requested
// -- CONNECTION STATUS --
 INT16U     SockState;                    // current socket state
 INT8U     TxConfirmed;                   // ACK received? (last num to ACK is SEQsent)
// -- SEND BUFFER --
 INT8U*    TxBuffer; //TxBuffer[BYTESTOSEND];         // Send buffer
 INT16U     TxLen;                        // bytes to send
 INT8U     FlagsSent;                     // flags recently sent
 INT32U     SEQsent,                      // recently send SEQ value
           ACKsent;                       // recently send ACK value
// -- RECEIVE BUFFER --
 INT8U*    RxBuffer; //RxBuffer[BYTESTORECEIVE];      // Receive buffer
 INT16U     RxLen;                        // bytes received
 INT8U     FlagsReceived;                 // flags received, just for info
 INT32U     SEQrcvd,                      // last SEQ number received
           ACKrcvd;                       // last ACK number received
// -- TIMER VALUES --
 // retransmission timer
 long      TimeToRetransmission;          // time until last segment is re-sent
 INT16U     RetransCounter;               // Number of retransmission cycles left
 // retransmit-SYN timer
 long      TimeToRetransSyn;              // time until SYN may be retransmitted
 INT16U     RetransSYNCounter;            // Number of SYN retransmission cycles left
// -- END OF STRUCTURE --
} __attribute__ ((packed)) tcp_socket;

// ---------------------------------------------------------------------------------------------
// function:   int SOCKET_bind (int socketno, int ownport)
// parameters: <socketno>     socket number
//             <ownport>      port of socket
// result:     int  0         socket available
//                < 0         bind to socket failed
// purpose:             binds a valid socket to own port/IP address. Socket must
//                      be available (use SOCKET_NewSocket) prior to binding.
//                      If OK, the new status is CLOSED.
// uses central:NO
// ---------------------------------------------------------------------------------------------

int SOCKET_bind (int socketno, int ownport);

// ---------------------------------------------------------------------------------------------
// function:   int SOCKET_close (int socketno)
// parameters: <socketno>     number of socket
// result:     int   0        socket will now be closed
//                  <0        cannot close from current state
// purpose:             closes the socket, state will change to CLOSED
// uses central:YES
// ---------------------------------------------------------------------------------------------

int SOCKET_close (int socketno);

// ---------------------------------------------------------------------------------------------
// function:   int SOCKET_connect (int socketno, INT32U foreignaddress, int foreignport)
// parameters: <socketno>        number of socket
//             <foreignaddress>  address where to send the message
//             <foreignport>     port where to send
// result:     int    0          successfully sent
//                   <0          invalid state to connect
// purpose:               launches a connect request
// uses central:YES
// ---------------------------------------------------------------------------------------------

int SOCKET_connect (int socketno, INT32U foreignaddress, int foreignport);

// ---------------------------------------------------------------------------------------------
// function:   int SOCKET_listen (int socketno)
// parameters: <socketno>     number of socket
// result:      0             successfully sent listen command
//             <0             invalid state to listen
// purpose:             moves to LISTEN state if currently in CLOSED state
// uses central:NO
// ---------------------------------------------------------------------------------------------

int SOCKET_listen (int socketno);

// ---------------------------------------------------------------------------------------------
// function:    int SOCKET_read (int socketno, byte huge *readbuffer, int length)
// parameters:  <socketno>     socket ID
//              <*readbuffer>  pointer to returned data
//              <length>       nominal number of bytes to be read
//                             (must be < RxLen, determined using <SOCKET_RxBytes>)
// result:       0             successfully read
//              <0             socket read error, eg. invalid state to read or requested
//                             block size too large
// purpose:               reads socket read buffer (LIFO organized)
// uses central:NO
// ---------------------------------------------------------------------------------------------

int SOCKET_read (int socketno, INT16U *readbuffer, int length);

// ---------------------------------------------------------------------------------------------
// function:   int SOCKET_newsocket (int socketno)
// parameters: <socketno>      socketID
// result:      0              socket now ready for binding
//             <0              cannot create socket, socket is ardy in use
// purpose:               changes UNAVAILABLE state to UNBOUND state, there are no
//                        special conditions to fulfil.
// uses central:NO
// ---------------------------------------------------------------------------------------------

int SOCKET_newsocket (int socketno);

// ---------------------------------------------------------------------------------------------
// function:   int SOCKET_getsocket ()
// parameters: <none>
// result:     >=0       socket now ready for binding
//              <0       cannot create socket, no socket is available
// purpose:         get a free socker number by using the function
//                  SOCKET_newsocket() and return it, if no socket is
//                  available return error code -20(SOCKERR_PORTS_EMPTY)
// uses central:NO
// ---------------------------------------------------------------------------------------------

int SOCKET_getsocket(void);

// ---------------------------------------------------------------------------------------------
// function:   int SOCKET_freesocket (int socketno)
// parameters: <socketno>     socket ID
// result:      0             successfully removed
//             <0             cannot free a socket which is in operation (not CLOSED)
// purpose:              frees a closed socket and makes it unavailable
// uses central:NO
// ---------------------------------------------------------------------------------------------

int SOCKET_freesocket (int socketno);

// ---------------------------------------------------------------------------------------------
// function:   int SOCKET_write (int socketno, byte huge *readbuffer, int length)
// parameters: <socketno>      socket ID
//             <*writebuffer>  pointer to data
//             <length>        data bytes to be written
// result:      0              successfully written
//             <0              socket write error, eg. invalid state to write or buffer full
// purpose:               writes data into socket
// uses central:YES
// ---------------------------------------------------------------------------------------------

int SOCKET_write (int socketno, INT16U *writebuffer, int length);

// ---------------------------------------------------------------------------------------------
// function:   int SOCKET_status (int socketno)
// parameters: <socketno>    socket ID (0..9)
// result:     int           socket status = [CLOSED, SYNSEND, LISTEN, SYNRECEIVED, ....]
// purpose:             returns the status of the socket
// uses central:NO
// ---------------------------------------------------------------------------------------------

int SOCKET_status (int socketno);

// ---------------------------------------------------------------------------------------------
// function:   int SOCKET_RxBytes (int socketno)
// parameters: <socketno>    socket ID (0..9)
// result:     int           bytes in RxBuffer[socketno] for read
// purpose:              returns the data bytes available in RxBuffer
// uses central:NO
// ---------------------------------------------------------------------------------------------

int SOCKET_RxBytes (int socketno);

// ---------------------------------------------------------------------------------------------
// function:   int SOCKET_TxBytesFree (int socketno)
// parameters: <socketno>      socket ID (0..9)
// result:     int             bytes in TxBuffer[socketno] free for sending
// purpose:               returns free data bytes in TxBuffer
// uses central:NO
// ---------------------------------------------------------------------------------------------

int SOCKET_TxBytesFree (int socketno);

// ---------------------------------------------------------------------------------------------
// function:   int SOCKET_RxBytesFree (int socketno)
// parameters: <socketno>      socket ID (0..9)
// result:                     bytes in RxBuffer[socketno] free for receiving
// purpose:               returns free data bytes in RxBuffer
// uses central:NO
// ---------------------------------------------------------------------------------------------

int SOCKET_RxBytesFree (int socketno);

// ---------------------------------------------------------------------------------------------
// function:   int SOCKET_seteventproc (int socketno, procref EventProc)
// parameters: <socketno>        socket ID
//             <EventProcedure>  address of procedure to be launched after receiving a data
//                               containing TCP segment.
// result:     func result  0    procedure successfully linked
//                         <0    unspecified error
// purpose:                   sets the procedure <EventProcedure> to be called after
//                            receiving TCP payload
// uses central:NO
// ---------------------------------------------------------------------------------------------

int SOCKET_seteventproc (int socketno, procref EventProc);

// ---------------------------------------------------------------------------------------------
// procedure:   void SOCKET_allinit (void)
// parameters:  <none>
// result:      <none>
// purpose:            init procedure, sets all sockets to unavailable.
// uses central:NO
// ---------------------------------------------------------------------------------------------

void SOCKET_allinit (void);

// ---------------------------------------------------------------------------------------------
// function:   int SOCKET_usedSocketno (INT16U portno, INT32U ipaddr, INT16U foreignport)
// parameters: <portno>        number of port
//             <ipaddr>        foreign ip address
//             <foreignport>   forein port
// returns:    int   [0..9]    number of socket which uses portno
//                   <0        if failed
// purpose:              finds out, which socket is dedicated to the given port, foreignIp,
// uses central:NO
// ---------------------------------------------------------------------------------------------

int SOCKET_usedSocketno (INT16U portno, INT32U ipaddr, INT16U foreignport);

// ---------------------------------------------------------------------------------------------
// function:   int SOCKET_usesport (INT16U portno)
// parameters: <portno>      number of port
// returns:    int   [0..9]  number of socket which uses portno
//                   <0      if failed
// purpose:              finds out, which socket is dedicated to the given port
// uses central:NO
// ---------------------------------------------------------------------------------------------

int SOCKET_usesport (INT16U portno);

// ---------------------------------------------------------------------------------------------
// function:    int SOCKET_receive (int socketno, INT16U huge *Data, INT16U Datalen)
// parameters:  <socketno>    number of socket
//              <*Data>       pointer to payload
//              <Datalen>     number of bytes to receive
// returns:     int    0      successful
//                    <0      socket error occured while reading
// purpose:               main socket receive function, moves TCP payload into read
//                        buffer, then calls the central function of state machine.
// uses central:YES
// ---------------------------------------------------------------------------------------------

int SOCKET_receive (int socketno, INT16U *Data, INT16U Datalen);

// ---------------------------------------------------------------------------------------------
// function:   int SOCKET_launcheventproc (INT16U socketno)
// parameters: <socketno>     number of socket
// returns:     int  0        event procedure successfully launched
//                  <0        undefined error eg. socketno not in range
// purpose:             launches socket event procedure
// uses central:NO
// ---------------------------------------------------------------------------------------------

int SOCKET_launcheventproc (INT16U socketno);

// ---------------------------------------------------------------------------------------------
// function:    void SOCKET_refresh (void)
// parameters:  <none>
// returns:     <none>
// purpose:             frequently called for re-transmission of segments. The central
//                      state machine function will launch a new segment if the send
//                      buffer contains some bytes to send or a control message must
//                      be transmitted.
// uses central:YES
// ---------------------------------------------------------------------------------------------

void SOCKET_refresh (void);

// ---------------------------------------------------------------------------------------------
// function:    void SOCKET_DecrementAllTimers (INT16U socketno)
// parameters:  <socketno>    number of port
// returns:     <none>
// purpose:             Decrements timer values of all sockets by one. (if >0)
// uses central:YES
// ---------------------------------------------------------------------------------------------

void SOCKET_DecrementAllTimers (INT16U socketno);

// ---------------------------------------------------------------------------------------------
// function:   int SOCKET_SetForeignIpandPortZero (INT16U socketno)
// parameters: <socketno>      number of port
// returns:    <int>  0        successful
//                   <0        socket error occured while reading
// purpose:               Set foreignIP and foreignPort to zero
// uses central:NO
// ---------------------------------------------------------------------------------------------

int SOCKET_SetForeignIpandPortZero (INT16U socketno);

#endif

// ---------------------------------------------------------------------------------------------
// END OF UNIT
// ---------------------------------------------------------------------------------------------

