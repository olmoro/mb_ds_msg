/*
   ---------------------------------------------------------------------------------
                            Файл конфигурации проекта
   ---------------------------------------------------------------------------------
*/

#pragma once

#include <stdint.h>
#include "esp_task.h"

#include <time.h>
#include <stdio.h>
//#include <stdint.h>
#include <stdbool.h>
#include "esp_bit_defs.h"
// ---------------------------------------------------------------------------------
//                                  Версии 
// ---------------------------------------------------------------------------------
#define APP_VERSION "MB_DS_MSG 20250404.05"
// 202500404.04 05:  add: slave, processing                 RAM:  3.4%  Flash: 13.0%  
// 202500404.02:  Received PDU (5 bytes): 06 00 01 00 01    RAM:  3.4%  Flash: 12.8%  
// 202500403.01:  Проверка DeepSeek msg                     RAM:  3.4%  Flash: 12.8%



// ---------------------------------------------------------------------------------
//                                  GPIO
// ---------------------------------------------------------------------------------
  // Плата SPN.55
  // Светодиоды
#define RGB_RED_GPIO 4   // Красный, катод на GND (7mA)
#define RGB_GREEN_GPIO 2 // Зелёный, катод на GND (5mA)
#define RGB_BLUE_GPIO 27 // Синий,   катод на GND (4mA)

// Входы
#define CONFIG_GPIO_IR 19 // Вход ИК датчика

 
    // // UART1
    // #define CONFIG_UART1_RXD          25
    // #define CONFIG_UART1_TXD          26
    // #define CONFIG_UART1_RTS          33
  
    // // UART2
    // #define CONFIG_UART2_RXD          21
    // #define CONFIG_UART2_TXD          23
    // #define CONFIG_UART2_RTS          22

// ---------------------------------------------------------------------------------
//                                    MODBUS 
// ---------------------------------------------------------------------------------

#define MB_PORT_NUM UART_NUM_1
#define BAUD_RATE 9600
#define CONFIG_MB_UART_RXD 25
#define CONFIG_MB_UART_TXD 26
#define CONFIG_MB_UART_RTS 33
#define SLAVE_ADDRESS 0x01
#define MAX_PDU_LENGTH 180
#define QUEUE_SIZE 10
#define FRAME_TIMEOUT_MS 4 // 3.5 символа при 19200 бод

#define BUF_SIZE (512) // размер буфера

// Структура для PDU
typedef struct {
    uint8_t *data;
    uint16_t length;
} pdu_packet_t;

// Test
// #define CONFIG_SLAVE_TASK_STACK_SIZE  1024 * 4
// #define CONFIG_SLAVE_TASK_PRIORITY    10

// #define CONFIG_EVENTS_TASK_STACK_SIZE  1024 * 4
// #define CONFIG_EVENTS_TASK_PRIORITY    10



// // // Буферы для регистров
// // static uint16_t holding_regs[10] = {0};  // Holding Registers (4xxxx)

// #define QUEUE_LENGTH 10        // Максимальное количество элементов в очереди
// #define ITEM_SIZE sizeof(modbus_packet_t)

// Test
#define CONFIG_STAFF_TASK_STACK_SIZE 1024 * 4
#define CONFIG_STAFF_TASK_PRIORITY CONFIG_SLAVE_TASK_PRIORITY - 1 // 9

// typedef struct
// {
//     char mode;        // режим
//     uint32_t *buffer; // буфер данных
//     size_t size;      // размер данных
// } Message;

// /* Используются две очереди FreeRTOS для двусторонней коммуникации */
// QueueHandle_t modbus_queue;
// QueueHandle_t processing_queue;

// /* Используются две очереди FreeRTOS для двусторонней коммуникации */
// typedef struct
// {
//     QueueHandle_t modbus_queue;    // От Modbus к SPnet
//     QueueHandle_t processing_queue; // От SPnet к Modbus
// } TaskQueues;


// ---------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------
