/******************************************************************************
 * ESP Client Firmware
 * Copyright (C) 2015  Blair Wyatt
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
*******************************************************************************/

#include "osapi.h"
#include "os_type.h"
#include "ets_sys.h"
#include "user_interface.h"
#include "driver/uart.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1

#define printf(...) os_printf( __VA_ARGS__ )
#define sprintf(...) os_sprintf( __VA_ARGS__ )

#define MAX_CHAN 13 //Setting to 14 overflows to 0 for some reason


os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void user_procTask(os_event_t *events);
long clockCounter = 0;

struct scan_config scan_conf;

LOCAL os_timer_t scan_timer;

uint8  ssid[30][33];  //30 ssids
uint16 rssi[30];
uint8  chan[30];
int i = 0; 
int current_chan = 1;

int scan_complete;

void user_rf_pre_init(void) {}

static void ICACHE_FLASH_ATTR
scan_done(void* arg,STATUS status)
{
    
    //ets_uart_printf("%d\r\n",status);
    if (status == OK)
    {
       
        struct bss_info *bss_link = (struct bss_info *)arg;
        bss_link = bss_link->next.stqe_next; //Get first SSID
        
        while (bss_link != NULL && i < 30) //Max 30 nodes.
        {
            //ets_uart_printf("%s\r\n",bss_link->ssid);
            if (bss_link->ssid[0] == 'S' && 
                bss_link->ssid[1] == 'P' && 
                bss_link->ssid[2] == 'S') 
            {
                //os_memset(ssid, 0, 33);
                if (os_strlen(bss_link->ssid) <= 32)
                {
                    os_memcpy(ssid[i], bss_link->ssid, os_strlen(bss_link->ssid));
                    chan[i] = bss_link->channel;
                    rssi[i] = bss_link->rssi;
					//ets_uart_printf("SPS Found:  %d\r\n",chan[i]);
                    i++;
					
                }
            }

            bss_link = bss_link->next.stqe_next;
        }
        
    }
    scan_complete = true;
    
}    

void scanner(void *arg)
{
	scan_conf.channel = current_chan;
	//This prevents another scann happening before the previous has stopped.
    if (scan_complete)
    {
		if (current_chan <= MAX_CHAN) {
			scan_complete = false;
			wifi_station_scan(&scan_conf,scan_done);
			
			current_chan++;
        }
		else 
		{
			if (i > 0) {
				ets_uart_printf("S\r\n"); //Start
				ets_uart_printf("%d\r\n",i); //Print number of items in Node scan
				while (i > 0)
				{
					int j = 0;
					while (j < 31) // Subpos SSIDs are 31 long
					{
						ets_uart_printf("%02x",ssid[i-1][j]);
						j++;
					}
					ets_uart_printf(",%d,%d\r\n",rssi[i-1],chan[i-1]);
					i--;
				}
				
				ets_uart_printf("E\r\n"); //End
			}
			
			current_chan = 1;	
			i = 0;
		}
	}
}



static void ICACHE_FLASH_ATTR
user_procTask(os_event_t *events)
{
    os_delay_us(10);
}


//Init function 
void ICACHE_FLASH_ATTR
user_init(void)
{

    char ssid[32] = {0};
    scan_complete = true;
    
    // Configure the UART0 and UART1 (TX only) to 9600
	uart_init(BIT_RATE_115200,BIT_RATE_115200);
	
	scan_conf.ssid = NULL;
	scan_conf.bssid = NULL;
	scan_conf.show_hidden = 1;
	
    
    //Set station mode
    wifi_set_opmode_current( STATION_MODE );
	
    struct station_config conf;
    wifi_station_get_config_default(&conf);
	os_memcpy(&conf.ssid, ssid, 32);
	conf.bssid_set = 0;
	
	wifi_station_set_config_current(&conf);
    
    //Disarm timer
    os_timer_disarm(&scan_timer);

    //Setup timer
    os_timer_setfn(&scan_timer, (os_timer_func_t *)scanner, NULL);

    //&scan_timer is callback
    //delay in ms
    //0 for once and 1 for repeating
    //This callback timer value can't be too small as a new scan will start before the previous finishes. 
    //The callback function checks if the scan is complete before starting a new scan.
    os_timer_arm(&scan_timer, 10, 1); //100Hz
    
    //Start os task
    system_os_task(user_procTask, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
}
