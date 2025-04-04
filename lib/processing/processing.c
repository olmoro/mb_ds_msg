


#include "processing.h"
#include "board.h"
//#include "target.h"
#include "project_config.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>                 // for standard int types definition
//#include <stddef.h>                 // for NULL and std defines
#include "esp_err.h"
// #include "mbcontroller.h"           // for mbcontroller defines and api
// #include "modbus_params.h"          // for modbus parameters structures
// #include "esp_modbus_slave.h"
#include "esp_log.h"
#include "sdkconfig.h"
// #include "soc/soc.h"                // for BITN definitions
// #include "freertos/FreeRTOS.h"      // for task creation and queues access
#include "freertos/task.h"
#include "freertos/queue.h"
// #include "freertos/event_groups.h"  // for event groups
// #include "esp_modbus_common.h"      // for common types
#include "driver/uart.h"

static const char *TAG = "PROCESSING";

extern QueueHandle_t modbus_queue;

void uart2_init()
{

}


// //void pdu_processing_task(void *arg);
// void pdu_processing_task(void *arg)
// {

// }

// Задача обработки PDU
void processing_task(void *arg) {
  //  TaskQueues *queues = (TaskQueues*)arg;
    pdu_packet_t pdu;

    while(1) 
    {
        // Прием сообщения от modbus_receive_task
        //if(xQueueReceive(queues->modbus_queue, &pdu, portMAX_DELAY)) {
        if(xQueueReceive(modbus_queue, &pdu, portMAX_DELAY)) {
            // Обработка данных
            ESP_LOGI(TAG, "Received PDU (%d bytes):", pdu.length);
            for(int i = 0; i < pdu.length; i++) {
                printf("%02X ", pdu.data[i]);
            }
            printf("\n");
           
    ledsGreen();        

            free(pdu.data); // Освобождение памяти
        }
    }
}
