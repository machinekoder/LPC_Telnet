// ---------------------------------------------------------------------------------------------
// protocols.h
// ---------------------------------------------------------------------------------------------

#ifndef _PROTOCOLS_H_
#define _PROTOCOLS_H_

#include "type.h"

// ---------------------------------------------------------------------------------------------
// DEFINES
// ---------------------------------------------------------------------------------------------

#ifndef MAX_PACKET_SIZE
#define MAX_PACKET_SIZE 1520
#endif

//convert little endian to big endian
#define LITTLE2BIG16(x) (((x>>8)&0x000000FF) | ((x<<8)&0x0000FF00))

#define ETH_ALEN  6
#define IP_LEN    4

#define APPL_ETH_PROT 	(0x8866)
#define APPL_IP_PROT  	(0x0800)
#define APPL_ARP_PROT 	(0x0806)

// ---------------------------------------------------------------------------------------------
// TYPEDEFS
// ---------------------------------------------------------------------------------------------

typedef union mac_a {
  INT8U       b[6];
  INT16U      w[3];
} MAC_A;

typedef union ip_a {
  INT8U       b[4];
  INT16U      w[2];
  INT32U      d;
} IP_A;

typedef struct ethernet_header {
  MAC_A             Destination;           	// 48 bit destination address
  MAC_A             Source;               	// 48 bit source address
  INT16U            Protocol;		           	// 16 bit ethernet type
  INT8U             Data[1];               															// data origin
} __attribute__ ((packed)) ETHERNET_HEADER;

typedef struct llc_header {
  INT32U           Bytes;								// number of bytes //for 4byte address alignment
  INT8U            Data[1];                															// data origin
} __attribute__ ((packed)) LLC_HEADER;


// ---------------------------------------------------------------------------------------------
// EOF
// ---------------------------------------------------------------------------------------------

#endif /* _PROTOCOLS_H_ */
