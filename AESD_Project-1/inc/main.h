#ifndef _MAIN_H
#define _MAIN_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

#define OK (0)
#define FAIL (1)
#define I2C_BUS	("/dev/i2c-2")
#define TIMER_TEMP	(1)
#define TIMER_LIGHT	(2)
//#define TEMP_EVENT	(0x01)
//#define LIGHT_EVENT	(0X02)
#define TEMP_UNIT	(1)		//Set 0 for degree celsius, 1 for kelvin, 2 for fahrenheit.

//Log Levels
#define INFO	(0x01)
#define WARNING	(0x02)
#define ERROR	(0x04)
#define	DEBUG	(0x08)

#define INFO_DEBUG		(0x0A)

//#define BACKUP_FILENAME			("backup")

pthread_mutex_t mutex_a;
pthread_mutex_t mutex_error;


int i2c_open;
char *filename;
volatile uint8_t temp_timerflag;
volatile uint8_t light_timerflag;
uint8_t g_ll;

//char *backup = BACKUP_FILENAME;


#define TEMP_RCV_ID (1)
#define LIGHT_RCV_ID (2)
#define ERROR_RCV_ID (3)
#define MSG_RCV_ID	(4)

typedef uint32_t err_t;


struct temp_struct
{
	float temp_c;
	struct timespec data_time;
	uint8_t logger_level;
};

struct light_struct
{
	float light;
	struct timespec data_time;
	uint8_t logger_level;
};

struct error_struct
{
	struct timespec data_time;
	err_t error_value;
	char error_str[50];
};

struct msg_struct
{
char msg_str[50];
};



typedef struct
{
	uint8_t id;
	union sensor_data 
	{
		struct temp_struct temp_data;
		struct light_struct light_data;
		struct error_struct error_data;
		struct msg_struct msg_data;

	}sensor_data;

} sensor_struct;

//Function Declarations
err_t create_threads(char *filename);
void *temp_thread(void *);
void *light_thread(void *);
void *logger_thread(void *filename);
void *sock_thread(void *filename);
err_t i2c_init(void);
sensor_struct read_error(char *error_str);
sensor_struct read_msg(char *msg_str);
void error_log(char *error_str);
void msg_log(char *msg_str, uint8_t loglevel);

#endif
