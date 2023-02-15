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

#ifdef CONFIG_MAX_CHANNEL
#define MAX_CHANNEL CONFIG_MAX_CHANNEL
#else
#define MAX_CHANNEL 11
#endif

#ifdef CONFIG_MIN_CHANNEL
#define MIN_CHANNEL CONFIG_MIN_CHANNEL
#else
#define MIN_CHANNEL 1
#endif

#define BLINK_GPIO GPIO_NUM_13

#define DATA_LENGTH 512 

#define ESP_SLAVE_ADDR CONFIG_I2C_SLAVE_ADDRESS /*!< ESP32 slave address, you can set any 7bit value */
#define I2C_SLAVE_SCL_IO CONFIG_I2C_SLAVE_SCL               /*!< gpio number for i2c slave clock */
#define I2C_SLAVE_SDA_IO CONFIG_I2C_SLAVE_SDA               /*!< gpio number for i2c slave data */
#define I2C_SLAVE_NUM /*I2C_NUMBER(*/CONFIG_I2C_SLAVE_PORT_NUM /*!< I2C port number for slave dev */
#define I2C_SLAVE_TX_BUF_LEN (2 * DATA_LENGTH)              /*!< I2C slave tx buffer size */
#define I2C_SLAVE_RX_BUF_LEN (2 * DATA_LENGTH)              /*!< I2C slave rx buffer size */

#define ESP_INTR_FLAG_DEFAULT 0

#ifdef CONFIG_GPIO_INPUT_PIN
#define GPIO_INPUT_PIN CONFIG_GPIO_INPUT_PIN
#else
#define GPIO_INPUT_PIN GPIO_NUM_14
#endif

#define GPIO_INPUT_PIN_SEL  1ULL<<GPIO_INPUT_PIN

#define DELAY_TIME_BETWEEN_ITEMS 10

static const char* TAG = "rssi_comms";
SemaphoreHandle_t print_mux = NULL;
SemaphoreHandle_t csi_mutex = xSemaphoreCreateMutex();

static xQueueHandle gpio_evt_queue = NULL;
xQueueHandle rssi_queue = NULL;
static TaskHandle_t rssi_comm_task = NULL;
static TaskHandle_t chan_surf_task = NULL;
//static TaskHandle_t gpio_task = NULL;

int curChannel = 0;
extern const TickType_t wifi_delay = 10;
extern int data_len = 0;

union phase_union {
    float double_var;
    char bytes_array[4];
    uint8_t u_bytes_array[4];
};

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    //uint32_t gpio_num = (uint32_t)arg;
    //xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(rssi_comm_task, &xHigherPriorityTaskWoken);
    //portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR();
}

static void send_csi_fake_info(void* arg) {
    for (;;) {
        vTaskDelay(5000);
        float fake_phase = -3.1415;
        phase_union phase_bytes;
        phase_bytes.double_var = fake_phase;
        size_t d_size;
        for (size_t i = 0; i < 65; i++) {
            printf("phase info bytes: ");
            for (size_t j = 0; j < 4; j++) {
                printf("%02x ", phase_bytes.u_bytes_array[j]);
                //printf(phase_bytes.bytes_array[j]);

                //uint8_t* phase_byte = reinterpret_cast<uint8_t*> (&phase_bytes.bytes_array[j]);
                //char* phase_uint = (char*)phase_byte;
                //printf("(uint byte) %u ", *phase_byte);
                //char* phase_b = reinterpret_cast<char*> (&phase_byte);

                d_size = i2c_slave_write_buffer(I2C_SLAVE_NUM, &phase_bytes.u_bytes_array[j], sizeof(char), 1000 / portTICK_RATE_MS);
                //d_size = i2c_slave_write_buffer(I2C_SLAVE_NUM, &phase_bytes.bytes_array[j], sizeof(char), 1000 / portTICK_RATE_MS);
            }
            if (d_size == 0) {
                ESP_LOGW(TAG, "i2c slave tx buffer full");
            }
            else {
                ESP_LOGW(TAG, "i2c slave writing to buffer\n");
            }
            printf("\n");
        }
    }
}

static void send_rssi_on_i2c_task(void* arg)
{
    for (;;) {
        //vTaskDelay(2000);
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        //double phase_info;
        double rssi_val;
        double peek_val;
        xSemaphoreTake(csi_mutex, portMAX_DELAY);
        if (rssi_queue != 0) {
            xQueueReset(rssi_queue);
            xSemaphoreGive(csi_mutex);
            printf("QUEUE RESET ------------------------------------\n");
            printf("Data Length: %d\n", data_len);
            
            if (xQueuePeek(rssi_queue, &(peek_val), 2000)) {
                printf("Peeked RSSI: %f\n", peek_val);
                xSemaphoreTake(csi_mutex, portMAX_DELAY);
                phase_union phase_bytes;
                if (xQueueReceive(rssi_queue, &(rssi_val), wifi_delay)) {
                    xSemaphoreTake(print_mux, portMAX_DELAY);
                    printf("RSSI: %f", rssi_val);
                    phase_bytes.double_var = rssi_val;
                    printf("RSSI double: %f\n", phase_bytes.double_var);
                    printf("RSSI bytes: ");
                    for (size_t j = 0; j < 4; j++) {
                        printf("%02x ", phase_bytes.u_bytes_array[j]);
                    }
                    size_t d_size;
                    for (size_t j = 0; j < 4; j++) {
                        d_size = i2c_slave_write_buffer(I2C_SLAVE_NUM, &phase_bytes.u_bytes_array[j], sizeof(char), 1000 / portTICK_RATE_MS);
                    }
                    if (d_size == 0) {
                        ESP_LOGW(TAG, "i2c slave tx buffer full");
                    }
                    else {
                        ESP_LOGW(TAG, "i2c slave writing to buffer\n");
                    }

                }
                /*
                for (size_t i = 0; i < (data_len / 2) + 1; i++) {
                    //printf("Inside the for loop\n");
                    //xSemaphoreTake(csi_mutex, portMAX_DELAY);
                    printf("%d", i);
                    if (xQueueReceive(rssi_queue, &(phase_info), wifi_delay)) {
                        xSemaphoreTake(print_mux, portMAX_DELAY);  //don't need this semaphore probs
                        printf("phase info: %f\n", phase_info);

                        phase_bytes.double_var = phase_info;
                        //const uint8_t* phase = reinterpret_cast<uint8_t*> (&phase_info);
                        //printf("phase info bytes: %d\n", *phase);
                        printf("phase info double: %f\n", phase_bytes.double_var);
                        printf("phase info bytes: ");
                        for (size_t j = 0; j < 4; j++) {
                            printf("%02x ", phase_bytes.u_bytes_array[j]);
                        }
                        printf("\n");
                        size_t d_size;
                        for (size_t j = 0; j < 4; j++) {
                            //const uint8_t* phase_byte = reinterpret_cast<uint8_t*> (&phase_bytes.bytes_array[j]);
                            d_size = i2c_slave_write_buffer(I2C_SLAVE_NUM, &phase_bytes.u_bytes_array[j], sizeof(char), 1000 / portTICK_RATE_MS);
                            //d_size = i2c_slave_write_buffer(I2C_SLAVE_NUM, phase_byte, sizeof(char), 1000 / portTICK_RATE_MS);
                        }
                        //size_t d_size = i2c_slave_write_buffer(I2C_SLAVE_NUM, phase, sizeof(double), 1000 / portTICK_RATE_MS);
                        if (d_size == 0) {
                            ESP_LOGW(TAG, "i2c slave tx buffer full");
                        }
                        else {
                            ESP_LOGW(TAG, "i2c slave writing to buffer\n");
                            //printf("====TASK[%d] Slave buffer data ====\n", 0);
                            //disp_buf(phase_info, d_size);
                        }
                        //xSemaphoreGive(csi_mutex);
                        xSemaphoreGive(print_mux);
                        
                    }
                    else {
                        printf("Queue receive failed\n");
                    }
                    //xSemaphoreGive(csi_mutex);
                }*/
                xSemaphoreGive(csi_mutex);
                xSemaphoreGive(print_mux);
            }
            else {
                printf("Nothing in queue\n");
            }
        }
        else {
            printf("QUEUE NOT CREATED\n");
            xSemaphoreGive(csi_mutex);
        }
    }
}

static void channel_surf_task(void* arg)
{
    while (1) {
        xSemaphoreTake(csi_mutex, portMAX_DELAY);
        xSemaphoreTake(print_mux, portMAX_DELAY);
        curChannel = (curChannel + 1) % (MAX_CHANNEL - MIN_CHANNEL + 2);
        if (curChannel == 0) {
            curChannel = curChannel + 1;
        }
        /*if (curChannel == 1) {
            curChannel = 6;
        }
        else if (curChannel == 6) {
            curChannel = 11;
        }
        else if (curChannel == 11) {
            curChannel = 1;
        }*/
        //curChannel = 1;
        esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);
        printf("CHANNEL: %d\n", curChannel);
        xSemaphoreGive(csi_mutex);
        xSemaphoreGive(print_mux);
        vTaskDelay(5000);
    }
}

/*
static void gpio_task(void* arg)
{
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}
*/

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
    printf("SLAVE_ADDRESS: %0x\n", CONFIG_I2C_SLAVE_ADDRESS);
    printf("SLAVE_SCL: %0d\n", CONFIG_I2C_SLAVE_SCL);
    printf("SLAVE_SDA: %0d\n", CONFIG_I2C_SLAVE_SDA);
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

    curChannel = WIFI_CHANNEL;
    //curChannel = 4;

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
    conf_slave.slave.maximum_speed = I2C_SCLK_MAX; // Needed to add this because of the esp-idf version we are using
    conf_slave.clk_flags = 0; // Needed to add this because of the esp-idf version we are using
    printf("Slave Max Speed: %d", I2C_SCLK_MAX);
    i2c_param_config(i2c_slave_port, &conf_slave);
    return i2c_driver_install(i2c_slave_port, conf_slave.mode, I2C_SLAVE_RX_BUF_LEN, I2C_SLAVE_TX_BUF_LEN, 0);
}

/*
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
        size_t d_size = i2c_slave_write_buffer(I2C_SLAVE_NUM, data, 512, 1000 / portTICK_RATE_MS);
        if (d_size == 0) {
            ESP_LOGW(TAG, "i2c slave tx buffer full");
            //ret = i2c_master_read_slave(I2C_MASTER_NUM, data_rd, DATA_LENGTH);
        }
        else {
            ESP_LOGW(TAG, "i2c slave writing to buffer\n");
            printf("====TASK[%d] Slave buffer data ====\n", task_idx);
            //disp_buf(data, d_size);

            //ret = i2c_master_read_slave(I2C_MASTER_NUM, data_rd, RW_TEST_LENGTH);
        }
        xSemaphoreGive(print_mux);
        //vTaskDelay((DELAY_TIME_BETWEEN_ITEMS_MS * (task_idx + 1)) / portTICK_RATE_MS);
        //---------------------------------------------------
        int size;
        for (i = 0; i < DATA_LENGTH; i++) {
            data_wr[i] = i + 10;
        }
    }
    vSemaphoreDelete(print_mux);
    vTaskDelete(NULL);
}
*/


/*
void app_main(void)
{
    print_mux = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(i2c_slave_init());
    //ESP_ERROR_CHECK(i2c_master_init());
    xTaskCreate(i2c_slave_test, "i2c_slave_test_0", 1024 * 2, (void*)0, 10, NULL);
    xTaskCreate(send_csi_on_i2c_task, "csi_comm_i2c", 1024 * 2, (void*)0, 10, NULL);
}
*/


// Use this for help: https://docs.espressif.com/projects/esp-idf/en/v4.3/esp32/api-reference/peripherals/i2c.html
extern "C" void app_main(void) {
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BLINK_GPIO, 1);

    gpio_config_t io_conf;

    //interrupt of rising edge
    io_conf.intr_type = (gpio_int_type_t) GPIO_PIN_INTR_POSEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = (gpio_pullup_t) 1;
    io_conf.pull_down_en = (gpio_pulldown_t) 0;
    gpio_config(&io_conf);

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    rssi_queue = NULL;

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add((gpio_num_t) GPIO_INPUT_PIN, gpio_isr_handler, (void*)GPIO_INPUT_PIN);

    //start gpio task
    //xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);
    xTaskCreate(send_rssi_on_i2c_task, "rssi_comm_task", 2048, NULL, 10, &rssi_comm_task);

    //xTaskCreate(send_csi_fake_info, "csi_comm_task", 2048, NULL, 10, &csi_comm_task);


    print_mux = xSemaphoreCreateMutex();

    config_print();
    nvs_init();
    //sd_init();
    passive_init();
    ESP_ERROR_CHECK(i2c_slave_init());
    csi_init((char *) "PASSIVE");
    //xTaskCreate(channel_surf_task, "channel_surf_task", 2048, NULL, 10, &chan_surf_task);
    input_loop();
}
