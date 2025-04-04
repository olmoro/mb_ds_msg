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
#define APP_VERSION "MB_DS_MSG 20250404.02"
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

// Test
#define CONFIG_SLAVE_TASK_STACK_SIZE  1024 * 4
#define CONFIG_SLAVE_TASK_PRIORITY    10

#define CONFIG_EVENTS_TASK_STACK_SIZE  1024 * 4
#define CONFIG_EVENTS_TASK_PRIORITY    10

// // Структура для хранения Modbus пакета
// typedef struct
// {
//     uint8_t function;     // Modbus функция
//     uint8_t *data;        // Указатель на данные
//     uint16_t data_length; // Длина данных
// } modbus_packet_t;

// // // Буферы для регистров
// // static uint16_t holding_regs[10] = {0};  // Holding Registers (4xxxx)

// #define QUEUE_LENGTH 10        // Максимальное количество элементов в очереди
// #define ITEM_SIZE sizeof(modbus_packet_t)
