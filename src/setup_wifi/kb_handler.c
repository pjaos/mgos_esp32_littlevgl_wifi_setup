/*
 * kb_handler.c
 *
 *  Created on: 29 Jul 2019
 *      Author: pja
 */
#include "lvgl/lvgl.h"
#include "lv_kb.h"

#define LV_KB_CTRL_BTN_FLAGS (LV_BTNM_CTRL_NO_REPEAT | LV_BTNM_CTRL_CLICK_TRIG)

static const char * kb_map_lc[] = {"1#", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "Bksp", "\n",
                                   "ABC", "a", "s", "d", "f", "g", "h", "j", "k", "l", "Enter", "\n",
                                   "_", "-", "z", "x", "c", "v", "b", "n", "m", ".", ",", ":", "\n",
                                   LV_SYMBOL_CLOSE, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, ""};

static const lv_btnm_ctrl_t kb_ctrl_lc_map[] = {
    LV_KB_CTRL_BTN_FLAGS | 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 7,
    LV_KB_CTRL_BTN_FLAGS | 6, 3, 3, 3, 3, 3, 3, 3, 3, 3, 7,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    LV_KB_CTRL_BTN_FLAGS | 2, 2, 6, 2, LV_KB_CTRL_BTN_FLAGS | 2};

static const char * kb_map_uc[] = {"1#", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "Bksp", "\n",
                                   "abc", "A", "S", "D", "F", "G", "H", "J", "K", "L", "Enter", "\n",
                                   "_", "-", "Z", "X", "C", "V", "B", "N", "M", ".", ",", ":", "\n",
                                   LV_SYMBOL_CLOSE, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, ""};

static const lv_btnm_ctrl_t kb_ctrl_uc_map[] = {
    LV_KB_CTRL_BTN_FLAGS | 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 7,
    LV_KB_CTRL_BTN_FLAGS | 6, 3, 3, 3, 3, 3, 3, 3, 3, 3, 7,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    LV_KB_CTRL_BTN_FLAGS | 2, 2, 6, 2, LV_KB_CTRL_BTN_FLAGS | 2};

static const char * kb_map_spec[] = {"0", "1", "2", "3", "4" ,"5", "6", "7", "8", "9", "Bksp", "\n",
                                     "abc", "+", "-", "/", "*", "=", "%", "!", "?", "#", "<", ">", "\n",
                                     "\\",  "@", "$", "(", ")", "{", "}", "[", "]", ";", "\"", "'", "\n",
                                     LV_SYMBOL_CLOSE, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, ""};

static const lv_btnm_ctrl_t kb_ctrl_spec_map[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, LV_KB_CTRL_BTN_FLAGS | 2,
    LV_KB_CTRL_BTN_FLAGS | 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    LV_KB_CTRL_BTN_FLAGS | 2, 2, 6, 2, LV_KB_CTRL_BTN_FLAGS | 2};

/**
 * This is the same as the default keyboard execpt that when the tick key is selected the keyboard closes
 * as this appears to me to be the behaviour that a user would expect.
 * Otherwise it is the same as the default keyboard in lv_kb.c
 * Default keyboard event to add characters to the Text area and change the map.
 * If a custom `event_cb` is added to the keyboard this function be called from it to handle the
 * button clicks
 * @param kb pointer to a  keyboard
 * @param event the triggering event
 */
void lv_kb_pja_event_cb(lv_obj_t * kb, lv_event_t event)
{
    if(event != LV_EVENT_VALUE_CHANGED && event != LV_EVENT_LONG_PRESSED_REPEAT) return;

    lv_kb_ext_t * ext = lv_obj_get_ext_attr(kb);
    uint16_t btn_id   = lv_btnm_get_active_btn(kb);
    if(btn_id == LV_BTNM_BTN_NONE) return;
    if(lv_btnm_get_btn_ctrl(kb, btn_id, LV_BTNM_CTRL_HIDDEN | LV_BTNM_CTRL_INACTIVE)) return;
    if(lv_btnm_get_btn_ctrl(kb, btn_id, LV_BTNM_CTRL_NO_REPEAT) && event == LV_EVENT_LONG_PRESSED_REPEAT) return;

    const char * txt = lv_btnm_get_active_btn_text(kb);
    if(txt == NULL) return;

    /*Do the corresponding action according to the text of the button*/
    if(strcmp(txt, "abc") == 0) {
        lv_btnm_set_map(kb, kb_map_lc);
        lv_btnm_set_ctrl_map(kb, kb_ctrl_lc_map);
        return;
    } else if(strcmp(txt, "ABC") == 0) {
        lv_btnm_set_map(kb, kb_map_uc);
        lv_btnm_set_ctrl_map(kb, kb_ctrl_uc_map);
        return;
    } else if(strcmp(txt, "1#") == 0) {
        lv_btnm_set_map(kb, kb_map_spec);
        lv_btnm_set_ctrl_map(kb, kb_ctrl_spec_map);
        return;
    } else if(strcmp(txt, LV_SYMBOL_CLOSE) == 0) {
        if(kb->event_cb != lv_kb_pja_event_cb) {
            lv_res_t res = lv_event_send(kb, LV_EVENT_CANCEL, NULL);
            if(res != LV_RES_OK) return;
        } else {
            lv_kb_set_ta(kb, NULL); /*De-assign the text area  to hide it cursor if needed*/
            lv_obj_del(kb);
            return;
        }
        return;
    } else if(strcmp(txt, LV_SYMBOL_OK) == 0) {
        if(kb->event_cb != lv_kb_pja_event_cb) {
            lv_res_t res = lv_event_send(kb, LV_EVENT_APPLY, NULL);
            if(res != LV_RES_OK) return;
        } else {
            lv_kb_set_ta(kb, NULL); /*De-assign the text area to hide it cursor if needed*/
            lv_obj_del(kb);
        }
        return;
    }

    /*Add the characters to the text area if set*/
    if(ext->ta == NULL) return;

    if(strcmp(txt, "Enter") == 0)
        lv_ta_add_char(ext->ta, '\n');
    else if(strcmp(txt, LV_SYMBOL_LEFT) == 0)
        lv_ta_cursor_left(ext->ta);
    else if(strcmp(txt, LV_SYMBOL_RIGHT) == 0)
        lv_ta_cursor_right(ext->ta);
    else if(strcmp(txt, "Bksp") == 0)
        lv_ta_del_char(ext->ta);
    else if(strcmp(txt, "+/-") == 0) {
        uint16_t cur        = lv_ta_get_cursor_pos(ext->ta);
        const char * ta_txt = lv_ta_get_text(ext->ta);
        if(ta_txt[0] == '-') {
            lv_ta_set_cursor_pos(ext->ta, 1);
            lv_ta_del_char(ext->ta);
            lv_ta_add_char(ext->ta, '+');
            lv_ta_set_cursor_pos(ext->ta, cur);
        } else if(ta_txt[0] == '+') {
            lv_ta_set_cursor_pos(ext->ta, 1);
            lv_ta_del_char(ext->ta);
            lv_ta_add_char(ext->ta, '-');
            lv_ta_set_cursor_pos(ext->ta, cur);
        } else {
            lv_ta_set_cursor_pos(ext->ta, 0);
            lv_ta_add_char(ext->ta, '-');
            lv_ta_set_cursor_pos(ext->ta, cur + 1);
        }
    } else {
        lv_ta_add_text(ext->ta, txt);
    }
}
