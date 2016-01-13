#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "ev_api.h"

ev_timer_id timerid;

void timer_cb3(ev_timer_id timer, void *data)
{
    printf("timeover!!! %p-%p, %s\n", timerid, timer, (char *)data);

}


void timer_cb2(ev_timer_id timer, void *data)
{
    printf("timeover!!! %p-%p, %s\n", timerid, timer, (char *)data);
    libev_api_sinst_destroy_timer(&timerid);
    printf("timerid is %p\n", timerid);

    //timerid = libev_api_sinst_create_timer(timer_cb3, "third timer", 5, 5);
}


void timer_cb(ev_timer_id timer, void *data)
{
    printf("timeover!!! %p-%p, %s\n", timerid, timer, (char *)data);
    libev_api_sinst_destroy_timer(&timerid);
    printf("timerid is %p\n", timerid);
    timerid = libev_api_sinst_create_timer(timer_cb2, "second timer", 5.0, 0);
}

void event_cb(void *data)
{
    printf("%s\n", (char *)data);
    timerid = libev_api_sinst_create_timer(timer_cb, "first timer", 5.0, 0);
}

void init_cb(void *data)
{
    int ret = 0;
    printf("%s\n", (char *)data);
    ret = libev_api_sinst_add_event(event_cb, "first event");
    printf("libev_api_sinst_add_event return %d\n", ret);
}

double test_double(double x)
{
    if(x == 0.) printf("param x is 0.\n");

    return -1;
}




int main()
{
    int ret = 0;
    
    ret = libev_api_sinst_start(init_cb, "start init", 1);
    if(ret){
        printf("libev_api_sinst_start retun %d:%s\n", ret, libev_get_err_str());
        exit(1);
    }

    printf("return double %lf\n", test_double(0));

    ev_timer_id test_timer;

    test_timer = libev_api_sinst_create_timer(timer_cb3, "outside timer", 0., 5.0);
    printf("timer 3 %p:%s\n", test_timer, libev_get_err_str());

    
    
    while(1) {
        printf("wiat========\n");
        sleep(5);
        //libev_api_sinst_stop();
    }
    
    return 0;
}

