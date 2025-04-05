#include "slave.h"
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


static const char *TAG = "MODBUS_SLAVE";

//static QueueHandle_t modbus_queue;
QueueHandle_t modbus_queue;

static portMUX_TYPE param_lock = portMUX_INITIALIZER_UNLOCKED;

void uart_mb_init()
{
    /* Configure parameters of an UART driver, communication pins and install the driver */
    uart_config_t uart_mb_config = {
        .baud_rate = MB_BAUD_RATE,
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


    ESP_ERROR_CHECK(uart_driver_install(MB_PORT_NUM, BUF_SIZE, BUF_SIZE, MB_QUEUE_SIZE, NULL, 0));

    ESP_ERROR_CHECK(uart_set_pin(MB_PORT_NUM, CONFIG_MB_UART_TXD, CONFIG_MB_UART_RXD, CONFIG_MB_UART_RTS, 32));   // IO32 свободен (трюк)

    ESP_ERROR_CHECK(uart_set_mode(MB_PORT_NUM, UART_MODE_RS485_HALF_DUPLEX)); // activate RS485 half duplex in the driver

    ESP_ERROR_CHECK(uart_param_config(MB_PORT_NUM, &uart_mb_config));  

    ESP_LOGI(TAG, "slave_uart initialized.");
}

static uint16_t mb_crc16(const uint8_t *buffer, size_t length) {
    uint16_t crc = 0xFFFF;
    
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)buffer[i];
        
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc;
}


/*  Задача приёма пакета по Modbus, его проверка и отправка в очередь для стаффинга */
void modbus_receive_task(void *arg) {

    //TaskQueues *queues = (TaskQueues*)arg;
    // Создание очереди
    //queues->modbus_queue = xQueueCreate(MB_QUEUE_SIZE, sizeof(pdu_packet_t));
    modbus_queue = xQueueCreate(MB_QUEUE_SIZE, sizeof(pdu_packet_t));

    uint8_t *frame_buffer = NULL;
    uint16_t frame_length = 0;
    uint32_t last_rx_time = 0;

    while(1) {
        uint8_t temp_buf[128];
        int len = uart_read_bytes(MB_PORT_NUM, temp_buf, sizeof(temp_buf), pdMS_TO_TICKS(10));

        if(len > 0) {
            // Начало нового фрейма
            if(frame_buffer == NULL) {
                frame_buffer = malloc(MAX_PDU_LENGTH + 3);
                frame_length = 0;
            }

            // Проверка переполнения буфера
            if(frame_length + len > MAX_PDU_LENGTH + 3) {
                ESP_LOGE(TAG, "Buffer overflow! Discarding frame");
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                continue;
            }

            memcpy(frame_buffer + frame_length, temp_buf, len);
            frame_length += len;
            last_rx_time = xTaskGetTickCount();
        }

        // Проверка завершения фрейма
        if(frame_buffer && (xTaskGetTickCount() - last_rx_time) > pdMS_TO_TICKS(MB_FRAME_TIMEOUT_MS)) {
            // Минимальная длина фрейма: адрес + функция + CRC
            if(frame_length < 4) {
                ESP_LOGE(TAG, "Invalid frame length: %d", frame_length);
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                continue;
            }
    ledsRed();
            // Проверка адреса
            if(frame_buffer[0] != SLAVE_ADDRESS) {
                ESP_LOGW(TAG, "Address mismatch: 0x%02X", frame_buffer[0]);
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                continue;
            }
  //  ledsGreen();
            // Проверка CRC
            uint16_t received_crc = (frame_buffer[frame_length-1] << 8) | frame_buffer[frame_length-2];
            uint16_t calculated_crc = mb_crc16(frame_buffer, frame_length - 2);
            
            if(received_crc != calculated_crc) {
                ESP_LOGE(TAG, "CRC error: %04X vs %04X", received_crc, calculated_crc);
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                continue;
            }

            // Подготовка PDU пакета
            pdu_packet_t pdu = {
                .length = frame_length - 3,  // Исключаем адрес и CRC
                .data = malloc(frame_length - 3)
            };

            if(pdu.data == NULL) {
                ESP_LOGE(TAG, "Memory allocation failed!");
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                continue;
            }

            memcpy(pdu.data, frame_buffer + 1, pdu.length);
            
    ledGreenToggle(); // По факту отправки в очередь
    ledBlueToggle();


            // Отправка в очередь
            if(xQueueSend(modbus_queue, &pdu, 0) != pdTRUE) {
            //if(xQueueSend(queues->modbus_queue, &pdu, 0) != pdTRUE) {     //if(xQueueSend(queues->modbus_queue, &pdu, pdMS_TO_TICKS(100)) != pdPASS) {
                ESP_LOGE(TAG, "Queue full! Dropping PDU");
                free(pdu.data);
            }

            free(frame_buffer);
            frame_buffer = NULL;
            frame_length = 0;
        }
    }
}

// // Задача обработки PDU
// void processing_task(void *arg) {
//     while(1) {
//         pdu_packet_t pdu;
//         if(xQueueReceive(modbus_queue, &pdu, portMAX_DELAY)) {
//             // Обработка данных
//             ESP_LOGI(TAG, "Received PDU (%d bytes):", pdu.length);
//             for(int i = 0; i < pdu.length; i++) {
//                 printf("%02X ", pdu.data[i]);
//             }
//             printf("\n");
            



//             free(pdu.data); // Освобождение памяти
//         }
//     }
// }