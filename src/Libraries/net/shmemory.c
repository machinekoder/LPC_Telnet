


#include "net_includes.h"

#if 0

volatile LLC_HEADER* pSHM_IP = (void *) 0;     // shared memory for IP-packet pointer

OS_EVENT* SEM_IP_READ;
OS_EVENT* SEM_IP_WRITE;

void shmemory_init(void)
{

  // create read semaphore, no read access allowed at startup
  SEM_IP_READ = OSSemCreate(0);
  #if DEBUG
    if(SEM_IP_READ == NULL) {
      uart_puts("no mutex semaphore created!\n");
    }
  #endif

  // create write semaphore, write access allowed at startup
  SEM_IP_WRITE = OSSemCreate(1);
  #if DEBUG
    if(SEM_IP_WRITE == NULL) {
      uart_puts("no mutex semaphore created!\n");
    }
  #endif

  pSHM_IP = (void *)0;
}


INT8S shmemory_put(LLC_HEADER* pSHM)
{
  INT8S ret = -1;
  INT8U mutex_ret = 0;

  // try to get acces to shared memory
  mutex_ret = OSSemAccept(SEM_IP_WRITE);

  if(mutex_ret != 0) {

    if(pSHM_IP == (void *) 0) {
      pSHM_IP = pSHM;
      ret = 0;
    }
    else {
      ret = -1;
    }
  }
  else {
    ret =-1;
  }

  OSSemPost(SEM_IP_READ);

  return ret;
}


LLC_HEADER* shmemory_get(void)
{
  LLC_HEADER *pFrame = (void *) 0;
  INT8U err;

  OSSemPend(SEM_IP_READ, 0, &err);

  #if DEBUG
    switch(err) {
      case      OS_ERR_NONE :                                   break;
      default:          uart_puts("Sem Mutex pend error\n");    break;
    }
  #endif

  if(pSHM_IP != (void *) 0) {
    pFrame = (LLC_HEADER*)pSHM_IP;
    pSHM_IP = (void *) 0;
  }

  err = OSSemPost(SEM_IP_WRITE);

  #if DEBUG
    switch(err) {
      case    OS_ERR_NONE :                                     break;
      default:          uart_puts("Sem Mutex post error\n");    break;
    }
  #endif

  return pFrame;
}

#endif /*0*/
