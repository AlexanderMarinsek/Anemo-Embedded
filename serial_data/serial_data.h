#ifndef SERIAL_SEND_DATA_H
#define SERIAL_SEND_DATA_H

#include <stdint.h>
#include <stddef.h>		// size_t

/* Determine payload length based on which modules you use */
#define SERIAL_DATA_PAYLOAD_LEN		512
#define SERIAL_DATA_OTHER_LEN		64
#define SERIAL_DATA_BUF_LEN			\
	SERIAL_DATA_PAYLOAD_LEN + SERIAL_DATA_OTHER_LEN

#define SERIAL_DATA_BALAST_LEN		64

#define SERIAL_DATA_JSON_FORMAT		""\
	"{"\
		"\"hash\":\"%s\","\
		"\"status\":%u,"\
		"\"error\":%u,"\
		"\"data\":%s"\
	"}"\
	"\n"

//#define DEVICE_HASH_LEN				32+1
//#define DEVICE_HASH_BUF_LEN			32 + DEVICE_HASH_LEN
//#define DEVICE_HASH_JSON_FORMAT     "\"hash\":\"%s\""


int8_t init_serial_data (size_t data_buf_len, char *hash, size_t hash_len);

int8_t send_serial_data (char *data_buff, uint16_t config, uint16_t error);




#endif
