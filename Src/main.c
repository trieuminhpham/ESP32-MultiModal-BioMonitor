#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

// Thư viện hệ thống
#include "driver/ledc.h"    // PWM
#include "driver/adc.h"     // ADC cho AD8232
#include "i2cdev.h"         // I2C chung

// Thư viện cảm biến
#include "sensor_drivers/adns3080.h"       // SPI
#include "ads111x.h"        // I2C
#include "max30102.h"       // I2C

static const char *TAG = "BIO_MONITOR";

// --- CẤU HÌNH PIN ---
#define I2C_SDA_GPIO    21  //
#define I2C_SCL_GPIO    22  //
#define AD8232_ADC_CH   ADC_CHANNEL_6 // GPIO34
#define ADNS_LED_PIN    4   // PWM cho ADNS

// Cấu hình SPI cho ADNS3080
#define ADNS_MOSI 23
#define ADNS_MISO 19
#define ADNS_SCLK 18
#define ADNS_CS   17
#define ADNS_RST  16

// --- CẤU TRÚC DỮ LIỆU CHUNG ---
typedef struct {
    float adns_brightness;
    int16_t ads_val;
    int ecg_val;
    uint32_t red;
    uint32_t ir;
} sensor_results_t;

sensor_results_t data = {0};

// ========================== 1. TASK ADNS3080 & PWM ==========================
void adns_task(void *pv) {
    // Khởi tạo PWM cho LED hỗ trợ sensor
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 40000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);
    ledc_channel_config_t chan = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .gpio_num = ADNS_LED_PIN,
        .duty = 512, // 50%
        .hpoint = 0
    };
    ledc_channel_config(&chan);

    adns3080_t sensor;
    if (adns3080_init(&sensor, ADNS_MOSI, ADNS_MISO, ADNS_SCLK, ADNS_CS, ADNS_RST) == ESP_OK) {
        ESP_LOGI(TAG, "ADNS3080 Khoi tao thanh cong!");
    }

    while (1) {
        data.adns_brightness = adns3080_frame_capture(&sensor); //
        vTaskDelay(pdMS_TO_TICKS(100)); // Đọc chậm để tiết kiệm tài nguyên
    }
}

// ========================== 2. TASK ADS1115 (I2C) ==========================
void ads_task(void *pv) {
    i2c_dev_t dev = {0};
    ads111x_init_desc(&dev, ADS111X_ADDR_GND, I2C_NUM_0, I2C_SDA_GPIO, I2C_SCL_GPIO); //
    ads111x_set_mode(&dev, ADS111X_MODE_CONTINUOUS);
    ads111x_set_gain(&dev, ADS111X_GAIN_4V096);
    ads111x_set_input_mux(&dev, ADS111X_MUX_0_1);
    ads111x_set_data_rate(&dev, ADS111X_DATA_RATE_128);

    while (1) {
        int16_t raw;
        if (ads111x_get_value(&dev, &raw) == ESP_OK) { //
            data.ads_val = raw;
        }
        vTaskDelay(pdMS_TO_TICKS(50)); // Nhường bus I2C cho MAX30102
    }
}

// ========================== 3. TASK AD8232 (ANALOG) ==========================
void ecg_task(void *pv) {
    adc1_config_width(ADC_WIDTH_BIT_12); //
    adc1_config_channel_atten(AD8232_ADC_CH, ADC_ATTEN_DB_11);

    TickType_t last_wake = xTaskGetTickCount();
    while (1) {
        data.ecg_val = adc1_get_raw(AD8232_ADC_CH); //
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10)); // 100Hz
    }
}

// ========================== 4. TASK MAX30102 (I2C) ==========================
void max_task(void *pv) {
    i2c_dev_t dev;
    struct max30102_record record;
    memset(&dev, 0, sizeof(i2c_dev_t));
    
    max30102_initDesc(&dev, 0, I2C_SDA_GPIO, I2C_SCL_GPIO); //
    // Giảm Sample Rate xuống một chút để ổn định toàn hệ thống
    max30102_init(0x1F, 4, 2, 100, 411, 16384, &record, &dev); //
    max30102_clearFIFO(&dev);

    while (1) {
        max30102_check(&record, &dev); //
        while (max30102_available(&record)) {
            data.red = max30102_getFIFORed(&record); //
            data.ir = max30102_getFIFOIR(&record);   //
            max30102_nextSample(&record);
        }
        vTaskDelay(pdMS_TO_TICKS(20)); // Rất quan trọng để không treo I2C
    }
}

// ========================== TASK REPORT (IN DỮ LIỆU) ==========================
void report_task(void *pv) {
    vTaskDelay(pdMS_TO_TICKS(2000)); // Đợi các sensor khởi động xong
    printf("ADNS,ADS1115,AD8232,RED,IR\n");
    
    while (1) {
        // In một dòng duy nhất để Serial Plotter dễ vẽ
        printf("%.1f,%d,%d,%lu,%lu\n", 
               data.adns_brightness, data.ads_val, data.ecg_val, data.red, data.ir);
        vTaskDelay(pdMS_TO_TICKS(20)); 
    }
}

void app_main(void) {
    // 1. Khởi tạo I2C dùng chung cho ADS và MAX
    ESP_ERROR_CHECK(i2cdev_init());

    // 2. Tạo các Task riêng biệt
    // Task ưu tiên cao hơn cho các sensor đo nhịp tim (Core 0)
    xTaskCreatePinnedToCore(ecg_task, "ECG", 3072, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(max_task, "MAX", 4096, NULL, 5, NULL, 0);

    // Task phụ trợ (Core 1)
    xTaskCreatePinnedToCore(adns_task, "ADNS", 3072, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(ads_task,  "ADS",  3072, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(report_task, "Print", 3072, NULL, 2, NULL, 1);
}