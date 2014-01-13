// ---------------------------------------------------------------------------------------------
// PROJECT:     TCP Stack for Keil MCB167-NET
// UNIT:        TCP_StateHandler.H
// PURPOSE:     simple handler for TCP states
// AUTHOR:      Martin Zauner, Matr.Nr.9911011114
// COPYRIGHT:   (c)2003, Fachhochschule Technikum Wien, Modul Telekom und
//              Computer & Systemtechnik
// LAST UPDATE: 24-04-2003
// ---------------------------------------------------------------------------------------------

#ifndef TCP_STATEHANDLERPROC_DEFINES
 #define TCP_STATEHANDLERPROC_DEFINES

#include "tcp_statehandler.h"    // TCP State Machine

// --------------------------------------------------------------------------------------------
// TCP socket states
// --------------------------------------------------------------------------------------------

#define  TCP_STATE_UNAVAILABLE 0    // returned if socket is unavailable
#define  TCP_STATE_UNBOUND     1    // returned if socket is unbound
#define  TCP_STATE_CLOSED      2    // initialized but not ready for operation
#define  TCP_STATE_SYNSEND     4    // SYN sent, want to establish a connection
#define  TCP_STATE_LISTEN      8    // listening for incoming SNYs
#define  TCP_STATE_SYNRECEIVED 16   // SYN received while listening
#define  TCP_STATE_ESTABLISHED 32   // connection is active
#define  TCP_STATE_FINWAIT1    64   // high OSI layer has sent CLOSE command, send FIN
#define  TCP_STATE_FINWAIT2    128  // ACKnowledge received to close
#define  TCP_STATE_CLOSING     256  // simulateneous closing detected
#define  TCP_STATE_TIMEDWAIT   512  // wait before closing
#define  TCP_STATE_CLOSEWAIT   1024 // FIN received, wait for CLOSE permission
#define  TCP_STATE_LASTACK     2048 // wait for ACK of my FIN

// --------------------------------------------------------------------------------------------
// resulting values of TCPSTATE_StateFunc
// --------------------------------------------------------------------------------------------

#define  TCPERR_NONE       0        // no error while changing state
#define  TCPERR_NOCHANGE  -1        // state could not be changed

// --------------------------------------------------------------------------------------------
// TCP signals to send or receive (constants start at zero)
// --------------------------------------------------------------------------------------------

#define  TCP_SIG_NONE  0            // careful, consts must not be changed!
#define  TCP_SIG_FIN   1
#define  TCP_SIG_SYN   2
#define  TCP_SIG_RST   4
#define  TCP_SIG_PSH   8
#define  TCP_SIG_ACK   16
#define  TCP_SIG_URG   32

// --------------------------------------------------------------------------------------------
// TCP commands received from higher OSI layers, consts must be >32!
// --------------------------------------------------------------------------------------------

#define  TCP_CONN_NONE     0        // NONE
#define  TCP_CONN_CONNECT  64       // ACTIVE OPEN
#define  TCP_CONN_LISTEN   128      // ACCEPT CONNECT, GOTO LISTEN
#define  TCP_CONN_SEND     256      // SEND SYN WHEN LISTENING
#define  TCP_CONN_CLOSE    512      // CLOSE CONNECTION

// --------------------------------------------------------------------------------------------
// TCP commands from other procedures > 512!
// --------------------------------------------------------------------------------------------

#define  TCP_CONN_TIMER       1024  // call for retransmission
#define  TCP_CONN_DATAMISREAD 2048  // data misreceived (send RST?)

// --------------------------------------------------------------------------------------------
// TCP socket constants
// --------------------------------------------------------------------------------------------

typedef void (*procref)(void);      // procedure header
#define CLOCK_CYCLETIME    200      // 200 ms cycle time for internal clock

// ---------------------------------------------------------------------------------------------
// function:   int TCPSTATE_Central(int socketno, INT32U foreignAddress, int foreignport,
//                                 int ownport, INT32U SEQnum, INT32U ACKnum, int Flags,
//                                 int Command, int Window, int SegmentMaxSize,
//                                 INT16U huge *Data, INT16U Datalen)
// parameters: <socketno>        socket number (redundant)
//             <foreignAddress>  foreign address
//             <foreignport>     foreign port
//             <ownport>         own port number (obligatory!)
//             <SEQnum>          incoming SEQ number
//             <ACKnum>          incoming ACK number
//             <Flags>           incoming Flags (none, if command is to be recognized)
//             <Command>         manual command to execute
//             <Window>          requested window size limit
//             <SegmentMaxSize>  desired max seg size
//             <*Data>           pointer to incoming data
//             <Datalen>         length of incoming data
// result:     int     0         OK
//                    -1         undefined error of the state machine
// purpose:                  simple TCP state machine, calls state handling
//                           procedures.
// ---------------------------------------------------------------------------------------------

int TCPSTATE_Central(int socketno,            // Sock number
                     INT32U foreignAddress,   // address
                     int foreignport,         // foreign port
                     int ownport,             // own port
                     INT32U SEQnum,           // incoming SEQ value
                     INT32U ACKnum,           // incoming ACK value
                     int Flags,               // incoming flags
                     int Command,
                     int Window,              // incoming window size limit
                     int SegmentMaxSize,      // max segment size desired
                     INT16U *Data,            // pointer to incoming data
                     INT16U Datalen);         // data	size in bytes

#endif

// ---------------------------------------------------------------------------------------------
// END OF UNIT
// ---------------------------------------------------------------------------------------------
