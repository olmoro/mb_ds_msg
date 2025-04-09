


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

/* Обмен может выполняться на скоростях 
    300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200 бит/с.*/
uint32_t bauds[10] = {300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};

static const char *TAG = "PROCESSING";

extern QueueHandle_t modbus_queue;

void uart_sp_init()
{
    /* Configure parameters of an UART driver, communication pins and install the driver */
    uart_config_t uart_sp_config = {
        .baud_rate = SP_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,  // RTS для управления направлением DE/RE !!
        .rx_flow_ctrl_thresh = 122,
    };

    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif


    ESP_ERROR_CHECK(uart_driver_install(SP_PORT_NUM, BUF_SIZE, BUF_SIZE, SP_QUEUE_SIZE, NULL, 0));

    ESP_ERROR_CHECK(uart_set_pin(SP_PORT_NUM, CONFIG_SP_UART_TXD, CONFIG_SP_UART_RXD, CONFIG_SP_UART_RTS, 32));   // IO32 свободен (трюк)

    ESP_ERROR_CHECK(uart_set_mode(SP_PORT_NUM, UART_MODE_RS485_HALF_DUPLEX)); // activate RS485 half duplex in the driver

    ESP_ERROR_CHECK(uart_param_config(SP_PORT_NUM, &uart_sp_config));  

    ESP_LOGI(TAG, "sp_uart initialized.");

}

/* Функция вычисляет и возвращает циклический код для 
  последовательности из len байтов, указанной *msg.
  Используется порождающий полином:
  (X в степени 16)+(X в степени 12)+(X в степени 5)+1.
  Полиному соответствует битовая маска 0x1021.
*/
int CRCode(char *msg, int len)
{
    int j, crc = 0;
    while (len-- > 0)
    {
        crc = crc ^ (int)*msg++ << 8;
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ CRC_INIT;    //0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}


// // Задача обработки PDU
// void processing_task(void *arg) {
//   //  TaskQueues *queues = (TaskQueues*)arg;
//     pdu_packet_t pdu;

//     while(1) 
//     {
//         // Прием сообщения от modbus_receive_task
//         //if(xQueueReceive(queues->modbus_queue, &pdu, portMAX_DELAY)) {
//         if(xQueueReceive(modbus_queue, &pdu, portMAX_DELAY)) {
//             // Обработка данных
//             ESP_LOGI(TAG, "Received PDU (%d bytes):", pdu.length);
//             for(int i = 0; i < pdu.length; i++) {
//                 printf("%02X ", pdu.data[i]);
//             }
//             printf("\n");
           
//  //   ledsGreen();        

//             free(pdu.data); // Освобождение памяти
//         }
//     }
// }
