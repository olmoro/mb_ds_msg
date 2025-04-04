/*=====================================================================================
 * Description:
 *  
 *  
 *  
 *====================================================================================*/
#ifndef _PROCESSING_H_
#define _PROCESSING_H_



#ifdef __cplusplus
extern "C"
{
#endif

void uart2_init();

// //void slave_task(void *arg);
// void uart_receive_task(void *arg);

//void pdu_processing_task(void *arg);
void processing_task(void *arg);


#ifdef __cplusplus
}
#endif

#endif // _PROCESSING_H_