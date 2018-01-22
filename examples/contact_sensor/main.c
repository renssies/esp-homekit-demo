/*
 * Example of using esp-homekit library to control
 * a simple $5 Sonoff Basic using HomeKit.
 * The esp-wifi-config library is also used in this
 * example. This means you don't have to specify
 * your network's SSID and password before building.
 *
 * In order to flash the sonoff basic you will have to
 * have a 3,3v (logic level) FTDI adapter.
 *
 * To flash this example connect 3,3v, TX, RX, GND
 * in this order, beginning in the (square) pin header
 * next to the button.
 * Next hold down the button and connect the FTDI adapter
 * to your computer. The sonoff is now in flash mode and
 * you can flash the custom firmware.
 *
 * WARNING: Do not connect the sonoff to AC while it's
 * connected to the FTDI adapter! This may fry your
 * computer and sonoff.
 *
 */

#include <stdio.h>
#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>

const int led_gpio = 2;

void led_write(bool on) {
    gpio_write(led_gpio, on ? 0 : 1);
}

homekit_characteristic_t contact_sensor_state = HOMEKIT_CHARACTERISTIC_(
    CONTACT_SENSOR_STATE, 0
);

void gpio_init() {
    gpio_enable(led_gpio, GPIO_OUTPUT);
    led_write(false);
}

void sensor_read_task(void *_args) {

    while (1) {
        // Read the ADC
        uint16_t adc_read = sdk_system_adc_read();
        uint8_t contact_detected = adc_read > 512 ? 0 : 1;
        if (contact_detected != contact_sensor_state.value.int_value) {
            contact_sensor_state.value.int_value = contact_detected;
            homekit_characteristic_notify(&contact_sensor_state, HOMEKIT_UINT8(contact_detected));
        }
        
        // Reading the ADC too much may cause all kinds of issues. We also don't want to
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

}

void contact_sensor_identify_task(void *_args) {
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            led_write(true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            led_write(false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    led_write(false);

    vTaskDelete(NULL);
}

void contact_sensor_identify(homekit_value_t _value) {
    printf("Contact Sensor identify\n");
    xTaskCreate(contact_sensor_identify_task, "Contact Sensor identify", 128, NULL, 2, NULL);
}

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_sensor, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Contact Sensor"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "renssies"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "0N7F5BAGF16D"),
            HOMEKIT_CHARACTERISTIC(MODEL, "CS1"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, contact_sensor_identify),
            NULL
        }),
        HOMEKIT_SERVICE(CONTACT_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Contact Sensor"),
            &contact_sensor_state,
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"
};

void on_wifi_ready() {
    homekit_server_init(&config);
}

void user_init(void) {
    uart_set_baud(0, 115200);

    wifi_config_init("contact-sensor", NULL, on_wifi_ready);
    xTaskCreate(sensor_read_task, "Contact Sensor read", 256, NULL, 2, NULL);
    gpio_init();
}
