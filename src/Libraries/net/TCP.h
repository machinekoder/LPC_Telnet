// ---------------------------------------------------------------------------------------------
// PROJECT:     TCP Stack for Keil MCB167-NET
// UNIT:        TCP.H
// PURPOSE:     Basic TCP unit, provides the universal segment send primitive <TCP_SendSegment>
//              and the decoding routine TCP_Process, which forwards incoming TCP information to
//              the state machine.
// AUTHOR:      Martin Zauner, Matr.Nr.9911011114
// COPYRIGHT:   (c)2003, Fachhochschule Technikum Wien, Modul Telekom und
//              Computer & Systemtechnik
// LAST UPDATE: 24-04-2003
// ---------------------------------------------------------------------------------------------

#ifndef TCP_PROC_DEFINES
 #define TCP_PROC_DEFINES

#include "utility_procs.h"       // prototyes definition swap byte order

// --------------------------------------------------------------------------------------------
// TCP Pseudo Header Definition
// --------------------------------------------------------------------------------------------

typedef struct tcp_pseudo_header {   // 12 bytes pseudo header
 IP_A   source;                      // source IP address
 IP_A   dest;                        // destination IP address
 INT8U  dummy;                       // dummy space
 INT8U  protocol;                    // level 4 protocol indicator (6=TCP)
 INT16U TCPlength;                   // length of TCP segment
 INT16U TCPheader;                   // will be overlaid with TCP header
} __attribute__ ((packed)) TCP_PSEUDO_HEADER;

// --------------------------------------------------------------------------------------------
// TCP Header Definition
// --------------------------------------------------------------------------------------------

typedef struct tcp_header {
 INT16U    sourceport;               // port ID, source
 INT16U    destport;                 // port ID, destination
 INT32U    seqnum;                   // SEQ number
 INT32U    acknum;                   // ACK number
 INT16U    offs_res_flags;           // offset + reserved + flags
 INT16U    window;                   // window size
 INT16U    checksum;                 // checksum
 INT16U    urgentptr;                // urgent pointer
 // 20 byte plus options
 INT32U    options;                  // options field, 4 bytes
 // 24 byte plus data:               // if no options, then data starts here
 INT16U    data;                     // data origin, if options are 4 bytes long
} __attribute__ ((packed)) TCP_HEADER;

// ---------------------------------------------------------------------------------------------
// function:    void TCP_SendSegment  (IP_A IP_Address, INT16U Sourceport, INT16U Destport,
//                                     INT32U SEQnum, INT32U ACKnum, INT16U Flags, INT16U WinSize,
//                                     int  MaxSegSize, INT16U huge *Data, INT16U Datalength)
// parameters:  <IP_Address>    target IP address
//              <Sourceport>    own port
//              <Destport>      destination port
//              <SEQnum>        sequence number
//              <ACKnum>        acknowledge number
//              <Flags>         flags to be sent
//              <WinSize>       desired window size
//              <MaxSegSize>    maximum segment size
//              <*Data>         pointer to payload
//              <Datalength>    length of payload
// result:      <none>
// purpose:              TCP segment send procedure, builds a new packet due to
//                       the desired parameters and forwards it down to IP level
// ---------------------------------------------------------------------------------------------

void TCP_SendSegment(IP_A IP_Address, INT16U Sourceport,
                     INT16U Destport, INT32U SEQnum, INT32U ACKnum,
                     INT16U Flags, INT16U WinSize, int  MaxSegSize,
                     INT16U *Data, INT16U Datalength);

// ---------------------------------------------------------------------------------------------
// function:   void TCP_Process (TCP_HEADER huge *pTCP, INT16U TCPlen, INT32U IPaddress) {
// parameters: <*pTCP>       pointer to received segment
//             <TCPlen>      length of segment
//             <IPaddress>   source address of segment
// result:     <none>
// purpose:             extracts fields into readable variables, gets options and
//                      finds out max. seg size if desired, then moves data into
//                      a local buffer and informs the TCP central.
// ---------------------------------------------------------------------------------------------

void TCP_Process (TCP_HEADER *pTCP, INT16U TCPlen, INT32U IPaddress);

#endif // TCP_PROC_DEFINES

// -------------------------------------------------------------------------------------------
// END OF UNIT
// -------------------------------------------------------------------------------------------


