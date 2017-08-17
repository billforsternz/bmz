// lengths.h, gather a few message length #defines together
//  note: this could be made more comprehensive
#ifndef LENGTHS_H
#define LENGTHS_H
#define ETHADDR_LEN         6
#define IPADDR_LEN          4
#define DEFAULT_MQ_DEPTH    8
#define DEFAULT_POOL_NBR    4
#define DEFAULT_POOL_LEN    500
#define DEFAULT_POOL_OFFSET 54
#define ETH_OFFSET   (ETHADDR_LEN + ETHADDR_LEN + 2)
#define ETH_MINFRAME 60 // eth header plus 46 bytes of data
#endif  // LENGTHS_H
