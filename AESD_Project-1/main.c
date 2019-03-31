
/**
 * @file main.c
 * @author Siddhant Jajoo and Satya Mehta
 * @brief This is a multithreaded C code which records temperature and light values at regular intervals
 * of time using a synchronized I2C and sends it to the logger thread using message queues to log into a
 * text file. A socket thread is also created to send data to a remote host on a socket request.  
 * @version 0.1
 * @date 2019-03-28
 * 
 * @copyright Copyright (c) 2019
 * 
 */

//#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "main.h"
#include "light.h"
#include "temp.h"
#include "logger.h"
#include "sockets.h"
#include "my_signal.h"
#include "queue.h"
#include "gpio.h"
#include "timer.h"

//Global Variables
pthread_t my_thread[4];
pthread_attr_t my_attributes;
volatile static uint32_t temp_hb_value;
volatile static uint32_t light_hb_value;
volatile static uint32_t logger_hb_value;

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		printf("ERROR: Wrong number of parameters.\n");
		printf("Input first parameter = name of log file; second parameter = log level: 'info' or 'warning' or 'error' or 'debug'.\n");
		exit(EXIT_FAILURE);
	}

	if ((strcmp(argv[1], "info") == 0) || (strcmp(argv[1], "warning") == 0) || (strcmp(argv[1], "error") == 0) || (strcmp(argv[1], "debug") == 0))
	{
		printf("ERROR: Filename not valid ; Choose another filename, cannot choose filename same as log levels.\n");
		exit(EXIT_FAILURE);
	}

	if ((strcmp(argv[2], "info") != 0) && (strcmp(argv[2], "warning") != 0) && (strcmp(argv[2], "error") != 0) && (strcmp(argv[2], "debug") != 0))
	{
		printf("ERROR: Invalid Log Level; Valid Log Levels: 'info' or 'warning' or 'error' or 'debug'.\n");
		exit(EXIT_FAILURE);
	}

	filename = argv[1];
	if (!(strcmp(argv[2], "info")))
	{
		g_ll = INFO;
	}
	else if (!(strcmp(argv[2], "debug")))
	{
		g_ll = DEBUG;
	}
	else if (!(strcmp(argv[2], "warning")))
	{
		g_ll = WARNING;
	}
	else if (!(strcmp(argv[2], "error")))
	{
		g_ll = ERROR;
	}

	//Initializing global variables
	temp_timerflag = 0;
	light_timerflag = 0;
	main_exit = 0;

	srand(time(NULL));
	gpio_init(LED1);
	gpio_init(LED2);
	gpio_init(LED3);
	gpio_init(LED4);
	sig_init();
	queue_init();
	i2c_init();
	mutex_init();
	printf("Before light id\n");

	if (remove(filename))
	{
		error_log("ERROR: remove(); cannot delete log file", ERROR_DEBUG, P2);
	}

	create_threads(filename);

	timer_init(TIMER_HB);

	msg_log("Reached main while loop.\n", DEBUG, P0);

	while (!main_exit)
	{
		hb_handle(hb_receive());
	}

	msg_log("Exiting Main while loop.\n", DEBUG, P0);
	msg_log("Destroying all initialized elements.\n", DEBUG, P0);

	destroy_all();

	for (int i = 0; i < 4; i++)
	{
		pthread_join(my_thread[i], NULL);
	}

	return OK;
}

/**
 * @brief - This thread records temperature from the sensor at regular intervals of time and sends the data
 * 			to the logger thread using message queues.
 * 			It also send data to the socket thread using message queues.
 * 
 * @param filename - This is the textfile name that is passed to the thread. This is obtained as a 
 * 					command line argument.
 * @return void* 
 */
void *temp_thread(void *filename)
{
	msg_log("Entered Temperature Thread.\n", DEBUG, P0);
	timer_init(TIMER_TEMP);
	//sensor_struct data_send;

	while (1)
	{
		usleep(1);
		if (temp_timerflag)
		{
			pthread_mutex_lock(&mutex_b);

			//Insert Mutex lock here
			temp_timerflag = 0;
			queue_send(log_mq, read_temp_data(TEMP_UNIT, TEMP_RCV_ID), INFO_DEBUG, P0);

			// data_send.id = SOCK_RCV_ID;
			// data_send.sensor_data.temp_data.temp_c = rand();
			//queue_send(log_mq, data_send, INFO_DEBUG, P0);

			msg_log("Temperature Timer event handled.\n", DEBUG, P0);
			//Insert mutex Unlock here
			pthread_mutex_unlock(&mutex_b);
			hb_send(TEMP_HB);
		}

		pthread_mutex_lock(&mutex_b);
		if (socket_flag & TC)
		{
			// queue_send(log_mq, data_send, INFO_DEBUG, P0);
			// queue_send(sock_mq, data_send, INFO_DEBUG, P0);
			queue_send(log_mq, read_temp_data(0, SOCK_TEMP_RCV_ID), INFO_DEBUG, P0);
			queue_send(sock_mq, read_temp_data(0, SOCK_TEMP_RCV_ID), INFO_DEBUG, P0);
			msg_log("Temp in celsius socket request event handled", DEBUG, P0);
			socket_flag &= (~TC);
		}
		else if (socket_flag & TK)
		{
			//queue_send(log_mq, data_send, INFO_DEBUG);
			//queue_send(sock_mq, data_send, INFO_DEBUG);
			socket_flag &= (~TK);
			queue_send(log_mq, read_temp_data(1, SOCK_TEMP_RCV_ID), INFO_DEBUG, P0);
			queue_send(sock_mq, read_temp_data(1, SOCK_TEMP_RCV_ID), INFO_DEBUG, P0);
			msg_log("Temp in kelvin socket request event handled", DEBUG, P0);
		}
		else if (socket_flag & TF)
		{
			socket_flag &= (~TF);
			queue_send(log_mq, read_temp_data(2, SOCK_TEMP_RCV_ID), INFO_DEBUG, P0);
			queue_send(sock_mq, read_temp_data(2, SOCK_TEMP_RCV_ID), INFO_DEBUG, P0);
			msg_log("Temp in fahrenheit socket request event handled", DEBUG, P0);
		}

		pthread_mutex_unlock(&mutex_b);
	}
}

/**
 * @brief - This thread records the lux value from the sensor at regular intervals of time and sends the 
 * 			data to the logger file using message queues.
 * 
 * @param filename - This is the textfile name that is passed to the thread. This is obtained as a 
 * 					command line argument.
 * @return void* 
 */
void *light_thread(void *filename)
{
	msg_log("Entered Light Thread.\n", DEBUG, P0);
	timer_init(TIMER_LIGHT);
	interrupt();
	//sensor_struct data_send;
	uint16_t ch0, ch1;
	read_light_data(LIGHT_RCV_ID);
	write_int_th(0x60,1);
	uint16_t th = read_int_th(1);
	printf("Threshold read %x\n", th);
	write_int_ctrl(0x00);
	write_int_ctrl(0x11);
	read_light_reg(INT_CTRL);

	while (1)
	{
		int rcv = gpio_poll();
		if(rcv == 0)
		{
			write_command(0x40);
		}
		usleep(1);
		if (light_timerflag)
		{
			ch0 = read_adc0();
			printf("Channel 0 data %x\n", ch0);
			if (ch0 > 75)
			{
				gpio_ctrl(GPIO54, GPIO54_V, 1);
			}
			else
			{
				gpio_ctrl(GPIO54, GPIO54_V, 0);
			}
			// 	ch1 = ADC_CH1();
			// 	printf("Channel 0 data %d\n",ch1);

			pthread_mutex_lock(&mutex_b);
			//printf("In main loop: Light timer event handled.\n");
			//Insert Mutex lock here
			light_timerflag = 0;
			queue_send(log_mq, read_light_data(LIGHT_RCV_ID), INFO_DEBUG, P0);

			// data_send.id = LIGHT_RCV_ID;
			// data_send.sensor_data.light_data.light = rand() % 10;
			//queue_send(log_mq, data_send, INFO_DEBUG, P0);

			msg_log("Light Timer event handled.\n", DEBUG, P0);
			//Insert mutex Unlock here
			pthread_mutex_unlock(&mutex_b);
			hb_send(LIGHT_HB);
		}
		if (socket_flag & L)
		{
			socket_flag &= (~L);
			queue_send(log_mq, read_light_data(SOCK_LIGHT_RCV_ID), INFO_DEBUG, P0);
			queue_send(sock_mq, read_light_data(SOCK_LIGHT_RCV_ID), INFO_DEBUG, P0);
			msg_log("Light socket request event handled", DEBUG, P0);
		}
	}
}

/**
 * @brief - This thread receives data from all the threads using message queues and prints the data to a 
 * 			textfile.
 * 
 * @param filename - This is the textfile name that is passed to the thread. This is obtained as a 
 * 					command line argument.
 * @return void* 
 */
void *logger_thread(void *filename)
{
	msg_log("Entered Logger Thread.\n", DEBUG, P0);
	while (1)
	{
		usleep(1);
		log_data(queue_receive(log_mq));

		hb_send(LOGGER_HB);
	}
}

/**
 * @brief - This thread initializes the socket and waits for a connection to be established from a
 * 			remote host. On socket connection it sends the requested data to the remote host.
 * 
 * @param filename - This is the textfile name that is passed to the thread. This is obtained as a 
 * 					command line argument.
 * @return void* 
 */
void *sock_thread(void *filename)
{
	msg_log("Entered Socket Thread.\n", DEBUG, P0);
	socket_init();
	while (1)
	{
		usleep(1);
		socket_listen();
		handle_socket_req();
		//pthread_mutex_lock(&mutex_b);
		socket_send(queue_receive(sock_mq));
		//pthread_mutex_unlock(&mutex_b);
	}
}

/**
 * @brief - Create 4 different threads.
 * 
 * @param filename - This is the textfile name that is passed to the thread. This is obtained as a 
 * 					command line argument.
 * @return err_t - Returns error value. (0 for success).
 */
err_t create_threads(char *filename)
{
	if (pthread_create(&my_thread[0],		   // pointer to thread descriptor
					   (void *)&my_attributes, // use default attributes
					   temp_thread,			   // thread function entry point
					   (void *)0))			   // parameters to pass in
	{

		error_log("ERROR: pthread_create(); in create_threads function, temp_thread not created", ERROR_DEBUG, P2);
		//Send Termination Signal
	}

	if (pthread_create(&my_thread[1],		   // pointer to thread descriptor
					   (void *)&my_attributes, // use default attributes
					   light_thread,		   // thread function entry point
					   (void *)0))			   // parameters to pass in

	{
		error_log("ERROR: pthread_create(); in create_threads function, light_thread not created", ERROR_DEBUG, P2);
	}

	if (pthread_create(&my_thread[2],		   // pointer to thread descriptor
					   (void *)&my_attributes, // use default attributes
					   logger_thread,		   // thread function entry point
					   (void *)filename))	  // parameters to pass in

	{
		error_log("ERROR: pthread_create(); in create_threads function, logger_thread not created", ERROR_DEBUG, P2);
	}
	if (pthread_create(&my_thread[3],		   // pointer to thread descriptor
					   (void *)&my_attributes, // use default attributes
					   sock_thread,			   // thread function entry point
					   (void *)filename))	  // parameters to pass in

	{
		error_log("ERROR: pthread_create(); in create_threads function, sock_thread not created", ERROR_DEBUG, P2);
	}

	return OK;
}

/**
 * @brief - This function initializes I2C.
 * 
 * @return err_t
 */
err_t i2c_init(void)
{
	if ((i2c_open = open(I2C_BUS, O_RDWR)) < 0)
	{
		error_log("ERROR: i2c_open(); in i2c_init() function", ERROR_DEBUG, P2);
	}
}

/**
 * @brief - This function initializes all the mutexes
 * 
 * @return err_t 
 */
err_t mutex_init(void)
{
	if (pthread_mutex_init(&mutex_a, NULL))
	{
		error_log("ERROR: pthread_mutex_init(mutex_a); mutex_a not created", ERROR_DEBUG, P2);
	}

	if (pthread_mutex_init(&mutex_b, NULL))
	{
		error_log("ERROR: pthread_mutex_init(mutex_b); mutex_b not created", ERROR_DEBUG, P2);
	}

	if (pthread_mutex_init(&mutex_error, NULL))
	{
		error_log("ERROR: pthread_mutex_init(mutex_error); mutex_error not created", ERROR_DEBUG, P2);
	}

	return OK;
}

/**
 * @brief - This function reads the error values i.e errno and stores it in a local structure.
 * 
 * @param error_str - The error string that needs to be printed in the text file.
 * @return sensor_struct 
 */
sensor_struct read_error(char *error_str)
{
	sensor_struct read_data;
	read_data.id = ERROR_RCV_ID;

	if (clock_gettime(CLOCK_REALTIME, &read_data.sensor_data.temp_data.data_time))
	{
		error_log("ERROR: clock_gettime(); in read_temp_data() function", ERROR_DEBUG, P2);
	}

	//Errno is thread safe , no mutex required.
	read_data.sensor_data.error_data.error_value = errno;

	//Cannot use a pointer, the below statement is wrong.
	//The below line will pass a pointer and no mutex is required as \
	everytime a new object will be created local to this function. \
	This will not change the value of the string at that particular pointer \
	as everytime this function is called a new objectwill be created giving \
	us a different pointer.
	strcpy(read_data.sensor_data.error_data.error_str, error_str);

	return read_data;
}

/**
 * @brief - This function stores the string that needs to be printed in the text file in a structure.
 * 
 * @param msg_str - The string that needs to be printed in the text file.
 * @return sensor_struct 
 */
sensor_struct read_msg(char *msg_str)
{
	sensor_struct read_data;
	read_data.id = MSG_RCV_ID;

	//Cannot use a pointer , the below statement is wrong.
	//The below line will pass a pointer and no mutex is required as \
	everytime a new object will be created local to this function. \
	This will not change the value of the string at that particular pointer \
	as everytime this function is called a new objectwill be created giving \
	us a different pointer.
	strcpy(read_data.sensor_data.msg_data.msg_str, msg_str);

	//pthread_mutex_unlock(&mutex_error);
	return read_data;
}

/**
 * @brief - This function sends the error string to be printed in the textfile to the logger thread via
 * 			message queue and prints it in the textile. 
 * 
 * @param error_str - The error string that needs to be printed in the textfile.
 */
void error_log(char *error_str, uint8_t loglevel, uint8_t prio)
{
	queue_send(log_mq, read_error(error_str), loglevel, prio);
}

/**
 * @brief - This function sends the message string to be printed in the textfile to the logger thread via
 * 			message queue and prints it in the textile. 
 * 
 * @param str - The message string that needs to be printed in the textfile.
 * @param loglevel - The loglevel of the message.
 */
void msg_log(char *str, uint8_t loglevel, uint8_t prio)
{
	queue_send(log_mq, read_msg(str), loglevel, prio);
}

/**
 * @brief - This function sends heartbeat to the main loop.
 * 
 * @param hb_value - This value specifies the heartbeat is from which thread. 
 * The values can be : 	TEMP_HB
 * 						LIGHT_HB
 * 						LOGGER_HB
 * 						SOCKET_HB
 * 						CLEAR_HB
 */
void hb_send(uint8_t hb_value)
{
	ssize_t res;
	res = mq_send(heartbeat_mq, (char *)&hb_value, sizeof(uint8_t), 0);
	if (res == -1)
	{
		//How to identify error due to which queue???? think!!!!!
		error_log("ERROR: mq_send(); in queue_send() function", ERROR_DEBUG, P2);
	}
}

/**
 * @brief - This function reads the value received from the messgae queue.
 * 
 * @return uint8_t - Returns the value signifying from which thread it has received the value.
 */
uint8_t hb_receive(void)
{
	ssize_t res;
	uint8_t hb_rcv;
	res = mq_receive(heartbeat_mq, (char *)&hb_rcv, sizeof(sensor_struct), NULL);
	if (res == -1)
	{
		error_log("ERROR: mq_receive(); in queue_receive() function", ERROR_DEBUG, P2);
	}
	return hb_rcv;
}

/**
 * @brief - This function handles the action after receiving the value from the hb_receive() function.
 * 
 * @param hb_rcv - This value is returned from the hb_receive() function.
 */

void hb_handle(uint8_t hb_rcv)
{
	switch (hb_rcv)
	{
	case TEMP_HB:
	{
		temp_hb_value++;
		msg_log("Temperaure Heartbeat received.\n", DEBUG, P0);
		break;
	}

	case LIGHT_HB:
	{
		light_hb_value++;
		msg_log("Light Heartbeat received.\n", DEBUG, P0);
		break;
	}

	case LOGGER_HB:
	{
		logger_hb_value++;
		//msg_log("Logger Heartbeat received.\n", DEBUG, P0);
		break;
	}

	case CLEAR_HB:
	{
		if (temp_hb_value == 0)
		{
			if (pthread_cancel(my_thread[0]))
			{
				error_log("ERROR: pthread_cancel(0); in hb_handle() function", ERROR_DEBUG, P2);
			}
			else
			{
				msg_log("Stopping temperature thread.\n", DEBUG, P0);
			}

			if (pthread_create(&my_thread[0],		   // pointer to thread descriptor
							   (void *)&my_attributes, // use default attributes
							   temp_thread,			   // thread function entry point
							   (void *)0))			   // parameters to pass in
			{
				error_log("ERROR: pthread_create(); in create_threads function, temp_thread not created", ERROR_DEBUG, P2);
			}
			else
			{
				msg_log("Resetting temperature thread.\n", DEBUG, P0);
			}

			//handle if heartbeats not received
		}
		else
		{
			temp_hb_value = 0;
		}

		if (light_hb_value == 0)
		{
			if (pthread_cancel(my_thread[1]))
			{
				error_log("ERROR: pthread_cancel(1); in hb_handle() function", ERROR_DEBUG, P2);
			}
			else
			{
				msg_log("Stopping light thread.\n", DEBUG, P0);
			}

			if (pthread_create(&my_thread[1],		   // pointer to thread descriptor
							   (void *)&my_attributes, // use default attributes
							   light_thread,		   // thread function entry point
							   (void *)0))			   // parameters to pass in
			{
				error_log("ERROR: pthread_create(); in create_threads function, temp_thread not created", ERROR_DEBUG, P2);
			}
			else
			{
				msg_log("Resetting light thread.\n", DEBUG, P0);
			}
			//handle if heartbeats not received
		}
		else
		{
			light_hb_value = 0;
		}

		if (logger_hb_value == 0)
		{
			if (pthread_cancel(my_thread[2]))
			{
				error_log("ERROR: pthread_cancel(2); in hb_handle() function", ERROR_DEBUG, P2);
			}
			else
			{
				msg_log("Stopping logger thread.\n", DEBUG, P0);
			}

			if (pthread_create(&my_thread[2],		   // pointer to thread descriptor
							   (void *)&my_attributes, // use default attributes
							   logger_thread,		   // thread function entry point
							   (void *)0))			   // parameters to pass in
			{
				error_log("ERROR: pthread_create(); in create_threads function, temp_thread not created", ERROR_DEBUG, P2);
			}
			else
			{
				msg_log("Resetting logger thread.\n", DEBUG, P0);
			}
			//handle if heartbeats not received
		}
		else
		{
			logger_hb_value = 0;
		}

		msg_log("Clearing all heartbeat values.\n", DEBUG, P0);
		break;
	}
	}
}

/**
 * @brief - Closes the i2c file descriptor
 * 
 * @return err_t 
 */
err_t i2c_close(void)
{
	if (close(i2c_open))
	{
		error_log("ERROR: close(); in i2c_close", ERROR_DEBUG, P2);
	}
}

/**
 * @brief - This function destroys all the mutexes created at the start.
 * 
 * @return err_t 
 */
err_t mutex_destroy(void)
{
	if (pthread_mutex_destroy(&mutex_a))
	{
		error_log("ERROR: pthread_mutex_destroy(mutex_a); cannot destroy mutex_a", ERROR_DEBUG, P2);
	}

	if (pthread_mutex_destroy(&mutex_b))
	{
		error_log("ERROR: pthread_mutex_destroy(mutex_b); cannot destroy mutex_b", ERROR_DEBUG, P2);
	}

	if (pthread_mutex_destroy(&mutex_error))
	{
		error_log("ERROR: pthread_mutex_destroy(mutex_error); cannot destroy mutex_error", ERROR_DEBUG, P2);
	}

	return OK;
}

err_t thread_destroy(void)
{
	if (pthread_cancel(my_thread[0]))
	{
		error_log("ERROR: pthread_cancel(0); in thread_destroy() function", ERROR_DEBUG, P2);
	}

	if (pthread_cancel(my_thread[1]))
	{
		error_log("ERROR: pthread_cancel(1); in thread_destroy() function", ERROR_DEBUG, P2);
	}

	if (pthread_cancel(my_thread[2]))
	{
		error_log("ERROR: pthread_cancel(2); in thread_destroy() function", ERROR_DEBUG, P2);
	}

	if (pthread_cancel(my_thread[3]))
	{
		error_log("ERROR: pthread_cancel(3); in thread_destroy() function", ERROR_DEBUG, P2);
	}
}

err_t destroy_all(void)
{
	thread_destroy();
	timer_del();
	mutex_destroy();
	queues_close();
	queues_unlink();
	i2c_close();

	FILE *fptr = fopen(filename, "a");
	fprintf(fptr, "Terminating gracefully due to signal.\n");
	printf("\nTerminating gracefully due to signal\n");
	fclose(fptr);
}