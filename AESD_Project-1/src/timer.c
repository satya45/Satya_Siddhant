/**
 * @file timer.c
 * @author Siddhant Jajoo and Satya Mehta
 * @brief All function related to timer have been defined in this file.
 * @version 0.1
 * @date 2019-03-28
 * 
 * @copyright Copyright (c) 2019
 * 
 */



#include "timer.h"


/**
 * @brief - This function creates and starts the respective timer.
 * 
 * @param timer_handle - This signifies which timer needs to be initialized.
 * The values can be:   TIMER_TEMP
 *                      TIMER_LIGHT
 *                      TIMER_HB
 * @return err_t - Error value (0 for success)
 */
err_t timer_init(uint8_t timer_handle)
{
    if (timer_handle == TIMER_TEMP)
    {
        timer_t timeout_temp;
        struct itimerspec trigger_temp;
        struct sigevent sev_temp;
        memset(&sev_temp, 0, sizeof(struct sigevent));
        memset(&trigger_temp, 0, sizeof(struct itimerspec));

        sev_temp.sigev_notify = SIGEV_THREAD;
        sev_temp.sigev_notify_function = &timer_handler;
        sev_temp.sigev_value.sival_int = timer_handle;
        timer_create(CLOCK_REALTIME, &sev_temp, &timeout_temp);

        //Setting the first timer interval and the repeating timer interval
        trigger_temp.it_value.tv_sec = TEMP_INTERVAL_SEC;
        trigger_temp.it_interval.tv_sec = TEMP_INTERVAL_SEC;
        trigger_temp.it_value.tv_nsec = TEMP_INTERVAL_NSEC;
        trigger_temp.it_interval.tv_nsec = TEMP_INTERVAL_NSEC;

        timer_settime(timeout_temp, 0, &trigger_temp, NULL);
    }
    else if (timer_handle == TIMER_LIGHT)
    {
        timer_t timeout_light;
        struct itimerspec trigger_light;
        struct sigevent sev_light;
        memset(&sev_light, 0, sizeof(struct sigevent));
        memset(&trigger_light, 0, sizeof(struct itimerspec));

        sev_light.sigev_notify = SIGEV_THREAD;
        sev_light.sigev_notify_function = &timer_handler;
        sev_light.sigev_value.sival_int = timer_handle;
        timer_create(CLOCK_REALTIME, &sev_light, &timeout_light);

        //Setting the first timer interval and the repeating timer interval
        trigger_light.it_value.tv_sec = LIGHT_INTERVAL_SEC;
        trigger_light.it_interval.tv_sec = LIGHT_INTERVAL_SEC;
        trigger_light.it_value.tv_nsec = LIGHT_INTERVAL_NSEC;
        trigger_light.it_interval.tv_nsec = LIGHT_INTERVAL_NSEC;

        timer_settime(timeout_light, 0, &trigger_light, NULL);
    }
    else if (timer_handle == TIMER_HB)
    {
        timer_t timeout_hb;
        struct itimerspec trigger_hb;
        struct sigevent sev_hb;
        memset(&sev_hb, 0, sizeof(struct sigevent));
        memset(&trigger_hb, 0, sizeof(struct itimerspec));

        sev_hb.sigev_notify = SIGEV_THREAD;
        sev_hb.sigev_notify_function = &timer_handler;
        sev_hb.sigev_value.sival_int = timer_handle;
        timer_create(CLOCK_REALTIME, &sev_hb, &timeout_hb);

        //Setting the first timer interval and the repeating timer interval
        trigger_hb.it_value.tv_sec = HB_INTERVAL_SEC;
        trigger_hb.it_interval.tv_sec = HB_INTERVAL_SEC;
        trigger_hb.it_value.tv_nsec = HB_INTERVAL_NSEC;
        trigger_hb.it_interval.tv_nsec = HB_INTERVAL_NSEC;

        timer_settime(timeout_hb, 0, &trigger_hb, NULL);
    }
    return OK;
}

/**
 * @brief - This function is invoked on timer expiration.
 * 
 * @param sigval - This parameter is passed in the timer_init function in their respective cases.
 */
void timer_handler(union sigval sv)
{
    if (sv.sival_int == TIMER_TEMP)
    {
        //pthread_mutex_lock(&mutex_a);
        temp_timerflag = 1;
        //timer_event |= TEMP_EVENT;
        //pthread_mutex_unlock(&mutex_a);
        //printf("In Timer Handler: Temperature Sensor Timer fired.\n");
    }
    else if (sv.sival_int == TIMER_LIGHT)
    {
        //pthread_mutex_lock(&mutex_a);
        light_timerflag = 1;
        //timer_event |= LIGHT_EVENT;
        //pthread_mutex_unlock(&mutex_a);
        //printf("In Timer Handler: Light Sensor Timer fired.\n");
    }
    else if (sv.sival_int == TIMER_HB)
    {
        hb_send(CLEAR_HB);
        printf("In Timer Handler: Heartbeat Timer fired.\n");
    }
}

