#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_continuous.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "stepper_driver.h"

#define BUTTON_GPIO_0       GPIO_NUM_39
#define BUTTON_GPIO_1       GPIO_NUM_41
#define SWITCH_GPIO_0       GPIO_NUM_48
#define SWITCH_GPIO_1       GPIO_NUM_45       

#define DEBOUNCE_TIME_US     50000 // 50 milliseconds in microseconds
static const char *TAG = "BUTTON_DEMO";

static QueueHandle_t gpio_evt_queue = NULL;

#define EXAMPLE_ADC_UNIT               ADC_UNIT_1
#define EXAMPLE_ADC_CONV_MODE          ADC_CONV_SINGLE_UNIT_1
#define EXAMPLE_ADC_BIT_WIDTH               SOC_ADC_DIGI_MAX_BITWIDTH
#define EXAMPLE_ADC_ATTEN                   ADC_ATTEN_DB_12
#define EXAMPLE_READ_LEN                    256

static adc_channel_t channel[1] = {ADC_CHANNEL_7};
static TaskHandle_t s_task_handle;

stepper_driver_cfg_t stepper_config = {
    .en_pin = CONFIG_ENABLE_PIN,
    .step_pin = CONFIG_STEP_PIN,
    .dir_pin = CONFIG_DIRECTION_PIN,
    .en_value = 0,
    .cw_value = CONFIG_CLOCKWISE_LEVEL,
    .steps_per_rev = CONFIG_STEPS_PER_REV,
    .microstep_count = CONFIG_MICROSTEPS_PER_STEP,
    .max_steps_per_sec = 1000000
};

stepper_motor_t stepper_motor = {
    .stepper_cfg = &stepper_config,
    .steps_per_second = 64,
    .timer_resolution_hz = 100000,
    .position = 0
};

// ISR Handler in IRAM for fast execution
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    static uint64_t last_time = 0;
    uint64_t current_time = esp_timer_get_time(); // Time since boot in us
    uint32_t gpio_num = (uint32_t) arg;

    // Software Debounce: Check if enough time has passed since the last edge
    if ((current_time - last_time) > DEBOUNCE_TIME_US) {
        last_time = current_time;
        xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL); // Notify main loop
    }
}

static void button_task(void* arg)
{
    uint32_t io_num;
    for (;;) {
        // Wait blockingly until a valid debounced press event arrives
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            // Read current value to verify state if needed
            int level = gpio_get_level(io_num);
            stepper_motor_t *motor = (stepper_motor_t*) arg;
            if(io_num == GPIO_NUM_45)
            {
                if(!level)
                {
                    StepperDriver_enable(motor);
                }
                else {
                    StepperDriver_disable(motor);
                }
            }

            if(io_num == GPIO_NUM_39)
            {
                if(!level){
                    StepperDriver_start_motion(motor, 0);
                }
                else {
                    StepperDriver_stop_motion(motor);
                }
            }

            if(io_num == GPIO_NUM_41)
            {
                if(!level) {
                    StepperDriver_start_motion(motor, 1);
                }
                else {
                    StepperDriver_stop_motion(motor);
                }
            }
        }
    }
}

static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    //Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR(s_task_handle, &mustYield);

    return (mustYield == pdTRUE);
}

static void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle)
{
    adc_continuous_handle_t handle = NULL;

    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = EXAMPLE_READ_LEN,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = 20 * 1000,
        .conv_mode = EXAMPLE_ADC_CONV_MODE,
    };

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    dig_cfg.pattern_num = channel_num;
    for (int i = 0; i < channel_num; i++) {
        adc_pattern[i].atten = EXAMPLE_ADC_ATTEN;
        adc_pattern[i].channel = channel[i] & 0x7;
        adc_pattern[i].unit = EXAMPLE_ADC_UNIT;
        adc_pattern[i].bit_width = EXAMPLE_ADC_BIT_WIDTH;

        ESP_LOGI(TAG, "adc_pattern[%d].atten is :%"PRIx8, i, adc_pattern[i].atten);
        ESP_LOGI(TAG, "adc_pattern[%d].channel is :%"PRIx8, i, adc_pattern[i].channel);
        ESP_LOGI(TAG, "adc_pattern[%d].unit is :%"PRIx8, i, adc_pattern[i].unit);
    }
    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

    *out_handle = handle;
}

void app_main(void)
{
    esp_err_t ret;
    uint32_t ret_num = 0;
    uint8_t result[EXAMPLE_READ_LEN] = {0};
    memset(result, 0xcc, EXAMPLE_READ_LEN);
    // 1. Configure the GPIO pin
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,       // Trigger on falling edge (Pressing down)
        .mode = GPIO_MODE_INPUT,               // Set as Input Mode
        .pin_bit_mask = (1ULL << BUTTON_GPIO_0) | (1ULL << BUTTON_GPIO_1) | (1ULL << SWITCH_GPIO_0) | (1ULL << SWITCH_GPIO_1), // Bit mask of the pin
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Disable internal pull-down
        .pull_up_en = GPIO_PULLUP_ENABLE       // Enable internal pull-up for standard buttons
    };
    gpio_config(&io_conf);

    // 2. Create a FreeRTOS Queue to communicate between ISR and Task
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    
    // Initialize stepper motor
    ESP_ERROR_CHECK(StepperDriver_new_stepper_motor(&stepper_motor, &stepper_config));
    ESP_LOGI(TAG, "Stepper motor initialized");
    StepperDriver_set_speed(&stepper_motor, 320);
    int en_lvl = gpio_get_level(GPIO_NUM_45);
    if(!en_lvl) {
        StepperDriver_enable(&stepper_motor);
    }
    else {
        StepperDriver_disable(&stepper_motor);
    }
    
    // 3. Start the handling thread
    xTaskCreate(button_task, "button_task", 2048, &stepper_motor, 10, NULL);

    // 4. Install GPIO ISR Service and attach the handler
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO_0, gpio_isr_handler, (void*) BUTTON_GPIO_0);
    gpio_isr_handler_add(BUTTON_GPIO_1, gpio_isr_handler, (void*) BUTTON_GPIO_1);
    gpio_isr_handler_add(SWITCH_GPIO_0, gpio_isr_handler, (void*) SWITCH_GPIO_0);
    gpio_isr_handler_add(SWITCH_GPIO_1, gpio_isr_handler, (void*) SWITCH_GPIO_1);

    ESP_LOGI(TAG, "Button debounce initialization complete.");

    s_task_handle = xTaskGetCurrentTaskHandle();

    adc_continuous_handle_t handle = NULL;
    continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &handle);

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));
    ESP_ERROR_CHECK(adc_continuous_start(handle));

    int last_value = 0;
    
    vTaskDelay(1000/portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Moving Clockwise 3200 steps");
    vTaskDelay(10/portTICK_PERIOD_MS);
    
    while(1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        while(1) {
            ret = adc_continuous_read(handle, result, EXAMPLE_READ_LEN, &ret_num, 0);
            if (ret == ESP_OK) {
                // ESP_LOGI("TASK", "ret is %x, ret_num is %"PRIu32" bytes", ret, ret_num);

                adc_continuous_data_t parsed_data[ret_num / SOC_ADC_DIGI_RESULT_BYTES];
                uint32_t num_parsed_samples = 0;

                esp_err_t parse_ret = adc_continuous_parse_data(handle, result, ret_num, parsed_data, &num_parsed_samples);
                if (parse_ret == ESP_OK) {
                    for (int i = 0; i < num_parsed_samples; i++) {
                        if (parsed_data[i].valid && parsed_data[i].raw_data != last_value) {
                            int diff = parsed_data[i].raw_data - last_value;
                            if(abs(diff) > 10 ){
                                last_value = parsed_data[i].raw_data;
                                double duty = (double)last_value / 4095;
                                if(last_value < 15)
                                {
                                    duty = 0;
                                }
                                else if(last_value > 4085)
                                {
                                    duty = 1;
                                }

                                ESP_LOGI(TAG, "ADC%d, Channel: %d, Value: %u, Duty: %f",
                                         parsed_data[i].unit + 1,
                                         parsed_data[i].channel,
                                         parsed_data[i].raw_data,
                                         duty);
                            }
                        }
                    }
                } else {
                    ESP_LOGE(TAG, "Data parsing failed: %s", esp_err_to_name(parse_ret));
                }
                vTaskDelay(10);
            } else if (ret == ESP_ERR_TIMEOUT) {
                break;
            }
        }
    }
}
