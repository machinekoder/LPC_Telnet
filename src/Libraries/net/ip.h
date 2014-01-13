// ---------------------------------------------------------------------------------------------
// PROJECT:     TCP Stack for Keil MCB167-NET
// UNIT:        IP.H
// PURPOSE:     OSI level 3 procedures are designed herein. Up to now IP and ICMP echo reply
//              messages are supported. Calls functions of higher levels, such as TCP routines.
// AUTHOR:      Martin Zauner, Matr.Nr.9911011114
// COPYRIGHT:   (c)2003, Fachhochschule Technikum Wien, Modul Telekom und
//              Computer & Systemtechnik
// LAST UPDATE: 24-04-2003
// ---------------------------------------------------------------------------------------------

#ifndef IP_PROC_DEFINES
 #define IP_PROC_DEFINES

#include "utility_procs.h"       // prototyes definition swap byte order

// --------------------------------------------------------------------------------------------
// IP Header definition, options field is located at Data[0] (if used)
// --------------------------------------------------------------------------------------------

typedef struct ip_header {           // total 20 bytes (without Data)
  INT16U Vht;                        // IP version, header size, ToS (ignore)
  INT16U Length;                     // total datagram length (includes header)
  INT16U Identification;             // this fragment is dedicated to segment no.
  INT16U Frag;                       // 3bit flags + 13bit fragment offset
  INT16U ttlProtocol;                // 8 bit TTL + 8 bit protocol :ICMP=1,TCP=6,UDP=17
  INT16U Checksum;                   // WORD16 checksum
  IP_A   source;                     // source address, 32bit
  IP_A   dest;                       // target for this datagram, 32bit address
  //--
  INT16U Data[1];                    // options (optional) plus data
} __attribute__ ((packed)) IP_HEADER;

// --------------------------------------------------------------------------------------------
// ICMP Header Definition
// --------------------------------------------------------------------------------------------

typedef struct icmp_header {        // 8 bytes without "Data"
  INT8U   ICMP_Type;                // type of ICMP message
  INT8U   Code;                     // code (for time exceed msg)
  INT16U  Checksum;                 // checksum
  INT16U  Identifier;               // echo identifier
  INT16U  Sequence;                 // echo sequence
//--
  INT16U  Data[1];                  // ICMP data
} __attribute__ ((packed)) ICMP_HEADER;

typedef struct icmp_control {
  INT16U Identification;           // ICMP identification process
  INT16U SequenceNumber;           // ICMP datagram sequence
  INT16U RequestFlag;              // Receive/Send synchronisation
  INT16U FragmentID;               // IP fragment identification
  IP_A   Address;                  // IP address
} __attribute__ ((packed)) ICMP_CONTROL;


// ---------------------------------------------------------------------------------------------
// function:   void IP_SendDatagram (IP_A IP_Address, INT16U IP_Protocol, INT16U Identification,
//                                   INT16U FragmentOffset, INT16U huge *Data, INT16U Datalen)
// parameters: <IP_Address>     IP address where to send the datagram
//             <IP_Protocol>    protocol type of the IP payload eg. TCP or UDP type
//             <Identification> identification of the TCP/UDP fragment
//             <FragmentOffset> fragment offset (ignored)
//             <*Data>          pointer to IP data area, segments may be placed here
//             <Datalen>        data length
// result:     <none>
// purpose:               creates a new datagram using the *Data payload pointer
//                        and launches transmission by calling ETHER_Sendframe
//
// note:       1.) fragmentation is not supported with this version!
//             2.) uses IP version 4 only
// ---------------------------------------------------------------------------------------------

void IP_SendDatagram (IP_A IP_Address,         // address where to send datagram
                      INT16U IP_Protocol,      // type of message (ICMP, TCP)
                      INT16U Identification,   // fragment ID
                      INT16U FragmentOffset,   // fragment offset
                      INT16U *Data,       // Message to be embedded into IP datagram
                      INT16U Datalen,          // Length of IP data
                      void * pLevel4Buf);      // pointer holding dynamic memory block

// ---------------------------------------------------------------------------------------------
// function:    void IP_AllTypesProcess (IP_HEADER huge *pIP)
// parameters:  <*pIP>       pointer to incoming IP datagram
// result:      <none>
// purpose:            reads the protocol field of the datagram to determine
//                     the transported payload type, then forwards the datagram
//                     to the general TCPIP or ICMP processing routine.
// ---------------------------------------------------------------------------------------------

void IP_AllTypesProcess (IP_HEADER *pIP);// Parameter: pointer to incoming IP

#endif

// ---------------------------------------------------------------------------------------------
// END OF UNIT
// ---------------------------------------------------------------------------------------------
