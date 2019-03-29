/**
 * @file temp.h
 * @author Siddhant Jajoo and Satya Mehta
 * @brief Header file of temp.c
 * @version 0.1
 * @date 2019-03-28
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#ifndef _TEMP_H
#define _TEMP_H

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include "main.h"
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define TEMP_ADDR (0x48)
#define TEMP_REG (0)
#define CONFIG_REG (1)
#define THIGH_REG (3)
#define TLOW_REG (2)

char read_buff[2];

//Function Declarations
//err_t i2c_init(void);
err_t read_temp_reg(uint8_t);
sensor_struct read_temp_data(uint8_t temp_unit, uint8_t);
err_t write_pointer(uint8_t);
err_t shutdown_mode(void);
err_t write_tlow(uint16_t);
err_t write_thigh(uint16_t);
#endif