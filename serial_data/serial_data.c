#include "serial_data.h"

#include "log.h"
#include "periph/uart.h"

#include <string.h>
#include <stdint.h>
#include <stddef.h>		// size_t
#include <stdlib.h>		// malloc

#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG (0)
#endif
#include "debug.h"

static size_t _payload_buf_len;
static char *_payload_buf;
static char *_hash_buf;

static void _rx_cb(void *arg, uint8_t data)
{
}

int8_t init_serial_data (size_t data_buf_len, char *hash, size_t hash_len) {

	/* Write hash to local buffer */
	_hash_buf = malloc(hash_len);
	snprintf(_hash_buf, hash_len, "%s", hash);

	/* Estimate payload buffer length */
	_payload_buf_len = data_buf_len + hash_len + SERIAL_DATA_BALAST_LEN;
	/* Allocate and clear memory for payload buffer */
	_payload_buf = malloc(_payload_buf_len);
	memset(_payload_buf, 0, _payload_buf_len);

	/* Check if memory allocation was OK (else NULL) */
	if (!_hash_buf || !_payload_buf) {
		LOG_ERROR("Failed: malloc\n");
		return -1;
	}

	/* Init UART device */
	int return_uart_init = uart_init(UART_DEV(1), 115200, _rx_cb, (void *)1);
	if (return_uart_init != 0) {
		LOG_ERROR("Failed: uart_init\n");
		return return_uart_init;
	}

	return 0;
}

int8_t send_serial_data (char *data_buf, uint16_t config, uint16_t error) {

	snprintf(_payload_buf, _payload_buf_len,
			SERIAL_DATA_JSON_FORMAT,
			_hash_buf, (unsigned int)config, (unsigned int)error, data_buf);

	//uart_write(UART_DEV(1), (uint8_t*)_payload_buf, _payload_buf_len);

	/* Avoid '\n', because it triggers an interrupt on the receiving RPi */
	//printf("%s\n", (uint8_t*)_payload_buf);
	printf("%s", (uint8_t*)_payload_buf);

	return 0;
}
