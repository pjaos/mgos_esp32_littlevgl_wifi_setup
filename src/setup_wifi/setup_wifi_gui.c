/*
 * setup_wifi_gui.c
 *
 *  Created on: 27 Jul 2019
 *      Author: pja
 */
#include "mgos.h"
#include <stdio.h>

#include "lvgl/lvgl.h"
#include "lv_kb.h"

#include "kb_handler.h"
#include "scan_wifi_networks.h"

#define MAX_SSID_LENGTH     32                  //The max number of characters in the WiFi SSID
#define MAX_PASSWORD_LENGTH 64                  //The max number of characters in the WiFi password
#define MARGIN              5                   //The gap between widegts in pixels
#define BUTTON_HEIGHT       LV_HOR_RES_MAX/10   //The button height for the interface in pixels

#define SHOW_PASSWORD_LABEL_TEXT "    Show\nPassword"
#define HIDE_PASSWORD_LABEL_TEXT "    Hide\nPassword"

static lv_obj_t     *next_label;
static lv_obj_t     *back_label;
static lv_obj_t     *keyboard;
static lv_obj_t     *ssid_text_area;
static lv_obj_t     *password_text_area;
static lv_obj_t     *win;
static lv_group_t   *group;
static lv_obj_t     *scan_button;
static lv_obj_t     *back_button;
static lv_obj_t     *next_button;
static lv_obj_t     *ddlist;
static lv_obj_t     *indicator_bar;
static lv_obj_t     *active_text_area;
static lv_obj_t     *ssid_label;
static lv_obj_t     *password_label;
static int          wizard_step;
static int          win_width;
static int          label_width;
static char         wifi_ssid[MAX_SSID_LENGTH];
static char         wifi_password[MAX_PASSWORD_LENGTH];
static int          label_height=0;
static lv_obj_t     *ssid_value_label;
static lv_obj_t     *password_value_label;
static lv_obj_t     *info_label;
static lv_obj_t     *password_visibility_button;
static bool         hide_password=false;
static lv_obj_t     *password_visibility_label;

static void event_handler(lv_obj_t * obj, lv_event_t event);
static void show_keyboad(lv_obj_t *text_area);
static void set_step_0(void);
static void set_step_1();
static void set_step_2(void);
static void setup_wifi(void);

/**
 * @brief Remove a widfget from the screen.
 * @param widget A pointer to the GUI widget to be removed.
 */
static void remove_widget(lv_obj_t *widget) {
    if( widget ) {
        lv_obj_del(widget);
    }
}

/**
 * @brief Scan the Wifi networks and show those found in the drop down list.
 */
void wifi_networks_scan_comnplete(char *ssid_list) {
    //Remove the indicator bar if it exists
    remove_widget(indicator_bar);
    indicator_bar=NULL;

    //Create a pulldown list of detected WiFi SSID's
    ddlist = lv_ddlist_create(lv_scr_act(), NULL);
    lv_ddlist_set_options(ddlist, ssid_list);
    lv_ddlist_set_fix_width(ddlist, win_width-(2*MARGIN) );
    lv_ddlist_set_draw_arrow(ddlist, true);
    lv_obj_align(ddlist, NULL, LV_ALIGN_IN_TOP_MID, 0, 2*BUTTON_HEIGHT+(3*MARGIN) );
    lv_obj_set_event_cb(ddlist, event_handler);

}

/**
 * @brief When called the keyboard is displayed.
 * @param text_area When keys are pressed on the keyboard they are sent to the text_area.
 */
static void show_keyboad(lv_obj_t *text_area)
{
    /*Create styles for the keyboard*/
    static lv_style_t rel_style, pr_style;

    lv_style_copy(&rel_style, &lv_style_btn_rel);
    rel_style.body.radius = 0;
    rel_style.body.border.width = 1;

    lv_style_copy(&pr_style, &lv_style_btn_pr);
    pr_style.body.radius = 0;
    pr_style.body.border.width = 1;

    /*Create a keyboard and apply the styles*/
    keyboard = lv_kb_create(lv_scr_act(), NULL);
    lv_kb_set_cursor_manage(keyboard, true);
    lv_kb_set_style(keyboard, LV_KB_STYLE_BG, &lv_style_transp_tight);
    lv_kb_set_style(keyboard, LV_KB_STYLE_BTN_REL, &rel_style);
    lv_kb_set_style(keyboard, LV_KB_STYLE_BTN_PR, &pr_style);

    /*Assign the text area to the keyboard*/
    lv_kb_set_ta(keyboard, text_area);

    lv_obj_set_event_cb(keyboard, lv_kb_pja_event_cb);
}

/**
 * @brief Set the selected SSID from pulldown SSID list
 */
static void set_selected_ssid(void) {
    if( ddlist ) {
        char buf[MAX_SSID_LENGTH];

        lv_ddlist_get_selected_str(ddlist, buf, sizeof(buf));
        if ( strlen(buf) > 0 && active_text_area != NULL) {
            lv_ta_set_text(active_text_area, buf);
        }

    }
}

/**
 * @brief Create the WiFi scan in progress indicator bar.
 */
static void create_wifi_scan_indicator(void) {
    indicator_bar = lv_bar_create(lv_scr_act(), NULL);
    lv_obj_set_size(indicator_bar, win_width-(2*MARGIN), 30);
    lv_obj_align(indicator_bar, NULL, LV_ALIGN_IN_TOP_MID, 0, 2*BUTTON_HEIGHT+(3*MARGIN));
    lv_bar_set_anim_time(indicator_bar, MAX_WIFI_SCAN_MS);
    lv_bar_set_value(indicator_bar, 100, LV_ANIM_ON);
    lv_btn_set_toggle(scan_button, false);
}

/**
 * @brief Remove (if present) the SSID list pulldown.
 */
static void remove_ssid_list(void) {
    remove_widget(ddlist);
    ddlist=NULL;
}


/**
 * @brief On the final screen before setting the WiFi SSID and password allow
 *        the user to display the WiFi password so that they can check it was
 *        entered correctly.
 *        Each time this function is called the password is either displayed of
 *        replaced with * characters.
 */
static void toggle_password_visibility(void) {
    if( password_value_label && password_visibility_label ) {
        hide_password=!hide_password;
        if( hide_password ) {
            int pw_char_count = strlen(wifi_password);
            if( pw_char_count > 0 ) {
                char pw_char_buf[pw_char_count+1];
                int char_index=0;
                for( char_index=0 ; char_index<pw_char_count ; char_index++ ) {
                    pw_char_buf[char_index]='*';
                }
                pw_char_buf[pw_char_count]=0;
                lv_label_set_text(password_value_label, pw_char_buf);
            }
            lv_label_set_text(password_visibility_label, SHOW_PASSWORD_LABEL_TEXT);
        }
        else {
            lv_label_set_text(password_value_label, wifi_password);
            lv_label_set_text(password_visibility_label, HIDE_PASSWORD_LABEL_TEXT);
        }
    }
}

/**
 * @brief handler events generated from all elements of the GUI.
 * @param obj A pointer to the GUI object that sent the event.
 * @param event The event identifier.
 */
static void event_handler(lv_obj_t * obj, lv_event_t event)
{

    if( obj == ssid_text_area && event == LV_EVENT_CLICKED ) {
        show_keyboad(ssid_text_area);
    }
    else if( obj == password_text_area && event == LV_EVENT_CLICKED ) {
        show_keyboad(password_text_area);
    }
    else if( obj == scan_button && event == LV_EVENT_CLICKED ) {
        if( start_wifi_scan(&wifi_networks_scan_comnplete) ) {
            remove_ssid_list();
            create_wifi_scan_indicator();
        }
    }
    else if( back_button && obj == back_button && event == LV_EVENT_CLICKED ) {
        if( wizard_step == 1 ) {
            set_step_0();
            wizard_step--;
        }
        else if( wizard_step == 2 ) {
            set_step_1();
            wizard_step--;
        }
    }
    else if( next_button && obj == next_button && event == LV_EVENT_CLICKED ) {
        if( wizard_step == 0 ) {
            set_step_1();
            wizard_step++;
        }
        else if( wizard_step == 1 ) {
            set_step_2();
            wizard_step++;
        }
        else if( wizard_step == 2 ) {
            setup_wifi();
        }
    }
    else if( ddlist != NULL && obj == ddlist && event == LV_EVENT_VALUE_CHANGED) {
        set_selected_ssid();
    }
    else if( obj == keyboard ) {
        if (event == LV_EVENT_APPLY || event == LV_EVENT_CANCEL) {
            remove_widget(keyboard);
            keyboard = NULL;
        }
        else if( event == LV_EVENT_VALUE_CHANGED ) {

        }
    }
    else if( password_visibility_button && obj == password_visibility_button && event == LV_EVENT_CLICKED ) {
        toggle_password_visibility();
    }
}

/**
 * @brief Set the display up for step 0 (input SSID)
 */
static void set_step_0(void) {
    lv_obj_t * scan_label=NULL;

    remove_widget(back_button);
    back_button=NULL;

    //Remove GUI widgets from next step
    remove_widget(password_label);
    password_label=NULL;
    remove_widget(password_text_area);
    password_text_area=NULL;

    if( !scan_button ) {
        scan_button = lv_btn_create(lv_scr_act(), NULL);
        lv_obj_set_event_cb(scan_button, event_handler);
        lv_obj_align(scan_button, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
        scan_label = lv_label_create(scan_button, NULL);
        lv_label_set_text(scan_label, "Scan");
    }

    ssid_label = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(ssid_label, "WiFi SSID:");
    label_width = lv_obj_get_width(ssid_label);
    label_height = lv_obj_get_height(ssid_label);
    lv_obj_align(ssid_label, NULL, LV_ALIGN_IN_TOP_LEFT, MARGIN, BUTTON_HEIGHT+(2*MARGIN)+(label_height/2));
    ssid_text_area = lv_ta_create(lv_scr_act(), NULL);
    lv_ta_set_one_line(ssid_text_area, true);
    lv_obj_set_size(ssid_text_area, win_width-(3*MARGIN)-label_width, label_height+(2*MARGIN));
    lv_obj_align(ssid_text_area, NULL, LV_ALIGN_IN_TOP_LEFT, label_width+(2*MARGIN), BUTTON_HEIGHT+(3*MARGIN));
    lv_ta_set_cursor_type(ssid_text_area, LV_CURSOR_BLOCK);
    lv_ta_set_text(ssid_text_area, wifi_ssid);
    lv_obj_set_event_cb(ssid_text_area, event_handler);

    active_text_area = ssid_text_area;
}

/**
 * @brief Set the display up for step 1 (input WiFi password)
 */
static void set_step_1() {
    int label_height=0;

    //Copy the WiFi SSID entered on the previous step
    if( ssid_text_area ) {
        strncpy(wifi_ssid, lv_ta_get_text(ssid_text_area), MAX_SSID_LENGTH);
    }
    if( password_text_area ) {
        //Copy the WiFi password entered on the next step
        strncpy(wifi_password, lv_ta_get_text(password_text_area), MAX_PASSWORD_LENGTH);
    }

    //Remove GUI widgets from previous step
    remove_widget(scan_button);
    scan_button=NULL;
    remove_widget(ssid_label);
    ssid_label=NULL;
    remove_widget(ssid_text_area);
    ssid_text_area=NULL;
    remove_widget(ddlist);
    ddlist=NULL;
    remove_widget(ssid_value_label);
    ssid_value_label=NULL;
    remove_widget(password_value_label);
    password_value_label=NULL;
    remove_widget(password_label);
    password_label=NULL;
    remove_widget(password_text_area);
    password_text_area=NULL;
    remove_widget(info_label);
    info_label=NULL;
    remove_widget(password_visibility_button);
    password_visibility_button=NULL;
    remove_widget(back_button);
    back_button=NULL;

    back_button = lv_btn_create(lv_scr_act(), NULL);
    lv_obj_set_event_cb(back_button, event_handler);
    lv_obj_align(back_button, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
    back_label = lv_label_create(back_button, NULL);
    lv_label_set_text(back_label, "Back");

    lv_label_set_text(next_label, "Next");

    password_label = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(password_label, "WiFi Password:");
    label_width = lv_obj_get_width(password_label);
    label_height = lv_obj_get_height(password_label);
    lv_obj_align(password_label, NULL, LV_ALIGN_IN_TOP_LEFT, MARGIN, BUTTON_HEIGHT+(2*MARGIN)+(label_height/2));
    password_text_area = lv_ta_create(lv_scr_act(), NULL);
    lv_ta_set_pwd_mode(password_text_area, true);
    lv_ta_set_one_line(password_text_area, true);
    lv_obj_set_size(password_text_area, win_width-(3*MARGIN)-label_width, label_height+(2*MARGIN));
    lv_obj_align(password_text_area, NULL, LV_ALIGN_IN_TOP_LEFT, label_width+(2*MARGIN), BUTTON_HEIGHT+(3*MARGIN));
    lv_ta_set_cursor_type(password_text_area, LV_CURSOR_BLOCK);
    lv_ta_set_text(password_text_area, wifi_password);
    lv_obj_set_event_cb(password_text_area, event_handler);

    active_text_area = password_text_area;

}


/**
 * @brief Set the display up for step 2 (setup WiFi)
 */
static void set_step_2(void) {

    //Copy the WiFi password entered on the previous step
    strncpy(wifi_password, lv_ta_get_text(password_text_area), MAX_PASSWORD_LENGTH);

    remove_widget(password_label);
    password_label=NULL;
    remove_widget(password_text_area);
    password_text_area=NULL;

    ssid_label = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(ssid_label, "WiFi SSID:");
    label_width = lv_obj_get_width(ssid_label);
    label_height = lv_obj_get_height(ssid_label);
    lv_obj_align(ssid_label, NULL, LV_ALIGN_IN_TOP_LEFT, MARGIN, BUTTON_HEIGHT+(2*MARGIN)+(label_height/2));

    ssid_value_label = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(ssid_value_label, wifi_ssid);
    lv_obj_align(ssid_value_label, NULL, LV_ALIGN_IN_TOP_LEFT, label_width+(2*MARGIN), BUTTON_HEIGHT+(2*MARGIN)+(label_height/2) );

    password_label = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(password_label, "WiFi Password:");
    label_width = lv_obj_get_width(password_label);
    label_height = lv_obj_get_height(password_label);
    lv_obj_align(password_label, NULL, LV_ALIGN_IN_TOP_LEFT, MARGIN, 2*BUTTON_HEIGHT+(3*MARGIN)+(label_height/2));

    password_value_label = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(password_value_label, wifi_password);
    lv_obj_align(password_value_label, NULL, LV_ALIGN_IN_TOP_LEFT, label_width+(2*MARGIN), 2*BUTTON_HEIGHT+(3*MARGIN)+(label_height/2) );

    info_label = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(info_label, "Select Finish to setup WiFi");
    lv_label_set_text(next_label, "Finish");
    lv_obj_align(info_label, NULL, LV_ALIGN_IN_BOTTOM_LEFT, MARGIN, -BUTTON_HEIGHT-(2*MARGIN)-(2*label_height));

    password_visibility_button = lv_btn_create(lv_scr_act(), NULL);
    lv_obj_set_event_cb(password_visibility_button, event_handler);
    lv_obj_align(password_visibility_button, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
    password_visibility_label = lv_label_create(password_visibility_button, NULL);

    toggle_password_visibility();
}

/**
 * @brief Setup the Wifi. This involves a reboot.
 */
static void setup_wifi(void) {
    mgos_sys_config_set_wifi_sta_ssid(wifi_ssid);
    mgos_sys_config_set_wifi_sta_pass(wifi_password);
    save_cfg(&mgos_sys_config, NULL);
    mgos_system_restart();
}

/**
 * @brief Start the process of setting up the WiFi connection.
 */
void setup_wifi_start(void)
{

    /*Create a window to hold all the objects*/
    static lv_style_t win_style;
    lv_style_copy(&win_style, &lv_style_transp);
    win_style.body.padding.left= LV_DPI / MARGIN;
    win_style.body.padding.right = LV_DPI / MARGIN;
    win_style.body.padding.top = LV_DPI / MARGIN;
    win_style.body.padding.bottom = LV_DPI / MARGIN;
    win_style.body.padding.inner = LV_DPI / MARGIN;

    win = lv_win_create(lv_disp_get_scr_act(NULL), NULL);

    //Set the button size. Thios also sets the window height size
    lv_win_set_btn_size(win, BUTTON_HEIGHT);
    win_width = lv_obj_get_width(win);

    lv_win_set_title(win, "Setup WiFi");
    lv_page_set_scrl_layout(lv_win_get_content(win), LV_LAYOUT_PRETTY);
    lv_win_set_style(win, LV_WIN_STYLE_CONTENT, &win_style);
    lv_group_add_obj(group, lv_win_get_content(win));

    next_button = lv_btn_create(lv_scr_act(), NULL);
    lv_obj_set_event_cb(next_button, event_handler);
    lv_obj_align(next_button, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
    next_label = lv_label_create(next_button, NULL);
    lv_label_set_text(next_label, "Next");

    set_step_0();
}
