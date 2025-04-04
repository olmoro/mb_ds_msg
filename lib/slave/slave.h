/*=====================================================================================
 * Description:
 *  FreeModbus Slave ESP32
 *  
 *  
 *====================================================================================*/
#ifndef _SLAVE_H_
#define _SLAVE_H_



#ifdef __cplusplus
extern "C"
{
#endif

void uart1_init();

//void slave_task(void *arg);
void modbus_receive_task(void *arg);

void processing_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif // _SLAVE_H_
