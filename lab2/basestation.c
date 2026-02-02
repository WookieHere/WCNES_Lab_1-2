#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "dev/leds.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"

/* Declare our "main" process, the basestation_process */
PROCESS(basestation_process, "Clicker basestation");
PROCESS(led_process, "led_turn_off");
/* The processes should be started automatically when
 * the node has booted. */
AUTOSTART_PROCESSES(&basestation_process, &led_process);

/* Callback function for received packets.
 *
 * Whenever this node receives a packet for its broadcast handle,
 * this function will be called.
 *
 */
static bool temp_flag = false;
static bool button_flag = false;
static void recv(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest) {
	printf("message received\n");
	if (*(char *)data == 't') {
		leds_on(0b1); // temperature warning
		temp_flag = true;
	}
	if (*(char *)data == 'b') { // button warning
		leds_on(0b10);
		button_flag = true;
	}
    leds_on(LEDS_ALL);
	process_poll(&led_process);
}

/* Our main process. */
PROCESS_THREAD(basestation_process, ev, data) {
	PROCESS_BEGIN();

	/* Initialize NullNet */
	nullnet_set_input_callback(recv);

	PROCESS_END();
}

/* Our led-turn-off process. */
PROCESS_THREAD(led_process, ev, data) {
	static struct etimer et;

	PROCESS_BEGIN();
	while(true) {
		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL); // we start the clock when an alarm is sounded
		while(true) {
			etimer_set(&et, CLOCK_SECOND*10);
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et) || ev == PROCESS_EVENT_POLL); //we wait until the clock expires or we get a new alarm
			//Determine if we are running because of the clock or a new alarm
			if(etimer_expired(&et)) {
				leds_off(LEDS_ALL); // if timer out, we turn off the leds, otherwise we just move on
				button_flag = false;
				temp_flag = false;
				break; //dont start the timer until an alarm is triggered again
			} else if(temp_flag && button_flag) { // if we received both kind of alarm, make it nice
				leds_on(LEDS_ALL);
			}
		}
	}
	PROCESS_END();
}