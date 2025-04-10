/*
   **Благие пожелания от DeepSeek`а:**

1. **Разделение задач**:
   - Чтение UART с детектированием фреймов по таймауту
   - Обработка фреймов с проверкой CRC
   - Формирование и отправка ответов

2. **Синхронизация**:
   - Мьютекс для защиты операций записи в UART
   - Очередь FreeRTOS для передачи фреймов между задачами

3. **Проверки ошибок**:
   - Контроль переполнения буфера
   - Проверка CRC входящих фреймов
   - Обработка ошибок выделения памяти
   - Защита от переполнения очереди

4. **Безопасность памяти**:
   - Динамическое выделение памяти для каждого фрейма
   - Гарантированное освобождение памяти

5. **Соответствие протоколу**:
   - Правильный расчет времени таймаута (3.5 символа)
   - Корректная реализация CRC16 для МАГИСТРАЛЬНОГО ПРОТОКОЛА СПСеть.
            см. Руководство программиста РАЖГ.00276-33
*/


#include "processor.h"
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

// Структура для передачи фреймов между задачами
typedef struct
{
    uint8_t *data;
    size_t length;
} mb_frame_t;

/* Обмен может выполняться на скоростях (бит/с.): */
uint32_t bauds[10] = {300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};

static const char *TAG = "PROCESSOR";

extern QueueHandle_t modbus_queue;
extern QueueHandle_t processor_queue;

void uart_sp_init()
{
    /* Configure parameters of an UART driver, communication pins and install the driver */
    uart_config_t uart_sp_config = 
    {
        .baud_rate = SP_BAUD_RATE,
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

    ESP_ERROR_CHECK(uart_driver_install(SP_PORT_NUM, BUF_SIZE, BUF_SIZE, SP_QUEUE_SIZE, NULL, 0));
    ESP_ERROR_CHECK(uart_set_pin(SP_PORT_NUM, CONFIG_SP_UART_TXD, CONFIG_SP_UART_RXD, CONFIG_SP_UART_RTS, 32));   // IO32 свободен (трюк)
    ESP_ERROR_CHECK(uart_set_mode(SP_PORT_NUM, UART_MODE_RS485_HALF_DUPLEX)); // activate RS485 half duplex in the driver
    ESP_ERROR_CHECK(uart_param_config(SP_PORT_NUM, &uart_sp_config));  
    ESP_LOGI(TAG, "sp_uart initialized.");
}

/* Функция вычисляет и возвращает циклический код для последовательности из len байтов, указанной *msg.
  Используется порождающий полином: (X в степени 16)+(X в степени 12)+(X в степени 5)+1.
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


// Задача обработки полученного по модбасу фрейма (без MB CRC)
void frame_processor_task(void *arg) 
{
    pdu_packet_t pdu;

    while(1) 
    {
        // Прием сообщения от modbus_receive_task
        if(xQueueReceive(modbus_queue, &pdu, portMAX_DELAY)) 
        {
            // Обработка данных
            ESP_LOGI(TAG, "Received PDU (%d bytes):", pdu.length);
            for(int i = 0; i < pdu.length; i++) 
            {
                printf("%02X ", pdu.data[i]);
            }
            printf("\n");
           
            // #ifdef TEST_MODBUS
            //     // Отправка в очередь
            //     if(xQueueSend(processor_queue, &pdu, pdMS_TO_TICKS(100)) != pdPASS) 
            //     {


            //     // if (xQueueSend(processor_queue, &pdu, 0) != pdTRUE)
            //     // {
            //     //     // if(xQueueSend(queues->modbus_queue, &pdu, 0) != pdTRUE) {     //if(xQueueSend(queues->modbus_queue, &pdu, pdMS_TO_TICKS(100)) != pdPASS) {
            //         ESP_LOGE(TAG, "Queue full! Dropping PDU");
            //         free(pdu.data);
            //  //       continue;
            //     }
            // #endif


            // далее staffing с выбором параметров обмена с целевым прибором
            // отправка фрейма по uart2
            // приём ответа, дестаффинг
            // отправка в очередь to_modbus_queue

    ledsGreen();        

            free(pdu.data); // Освобождение памяти
        }
    }
}
