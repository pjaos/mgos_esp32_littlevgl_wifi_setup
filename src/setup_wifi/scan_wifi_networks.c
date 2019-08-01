/*
 * scan_wifi_networks.c
 *
 *  Created on: 28 Jul 2019
 *      Author: pja
 */

#include "mgos.h"
#include "mgos_wifi.h"

#include "lvgl/lvgl.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "scan_wifi_networks.h"

#define SSID_BUF_LEN 256

static char ssid_list_buf[SSID_BUF_LEN];
static void (*scan_done_fp)(char *ssid_list);
int    scan_start_time;

/**
 * @brief Wifi Scan callback. This function is called when a single Wifi network scan has been comnpleted.
 * @param network_count The number of WiFi networks detected. A negative value here indicates a scan failure.
 * @param res A pointer to a mgos_wifi_scan_result that contains details of the network found.
 * @pram arg NULL
 */
static void wifi_scan_cb(int network_count, struct mgos_wifi_scan_result *res, void *arg) {
    int scan_seconds = mgos_uptime() - scan_start_time;
    bool scan_complete = false;

    if( scan_seconds >= MAX_WIFI_SCAN_SECONDS ) {
        scan_complete = true;
    }
    else {
        if( network_count > 0 ) {
            for (int i = 0; i < network_count; i++) {
                //If we have not already seen this SSID
                if( !strstr(ssid_list_buf, res[i].ssid) ) {
                    //Add it to the ssid list
                    strncat(ssid_list_buf, res[i].ssid, SSID_BUF_LEN-2); //Leave space for \n and 0
                    strncat(ssid_list_buf, "\n", SSID_BUF_LEN-1); //Leave space for 0
                }
            }
        }
        if( strlen(ssid_list_buf) >= SSID_BUF_LEN-1 ) {
            scan_done_fp = NULL;
        }
        else {
            mgos_wifi_scan(wifi_scan_cb, NULL);
        }
    }

    if( scan_complete ) {
        if( strlen(ssid_list_buf) > 0 ) {
            int end = strlen(ssid_list_buf)-1; //The location of the last \n characterq
            if( end > 0 ) {
                ssid_list_buf[end]=0; //Remove last \n character as pulldown requires this.
            }
        }
        (*scan_done_fp)(ssid_list_buf);
        scan_done_fp = NULL;
    }

}

/**
 * @brief Start scanning WiFi networks
 * @param _scan_done_fp pointer to function to be called when the scan is complete.
 * @return True if scan started, false if scan already in progress.
 */
bool start_wifi_scan( void (*_scan_done_fp)(char *ssid_list) ) {
    bool scan_started = false;
    if( !scan_done_fp ) {
        scan_start_time = mgos_uptime();
        scan_done_fp = _scan_done_fp;
        scan_started= true;
        memset(ssid_list_buf, 0, SSID_BUF_LEN);
        mgos_wifi_scan(wifi_scan_cb, NULL);
    }
    return scan_started;
}

