/*
 * scan_wifi~_networks.h
 *
 *  Created on: 28 Jul 2019
 *      Author: pja
 */

#ifndef SCAN_WIFI_NETWORKS_H_
#define SCAN_WIFI_NETWORKS_H_

#define MAX_WIFI_SCAN_MS 5000
#define MAX_WIFI_SCAN_SECONDS MAX_WIFI_SCAN_MS/1000

bool start_wifi_scan( void (*scan_done_fp)(char *ssid_list) );

#endif /* SCAN_WIFI_NETWORKS_H_ */
