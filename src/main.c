#include "mgos.h"
#include "mgos_spi.h"

#include "mgos_ili9341.h"
#include "mgos_ili9341.h"
#include "mgos_ads7843.h"
#include "lv_conf.h"
#include "lvgl/lvgl.h"
#include "lv_hal_disp.h"
#include "lv_hal_disp.h"
#include "lv_color.h"
#include "setup_wifi/scan_wifi_networks.h"
#include "setup_wifi/setup_wifi_gui.h"
#include "app/app.h"

static lv_coord_t last_x=0;                 //The last x pos pressed on the touch screen
static lv_coord_t last_y=0;                 //The last y pos pressed on the touch screen
static bool touch_screen_pressed = false;   //The last touched status of the touch screen

#define SYS_STATS_TIMER_MS 1000             //Period of the stats timer in MS
#define LVGL_TIMER__MS 5                    //Period of the timer tasks required to run LVGL

static void start_gui(void);

/**
 * @brief A timer callback (called every SYS_STATS_TIMER_MS) to display
 *        memory usage information via the debug port.
 * @param arg Arguments passed to the callback (Null in this case).
 */
static void sys_stats_timer_cb(void *arg) {
  static bool s_tick_tock = false;
  LOG(LL_INFO,
      ("%s uptime: %.2lf, RAM: %lu, %lu free", (s_tick_tock ? "Tick" : "Tock"),
       mgos_uptime(), (unsigned long) mgos_get_heap_size(),
       (unsigned long) mgos_get_free_heap_size()));
  s_tick_tock = !s_tick_tock;
  (void) arg;
}

/**
 * @brief LVGL timer task (callback) that informs lvgl of time passing.
 * @param arg Arguments passed to the callback (Null in this case).
 */
static void lvgl_inc_timer_cb(void *arg) {
    lv_tick_inc(LVGL_TIMER__MS);
    (void) arg;
}

/**
 * @brief LVGL timer task (callback) that is called periodically to informs lvgl of time passing.
 * @param arg Arguments passed to the callback (Null in this case).
 */
static void lvgl_task_handler_cb(void *arg) {
    lv_task_handler();
    (void) arg;
}

/**
 * @brief The touch screen handler function. Called when the user presses/releases the touch screen.
 * @param event_data A pointer to a mgos_ads7843_event_data that holds the x,y and touch down/up details
 *        of the touch screen.
 */
static void touch_screen_handler(struct mgos_ads7843_event_data *event_data) {

  if (!event_data) {
    return;
  }

  /*
  LOG(LL_INFO, ("orientation=%s", event_data->orientation ? "PORTRAIT" : "LANDSCAPE"));
  LOG(LL_INFO, ("Touch %s, down for %.1f seconds", event_data->direction==TOUCH_UP?"UP":"DOWN", event_data->down_seconds));
  LOG(LL_INFO, ("pixels x/y = %d/%d, adc x/y = %d/%d",  event_data->x, event_data->y, event_data->x_adc, event_data->y_adc));
  */

  if( event_data->direction ) {
      touch_screen_pressed = false;
  }
  else {
      touch_screen_pressed = true;
  }
  last_x = event_data->x;
  last_y = event_data->y;

}

/**
 * @brief The lvgl HAL (hardware adaption layer) function responsible for writing
 *        data to the display.
 * @param disp A pointer to an lv_disp_buf_t
 * @param area A pointer to an lv_area_to be drawn
 * @param color_p A pointer to an array of lv_color_t holding the screen colours for each pixel.
 */
void lvgl_hal_display_write(lv_disp_t *disp, const lv_area_t *area, lv_color_t *color_p){
  int winsize = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);
  ili9341_send_pixels(area->x1, area->y1, area->x2, area->y2, (uint8_t *)color_p, winsize*2);
  lv_disp_flush_ready((void *)disp); // Indicate you are ready with the flushing
}

/**
 * @brief The lvgl HAL to interface with the touch screen.
 * @param indev A pointer to a lv_indev_t. The touchscreen input device.
 * @param data A pointer to a lv_indev_data_t the holds the x,y and touchscreen down/up data.
 */
bool lvgl_hal_touchscreen_read(lv_indev_t*indev, lv_indev_data_t*data) {

    if( touch_screen_pressed ) {
        data->state=LV_INDEV_STATE_PR;
     }
    else {
        data->state=LV_INDEV_STATE_REL;
    }

    data->point.x=last_x;
    data->point.y=last_y;
    //LOG(LL_INFO, ("%s: data->state=%d, last_x=%d, last_y=%d", __FUNCTION__, data->state, last_x, last_y));

    return false; //return `false` because we are not buffering and no more data to,!read
}

/**
 * @brief Program entry point
 **/
enum mgos_app_init_result mgos_app_init(void) {

    mgos_sys_config_set_ili9341_width(LV_HOR_RES_MAX);
    mgos_sys_config_set_ili9341_height(LV_VER_RES_MAX);
    //mgos_ili9341_spi_init();

    mgos_ads7843_set_handler(touch_screen_handler);

    lv_mem_init();

    lv_init();                                                  //Init lvgl lib

    static lv_disp_buf_t disp_buf;
    static lv_color_t buf[LV_HOR_RES_MAX*10];                   //Declare a buffer␣,!for 10 lines
    lv_disp_buf_init(&disp_buf, buf,NULL, LV_HOR_RES_MAX*10);   //Initialize the␣,!display buffer

    lv_disp_drv_t disp_drv;                                     //Descriptor of a display driver
    lv_disp_drv_init(&disp_drv);                                //Basic initialization
    disp_drv.flush_cb=(void *)lvgl_hal_display_write;           //Set your driver function
    disp_drv.buffer=&disp_buf;                                  //Assign the buffer to the display

    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);          //Finally register the driver
    if( disp != NULL ) {
        lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);                          //Descriptor of a input device driver
        indev_drv.type=LV_INDEV_TYPE_POINTER;                   //Touch pad is a pointer-like device
        indev_drv.read_cb = (void *)lvgl_hal_touchscreen_read;
        lv_indev_drv_register(&indev_drv);                      //Finally register the driver

        mgos_set_timer(LVGL_TIMER__MS, MGOS_TIMER_REPEAT, lvgl_inc_timer_cb, NULL);

        mgos_set_timer(LVGL_TIMER__MS, MGOS_TIMER_REPEAT, lvgl_task_handler_cb, NULL);

        mgos_set_timer(SYS_STATS_TIMER_MS, MGOS_TIMER_REPEAT, sys_stats_timer_cb, NULL);

        start_gui();

    }

    //Set the display LED pin high to turn on display
    mgos_gpio_set_mode(mgos_sys_config_get_ili9341_led_pin(), MGOS_GPIO_MODE_OUTPUT);
    mgos_gpio_write(mgos_sys_config_get_ili9341_led_pin(), true);

    return MGOS_APP_INIT_SUCCESS;
}

/**
 * @brief Start the GUI. If an WiFi SSID (term) is not configured then
 *        start a GUI to configured the WiFi. Otherwise run the application GUI.
 */
static void start_gui(void) {
    char *ssid = (char *)mgos_sys_config_get_wifi_sta_ssid();

    if( ssid && strlen(ssid) > 0 ) {
        gui();
    }
    else {
        setup_wifi_start();
    }
}
