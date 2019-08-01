/*
 * app.c
 *
 *  Created on: 30 Jul 2019
 *      Author: pja
 */
#include "mgos.h"
#include "mgos_sys_config.h"
#include "lvgl/lvgl.h"
#include <stdio.h>

#define MARGIN 5                                //The gap between widgets in pixels

static lv_obj_t     *win;
static lv_obj_t     *reset_button;

/**
 * @brief Reset to the factory default config as defined in mos.yml
 */
static void reset_to_factory_defaults(void) {
    //Revert to the conf0.json file
    if (remove(CONF_USER_FILE) == 0) {
      LOG(LL_WARN, ("Removed %s", CONF_USER_FILE));
    }
    mgos_system_restart();
}

/**
 * @brief handler events generated from all elements of the GUI.
 * @param obj A pointer to the GUI object that sent the event.
 * @param event The event identifier.
 */
static void event_handler(lv_obj_t * obj, lv_event_t event)
{

    if( obj == reset_button && event == LV_EVENT_CLICKED ) {
        reset_to_factory_defaults();
    }

}

/**
 * @brief Start the process of setting up the WiFi connection.
 */
void gui(void) {
    lv_obj_t            *reset_label;
    /*Create a window to hold all the objects*/
    static lv_style_t win_style;
    lv_style_copy(&win_style, &lv_style_transp);
    win_style.body.padding.left= LV_DPI / MARGIN;
    win_style.body.padding.right = LV_DPI / MARGIN;
    win_style.body.padding.top = LV_DPI / MARGIN;
    win_style.body.padding.bottom = LV_DPI / MARGIN;
    win_style.body.padding.inner = LV_DPI / MARGIN;

    win = lv_win_create(lv_disp_get_scr_act(NULL), NULL);
    lv_win_set_title(win, "IoT Application");

    reset_button = lv_btn_create(lv_scr_act(), NULL);
    lv_obj_set_event_cb(reset_button, event_handler);
    lv_obj_align(reset_button, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
    reset_label = lv_label_create(reset_button, NULL);
    lv_label_set_text(reset_label, " Factory\nDefaults");
}
