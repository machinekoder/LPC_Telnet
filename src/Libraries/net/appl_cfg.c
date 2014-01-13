
#include "net_includes.h"

// own mac address
const MAC_A my_mac   = {.b[0]=0x5a, 0x01, 0x02, 0x03, 0x04, 0x05};

//mac mask
const MAC_A mac_mask = {.b[0]=0xff, 0xff, 0xff, 0xff, 0xff, 0x00};

// own ip address
const IP_A  my_ip = {.b[0]=192, 168, 5, 10};

// user defined destination mac address
const MAC_A dest_mac = {.b[0]=0x00, 0xE5, 0xC1, 0x67, 0x00, 0x01};
