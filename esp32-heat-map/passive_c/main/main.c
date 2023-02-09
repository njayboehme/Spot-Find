#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"

#include "../../_components/nvs_component.h"
#include "../../_components/sd_component.h"
#include "../../_components/csi_component.h"
#include "../../_components/time_component.h"
#include "../../_components/input_component.h"

#ifdef CONFIG_WIFI_CHANNEL
#define WIFI_CHANNEL CONFIG_WIFI_CHANNEL
#else
#define WIFI_CHANNEL 6
#endif

#ifdef CONFIG_SHOULD_COLLECT_CSI
#define SHOULD_COLLECT_CSI 1
#else
#define SHOULD_COLLECT_CSI 0
#endif

#ifdef CONFIG_SHOULD_COLLECT_ONLY_LLTF
#define SHOULD_COLLECT_ONLY_LLTF 1
#else
#define SHOULD_COLLECT_ONLY_LLTF 0
#endif

#ifdef CONFIG_SEND_CSI_TO_SERIAL
#define SEND_CSI_TO_SERIAL 1
#else
#define SEND_CSI_TO_SERIAL 0
#endif

#ifdef CONFIG_SEND_CSI_TO_SD
#define SEND_CSI_TO_SD 1
#else
#define SEND_CSI_TO_SD 0
#endif

#define BLINK_GPIO GPIO_NUM_13

#define I2C_SLAVE_SCL_IO CONFIG_I2C_SLAVE_SCL               /*!< gpio number for i2c slave clock */
#define I2C_SLAVE_SDA_IO CONFIG_I2C_SLAVE_SDA               /*!< gpio number for i2c slave data */
#define I2C_SLAVE_NUM I2C_NUMBER(CONFIG_I2C_SLAVE_PORT_NUM) /*!< I2C port number for slave dev */
#define I2C_SLAVE_TX_BUF_LEN (2 * DATA_LENGTH)              /*!< I2C slave tx buffer size */
#define I2C_SLAVE_RX_BUF_LEN (2 * DATA_LENGTH)              /*!< I2C slave rx buffer size */

#define ESP_INTR_FLAG_DEFAULT 0

#ifdef CONFIG_GPIO_INPUT_PIN
#define GPIO_INPUT_PIN CONFIG_GPIO_INPUT_PIN
#else
#define GPIO_INPUT_PIN GPIO_NUM_14
#endif

#define GPIO_INPUT_PIN_SEL  1ULL<<GPIO_INPUT_PIN

static xQueueHandle gpio_evt_queue = NULL;
extern xQueueHandle csi_queue = NULL;
static TaskHandle_t csi_comm_task = NULL;

static const TickType_t wifi_delay = 300;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
    vTaskNotifyGiveFromISR(send_csi_on_i2c_task);

}

static void send_csi_on_i2c_task(void* arg)
{
    double phase_info;
    xQueueReset(csi_queue);
    //vTaskDelay(wifi_delay);
    if (xQueueReceive(csi_queue, &phase_info, wifi_delay)) {
        xSemaphoreTake(print_mux, portMAX_DELAY);  //don't need this semaphore probs
        size_t d_size = i2c_slave_write_buffer(I2C_SLAVE_NUM, phase_info, sizeof(double), 1000 / portTICK_RATE_MS);
        if (d_size == 0) {
            ESP_LOGW(TAG, "i2c slave tx buffer full");
        }
        else {
            ESP_LOGW(TAG, "i2c slave writing to buffer\n");
            printf("====TASK[%d] Slave buffer data ====\n", task_idx);
            disp_buf(phase_info, d_size);
        }
        xSemaphoreGive(print_mux);
        vTaskDelay((DELAY_TIME_BETWEEN_ITEMS_MS * (task_idx + 1)) / portTICK_RATE_MS);
    }
}

static void gpio_task(void* arg)
{
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}

void config_print() {
    printf("\n\n\n\n\n\n\n\n");
    printf("-----------------------\n");
    printf("ESP32 CSI Tool Settings\n");
    printf("-----------------------\n");
    printf("PROJECT_NAME: %s\n", "PASSIVE");
    printf("CONFIG_ESPTOOLPY_MONITOR_BAUD: %d\n", CONFIG_ESPTOOLPY_MONITOR_BAUD);
    printf("CONFIG_ESP_CONSOLE_UART_BAUDRATE: %d\n", CONFIG_ESP_CONSOLE_UART_BAUDRATE);
    printf("IDF_VER: %s\n", IDF_VER);
    printf("-----------------------\n");
    printf("WIFI_CHANNEL: %d\n", WIFI_CHANNEL);
    printf("SHOULD_COLLECT_CSI: %d\n", SHOULD_COLLECT_CSI);
    printf("SHOULD_COLLECT_ONLY_LLTF: %d\n", SHOULD_COLLECT_ONLY_LLTF);
    printf("SEND_CSI_TO_SERIAL: %d\n", SEND_CSI_TO_SERIAL);
    printf("SEND_CSI_TO_SD: %d\n", SEND_CSI_TO_SD);
    printf("-----------------------\n");
    printf("\n\n\n\n\n\n\n\n");
}

void passive_init() {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    ESP_ERROR_CHECK(esp_wifi_start());

    const wifi_promiscuous_filter_t filt = {
            .filter_mask = WIFI_PROMIS_FILTER_MASK_DATA
            //.filter_mask = WIFI_PROMIS_FILTER_MASK_ALL
    };

    int curChannel = WIFI_CHANNEL;

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_filter(&filt);
    esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);
}

static esp_err_t i2c_slave_init(void)
{
    int i2c_slave_port = I2C_SLAVE_NUM;
    i2c_config_t conf_slave;
    conf_slave.sda_io_num = I2C_SLAVE_SDA_IO;
    conf_slave.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf_slave.scl_io_num = I2C_SLAVE_SCL_IO;
    conf_slave.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf_slave.mode = I2C_MODE_SLAVE;
    conf_slave.slave.addr_10bit_en = 0;
    conf_slave.slave.slave_addr = ESP_SLAVE_ADDR;
    i2c_param_config(i2c_slave_port, &conf_slave);
    return i2c_driver_install(i2c_slave_port, conf_slave.mode, I2C_SLAVE_RX_BUF_LEN, I2C_SLAVE_TX_BUF_LEN, 0);
}

static void i2c_slave_test(void* arg)
{
    int i = 0;
    int ret;
    uint32_t task_idx = (uint32_t)arg;
    uint8_t* data = (uint8_t*)malloc(DATA_LENGTH);
    uint8_t* data_wr = (uint8_t*)malloc(DATA_LENGTH);
    uint8_t* data_rd = (uint8_t*)malloc(DATA_LENGTH);
    uint8_t sensor_data_h, sensor_data_l;
    int cnt = 0;
    while (1) {
        ESP_LOGI(TAG, "TASK[%d] test cnt: %d", task_idx, cnt++);

        for (i = 0; i < DATA_LENGTH; i++) {
            data[i] = i;
        }
        xSemaphoreTake(print_mux, portMAX_DELAY);
        size_t d_size = i2c_slave_write_buffer(I2C_SLAVE_NUM, data, RW_TEST_LENGTH, 1000 / portTICK_RATE_MS);
        if (d_size == 0) {
            ESP_LOGW(TAG, "i2c slave tx buffer full");
            //ret = i2c_master_read_slave(I2C_MASTER_NUM, data_rd, DATA_LENGTH);
        }
        else {
            ESP_LOGW(TAG, "i2c slave writing to buffer\n");
            printf("====TASK[%d] Slave buffer data ====\n", task_idx);
            disp_buf(data, d_size);

            //ret = i2c_master_read_slave(I2C_MASTER_NUM, data_rd, RW_TEST_LENGTH);
        }
        xSemaphoreGive(print_mux);
        vTaskDelay((DELAY_TIME_BETWEEN_ITEMS_MS * (task_idx + 1)) / portTICK_RATE_MS);
        //---------------------------------------------------
        int size;
        for (i = 0; i < DATA_LENGTH; i++) {
            data_wr[i] = i + 10;
        }
    }
    vSemaphoreDelete(print_mux);
    vTaskDelete(NULL);
}

void app_main(void)
{
    print_mux = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(i2c_slave_init());
    //ESP_ERROR_CHECK(i2c_master_init());
    xTaskCreate(i2c_slave_test, "i2c_slave_test_0", 1024 * 2, (void*)0, 10, NULL);
    xTaskCreate(send_csi_on_i2c_task, "csi_comm_i2c", 1024 * 2, (void*)0, 10, NULL);
}

extern "C" void app_main(void) {
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BLINK_GPIO, 1);

    gpio_config_t io_conf;

    //interrupt of rising edge
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode    
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    csi_queue = xQueueCreate(10, sizeof(double));

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_PIN, gpio_isr_handler, (void*)GPIO_INPUT_PIN);

    //start gpio task
    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);


    //config_print();
    nvs_init();
    //sd_init();
    passive_init();
    ESP_ERROR_CHECK(i2c_slave_init());
    csi_init((char*)"PASSIVE");
    input_loop();
}
