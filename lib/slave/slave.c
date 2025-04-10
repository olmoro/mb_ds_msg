/*
 *      Ключевые проверки и особенности:
 *   1.  Валидация входных параметров
 *   2.  Проверка успешности создания очереди
 *   3.  Обработка таймаута при чтении
 *   4.  Проверка целостности данных в сообщении
 *   5.  Контроль размера буфера получателя
 *   6.  Безопасное копирование данных
 *   7.  Возврат разных кодов ошибок для диагностики - не реализовано
 *   8.  Использование структуры с размером данных для защиты от переполнения - не реализовано
 *
 *       Индикация:
 *               Blue        - ожидание ввода нового пакета modbus;
 *               Red         - чужой адрес, ошибка CRC, ошибка выделения памяти, очередь переполнена или
 *                             возвращён пакет с установленным битом ошибки;
 *               Green       - пакет отправлен в очередь на обработку;
 */

#include "slave.h"
#include "board.h"
#include "project_config.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>                 // for standard int types definition
#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"


static const char *TAG = "MODBUS_SLAVE";

// Структура для передачи фреймов между задачами
typedef struct {
    uint8_t *data;
    size_t length;
} mb_frame_t;

QueueHandle_t modbus_queue;
static SemaphoreHandle_t uart_mutex = NULL;

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

//     int intr_alloc_flags = 0;

// #if CONFIG_UART_ISR_IN_IRAM
//     intr_alloc_flags = ESP_INTR_FLAG_IRAM;
// #endif

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

/* */
static void send_mb_err()
{
   
    //     /* Если mb_err == true, то пакет принят с ошибкой, либо очередь на обработку пакета
    //       переполнена. Формируется ответ c установленным старшим битом в коде команды */
    //     uint8_t response[4] =
    //         {
    //             pdu.data[0],         // Адрес
    //             pdu.data[1] |= 0x80, // Функция
    //             0x00, 0x00           // Место для CRC
    //         };

    //     uint8_t len = sizeof(response);

    //     /* Расчет CRC для ответа */
    //     uint16_t response_crc = mb_crc16(response, len - 2);
    //     response[2] = response_crc & 0xFF;
    //     response[3] = response_crc >> 8;

    //     ESP_LOGI(TAG, "Response (%d bytes):", len);
    //     for (int i = 0; i < len; i++)
    //     {
    //         printf("%02X ", response[i]);
    //     }
    //     printf("\n");

    //     /* Отправка ответа с синхронизацией */
    //     xSemaphoreTake(uart_mutex, portMAX_DELAY);
    //     uart_write_bytes(MB_PORT_NUM, (const char *)response, sizeof(response));
    //     xSemaphoreGive(uart_mutex);

        ledsBlue();     // Ожидание ввода нового пакета
    //    free(pdu.data); // Освобождение памяти после обработки сообщения

   
}

/*  Задача приёма пакета по Modbus, его проверка и отправка в очередь для стаффинга */
void modbus_receive_task(void *arg) 
{
    // Создание очереди и мьютекса
    modbus_queue = xQueueCreate(MB_QUEUE_SIZE, sizeof(pdu_packet_t));

    uart_mutex = xSemaphoreCreateMutex();
    if(!uart_mutex) 
    {
        ESP_LOGE(TAG, "Mutex creation failed");
        return;
    }

    uint8_t *frame_buffer = NULL;
    uint16_t frame_length = 0;
    uint32_t last_rx_time = 0;

    while(1) 
    {
        uint8_t temp_buf[128];
        uint8_t mb_err = 0x00; // Ошибок приёма пакета по modbus нет

        int len = uart_read_bytes(MB_PORT_NUM, temp_buf, sizeof(temp_buf), pdMS_TO_TICKS(10));

        if(len > 0) 
        {
            // Начало нового фрейма
            if(frame_buffer == NULL) 
            {
                frame_buffer = malloc(MAX_PDU_LENGTH + 3);
                frame_length = 0;
            }

            // Проверка переполнения буфера
            if(frame_length + len > MAX_PDU_LENGTH + 3) {
                ESP_LOGE(TAG, "Buffer overflow! Discarding frame");
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                mb_err = 0x04; // continue;
            }

            memcpy(frame_buffer + frame_length, temp_buf, len);
            frame_length += len;
            last_rx_time = xTaskGetTickCount();
        }

        // Проверка завершения фрейма
        if(frame_buffer && (xTaskGetTickCount() - last_rx_time) > pdMS_TO_TICKS(MB_FRAME_TIMEOUT_MS)) {
            // Минимальная длина фрейма: адрес + функция + CRC
            if(frame_length < 4) 
            {
                ESP_LOGE(TAG, "Invalid frame length: %d", frame_length);
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                mb_err = 0x04; // continue;
            }
  //  ledsRed();
            // Проверка адреса
            if(frame_buffer[0] != SLAVE_ADDRESS) 
            {
                ESP_LOGW(TAG, "Address mismatch: 0x%02X", frame_buffer[0]);
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                mb_err = 0x04; // continue;
            }
  //  ledsGreen();
            // Проверка CRC
            uint16_t received_crc = (frame_buffer[frame_length-1] << 8) | frame_buffer[frame_length-2];
            uint16_t calculated_crc = mb_crc16(frame_buffer, frame_length - 2);
            
            if(received_crc != calculated_crc) 
            {
                ESP_LOGE(TAG, "CRC error: %04X vs %04X", received_crc, calculated_crc);
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                mb_err = 0x04; // continue;
            }

                    // Подготовка PDU пакета
            pdu_packet_t pdu = 
            {
                .length = frame_length - 2,  // Исключаем CRC
                .data = malloc(frame_length - 2)
            };

            if(pdu.data == NULL) 
            {
                ESP_LOGE(TAG, "Memory allocation failed!");
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                mb_err = 0x04; // continue;;
            }

            memcpy(pdu.data, frame_buffer, pdu.length);
 
    ledGreenToggle(); // По факту отправки в очередь
    ledBlueToggle();

    //mb_err = 0x04;

            if(mb_err == 0x00)
            {
                // Отправка в очередь
                if(xQueueSend(modbus_queue, &pdu, 0) != pdTRUE) 
                {
                //if(xQueueSend(queues->modbus_queue, &pdu, 0) != pdTRUE) {     //if(xQueueSend(queues->modbus_queue, &pdu, pdMS_TO_TICKS(100)) != pdPASS) {
                    ESP_LOGE(TAG, "Queue full! Dropping PDU");
                    free(pdu.data);
                    continue;
                }
            }
            else
            {
                /* Если mb_err == true, то пакет принят с ошибкой, либо очередь на обработку пакета
                    переполнена. Формируется ответ c установленным старшим битом в коде команды */
                uint8_t response[5] =
                {
                    pdu.data[0],            // Адрес
                    pdu.data[1] |= 0x80,    // Функция
                    pdu.data[2] = mb_err,   // Код ошибки
                    0x00, 0x00              // Место для CRC
                };
                uint8_t len = sizeof(response);

                /* Расчет CRC для ответа */
                uint16_t response_crc = mb_crc16(response, len - 2);
                response[3] = response_crc & 0xFF;
                response[4] = response_crc >> 8;

                ESP_LOGI(TAG, "Response (%d bytes):", len);
                for (int i = 0; i < len; i++)
                {
                    printf("%02X ", response[i]);
                }
                printf("\n");

                /* Отправка ответа с синхронизацией */
                xSemaphoreTake(uart_mutex, portMAX_DELAY);
                uart_write_bytes(MB_PORT_NUM, (const char *)response, sizeof(response));
                xSemaphoreGive(uart_mutex);

                ledsBlue();     // Ожидание ввода нового пакета
                free(pdu.data); // Освобождение памяти после обработки сообщения
            }

            free(frame_buffer);
            frame_buffer = NULL;
            frame_length = 0;
        }
    }
}


// Задача отправления ответа
void mb_send_task(void *arg) 
{
    pdu_packet_t pdu;

    while(1) 
    {
       // mb_frame_t frame;   // структура

        //if(xQueueReceive(frame_queue, &frame, portMAX_DELAY) 
        if(xQueueReceive(modbus_queue, &pdu, portMAX_DELAY))        // Test
        {

            // Обработка данных
            ESP_LOGI(TAG, "Received PDU (%d bytes):", pdu.length);
            for(int i = 0; i < pdu.length; i++) 
            {
                printf("%02X ", pdu.data[i]);
            }
            printf("\n");
           
                //ledsGreen();        
            ledsOn();        

            // // Проверка минимальной длины фрейма
            // if(pdu.length < 4) {
            //     ESP_LOGE(TAG, "Invalid pdu length: %d", pdu.length);
            //     free(pdu.data);
            //     continue;
            // }

            // Формирование ответа
        
            uint8_t response[pdu.length + 2];
            memcpy(response, pdu.data, pdu.length);

            // Расчет CRC для ответа
            uint16_t response_crc = mb_crc16(response, pdu.length);
            response[pdu.length] = response_crc & 0xFF;
            response[pdu.length + 1] = response_crc >> 8;
       
            ESP_LOGI(TAG, "Response (%d bytes):", pdu.length+2);
            for(int i = 0; i < pdu.length + 2; i++) 
            {
                printf("%02X ", response[i]);
            }
            printf("\n");

            // Отправка ответа с синхронизацией
            xSemaphoreTake(uart_mutex, portMAX_DELAY);
            uart_write_bytes(MB_PORT_NUM, (const char *)response, sizeof(response));
            xSemaphoreGive(uart_mutex);

            ledsBlue();        

            free(pdu.data);
        }
    }
}
