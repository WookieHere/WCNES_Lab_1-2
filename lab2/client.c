#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "dev/sensor/sht11/sht11-sensor.h"

/* Declare our "main" process, the client process*/
PROCESS(client_process, "Clicker client");
PROCESS(remote_sht11_process, "SHT11 test");
/* The client process should be started automatically when
 * the node has booted. */
AUTOSTART_PROCESSES(&client_process, &remote_sht11_process);

/* Callback function for received packets.
 *
 * Whenever this node receives a packet for its broadcast handle,
 * this function will be called.
 *
 * As the client does not need to receive, the function does not do anything
 */
static void recv(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest) {
}

/* Our main process. */
PROCESS_THREAD(client_process, ev, data) {
	static char payload[] = "hej";

	PROCESS_BEGIN();

	/* Activate the button sensor. */
	SENSORS_ACTIVATE(button_sensor);

	/* Initialize NullNet */
	nullnet_buf = (uint8_t *)&payload;
	nullnet_len = sizeof(payload);
	nullnet_set_input_callback(recv);

	/* Loop forever. */
	while (1) {
		//wait until temp sensor triggers a warning
		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);

		leds_toggle(LEDS_RED);
		/* Copy the string "hej" into the packet buffer. */
		memcpy(nullnet_buf, &payload, sizeof(payload));
    nullnet_len = sizeof(payload);

		/* Send the content of the packet buffer using the
		 * broadcast handle. */
		NETSTACK_NETWORK.output(NULL);
	}

	PROCESS_END();
}

/*---------------------------------------------------------------------------*/
static struct etimer et;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(remote_sht11_process, ev, data)
{
  int16_t temperature;

  PROCESS_BEGIN();
  SENSORS_ACTIVATE(sht11_sensor);

  /* Let it spin and read sensor data */
  while(1) {
    etimer_set(&et, CLOCK_SECOND/100); //hundredth of a second wait
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    temperature = sht11_sensor.value(SHT11_SENSOR_TEMP);;
    printf("Temperature %02d.%02d ºC\n ", temperature / 100, temperature % 100);
	if (temperature > 2300) {
	  process_poll(&client_process); //tell the client process to send alert
	}
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/