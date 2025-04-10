#include <string.h>
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "project_config.h"
#include "board.h"
#include "slave.h"
#include "processing.h"

static const char *TAG = "MODBUS_MAIN";

void app_main()
{
    // Инициализация периферии
    boardInit();
    uart_mb_init();
    // uart_sp_init();

    /* Проверка RGB светодиода */
    ledsBlue();

    // Создание задач
    BaseType_t modbus_receive_task_handle = xTaskCreate(modbus_receive_task, "mb_receive", 4096, NULL, 5, NULL);
    if (!modbus_receive_task_handle)
    {
        ESP_LOGE(TAG, "Failed to create MB Receive task");
        return;
    }
    ESP_LOGI(TAG, "MB Receive task created successfully");

    BaseType_t modbus_send_task_handle = xTaskCreate(mb_send_task, "mb_send", 4096, NULL, 4, NULL);
    if (!modbus_send_task_handle)
    {
        ESP_LOGE(TAG, "Failed to create MB Send task");
        return;
    }
    ESP_LOGI(TAG, "MB Send task created successfully");

    // BaseType_t processing_task_handle = xTaskCreate(processing_task, "processing", 4096, NULL, 4, NULL);
    // if (!processing_task_handle)
    // {
    //     ESP_LOGE(TAG, "Failed to create Processing task");
    //     return;
    // }
    // ESP_LOGI(TAG, "Processing task created successfully");
}
