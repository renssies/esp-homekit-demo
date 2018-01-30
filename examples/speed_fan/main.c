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
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>

// The GPIO pin that is connected to the LED on the Sonoff Basic.
const int led_gpio = 4;

void led_write(bool on) {
    gpio_write(led_gpio, on ? 0 : 1);
}

void fan_active_callback(homekit_characteristic_t *_ch, homekit_value_t active, void *context);
void fan_rotation_speed_callback(homekit_characteristic_t *_ch, homekit_value_t speed, void *context);

homekit_characteristic_t fan_active = HOMEKIT_CHARACTERISTIC_(
    ACTIVE, 1, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(fan_active_callback)
);

homekit_characteristic_t fan_rotation_speed = HOMEKIT_CHARACTERISTIC_(
                                                              ROTATION_SPEED,
                                                                      1,
                                                                      .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(fan_rotation_speed_callback),
                                                                      .format = homekit_format_uint8,
                                                                      .unit = homekit_unit_none,
                                                                      .min_value = (float[]) {0},
                                                                      .max_value = (float[]) {3},
                                                                      .value = HOMEKIT_UINT8_(1),
                                                                      .min_step = (float[]) {1},
                                                              );

void gpio_init() {
    gpio_enable(led_gpio, GPIO_OUTPUT);
    led_write(false);
}

void fan_active_callback(homekit_characteristic_t *_ch, homekit_value_t active, void *context) {
    printf("Fan active %d\n",active.bool_value);
    //relay_write(switch_on.value.bool_value);
}

void fan_rotation_speed_callback(homekit_characteristic_t *_ch, homekit_value_t speed, void *context) {
    printf("Fan speed %d\n",speed.int_value);
    //relay_write(switch_on.value.bool_value);
}

void fan_identify_task(void *_args) {
    // We identify the Sonoff by Flashing it's LED.
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

void switch_identify(homekit_value_t _value) {
    printf("Fan identify\n");
    xTaskCreate(fan_identify_task, "Fan identify", 128, NULL, 2, NULL);
}

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, "3 Speed Fan");

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_fan, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            &name,
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "renssies"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "037A2BABF19D"),
            HOMEKIT_CHARACTERISTIC(MODEL, "3 Speed Fan"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, switch_identify),
            NULL
        }),
        HOMEKIT_SERVICE(FAN2, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "3 Speed Fan"),
            &fan_active,
            &fan_rotation_speed,
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

void create_accessory_name() {
    uint8_t macaddr[6];
    sdk_wifi_get_macaddr(STATION_IF, macaddr);
    
    int name_len = snprintf(NULL, 0, "3 Speed Fan-%02X%02X%02X",
                            macaddr[3], macaddr[4], macaddr[5]);
    char *name_value = malloc(name_len+1);
    snprintf(name_value, name_len+1, "3 Speed Fan-%02X%02X%02X",
             macaddr[3], macaddr[4], macaddr[5]);
    
    name.value = HOMEKIT_STRING(name_value);
}

void user_init(void) {
    uart_set_baud(0, 115200);

    create_accessory_name();
    
    wifi_config_init("3-speed-fan", NULL, on_wifi_ready);
    gpio_init();
}
