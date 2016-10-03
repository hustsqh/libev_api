#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "ev.h"
#include "event.h"
#include "api.h"


static pthread_key_t s_errcode_key = 0;
static pthread_once_t s_key_once = PTHREAD_ONCE_INIT;

void (*log_fun)(const char *) = NULL;
#if 1

static char *s_errcode_str[LIBEV_E_MAX] =
{
    "OK",
    "Out of mem",
    "System ERROR, please check errno",
    "Ev lib api error",
    "Api param error",
    "Ev lib api instan is not exist"
};

static void key_destructor(void *ptr)
{
    free(ptr);
}

static void key_init_once(void)
{
    pthread_key_create(&s_errcode_key, key_destructor);
}

void libev_set_log(void (*fun)(const char *str))
{
    log_fun = fun;
}

static void *get_key_ptr()
{
    pthread_once(&s_key_once, key_init_once);
    void *p = pthread_getspecific(s_errcode_key);
    if(p == NULL){
        p = malloc(sizeof(int));
        if(p){
            pthread_setspecific(s_errcode_key, p);
            *((int *)p) = 0;
        }
    }
    return p;
}

static int libev_errcode()
{
    int *p = (int *)get_key_ptr();
    if(!p) return 0;
    return *p;
}

static void set_errcode(int errcode)
{
    int *p = (int *)get_key_ptr();
    if(p) *p = errcode;
}

const char *libev_get_err_str()
{
    int err = libev_errcode();
    if(err >= LIBEV_E_MAX || err < 0) return "libev errnum err";

    return s_errcode_str[err];
}

static int check_self_thread(libev_inst_st *ev_st)
{
    return pthread_equal(ev_st->thd_id, pthread_self());
}
#if 0
void log_msg(const char *file_name, size_t lineno, const char *fmt, ...)
{
    va_list ap;
    char buf[512] = {0};
    int len = snprintf(buf, 512, "[%s:%d]",file_name,lineno);
    va_start(ap, fmt);\
    vsnprintf(buf+len,512-len,fmt,ap);
    va_end(ap);
    if(log_fun) log_fun(buf);
    else printf("%s",buf);
}
#endif
#endif

#if 1
static void libev_event_cb(struct ev_loop *loop, ev_io *io_w, int e)
{
    libev_sfun_t event;
    int rd_n = 0;

    memset(&event, 0, sizeof(event));
#if 0
    do{
        rd_n = read(io_w->fd, &event, sizeof(event));
        if(rd_n == sizeof(event)){
            event.fun(event.data);
        }else if(rd_n < 0 && (errno == EAGAIN || errno == EINTR)){
            LOG("read pipe error errno(%d:%s) continue\n", errno,strerror(errno));
        }else if(rd_n >= 0){
            LOG("read pipe len is not %u:%d\n", sizeof(event), rd_n);
        }else{
            LOG("read pipe error!(%d:%s)\n", errno, strerror(errno));
            set_errcode(LIBEV_E_SYSTEM);
        }
    }while(rd_n > 0);
#else
    do{
        rd_n += read(io_w->fd, (char *)&event + rd_n, sizeof(event) - rd_n);
        if(rd_n < 0 && (errno == EAGAIN || errno == EINTR)){
            LOG("read pipe error errno(%d:%s) continue\n", errno,strerror(errno));
            continue;
        }else if(rd_n > (int)sizeof(event)){
            LOG("read pipe fatal, read data over %u", sizeof(event));
            break;
        }else if(rd_n == (int)sizeof(event)){
            event.fun(event.data);
            break;
        }else{
            LOG("read pipe error!(%d:%s)\n", errno, strerror(errno));
            set_errcode(LIBEV_E_SYSTEM);
            break;
        }
    }while(1);
#endif
    return;
}

static libev_inst_st *libev_alloc()
{
    libev_inst_st *ev = NULL;

    ev = malloc(sizeof(libev_inst_st));
    if(!ev){
        set_errcode(LIBEV_E_OUT_OF_MEM);
        return NULL;
    }
    ev->ev = NULL;
    ev->thd_id = INVALID_THREAD_ID;
    ev->pipefd[0] = -1;
    ev->pipefd[1] = -1;
    ev->state = LIBEV_LOOP_INVALID;
    ev->init.fun = NULL;
    ev->init.data = NULL;

    return ev;
}


static void thread_clean(libev_inst_st *ev)
{
    if(ev){
        if(ev->ev)
            event_base_free(ev->ev);
        if(ev->read_fd != INVALID_FD)
            close(ev->read_fd);
        if(ev->write_fd != INVALID_FD)
            close(ev->write_fd);
        free(ev);
    }
}

static void * thread_start(void *param)
{
    libev_inst_st *ev = (libev_inst_st *)param;

    ev_io_init(&ev->io, libev_event_cb, ev->read_fd, EV_READ);
	
	ev_io_start(ev->ev, &ev->io);

    if(ev->init.fun) ev->init.fun(ev->init.data);

    int ret = event_base_loop(ev->ev, 0);

    thread_clean(ev);

    LOG("Event loop over, return %d!\n", ret);

    return NULL;
}

int libev_api_add_event(libev_inst_st *ev_st, void (*cb)(void *data), void *data)
{
    libev_sfun_t event;
    size_t write_n = 0;
    ssize_t ret = 0;

    if(!ev_st) {set_errcode(LIBEV_E_PARAM); return -1; }

    event.fun = cb;
    event.data = data;

    do{
        ret = write(ev_st->write_fd, (char *)&event + write_n, sizeof(event) - write_n);
        if(ret < 0 && (errno == EAGAIN || errno == EINTR)){
            LOG("write need to try again! errno %d(%s)", errno, strerror(errno));
            continue;
        }else if(ret < 0){
            LOG("write failed! (%d:%s)\n", errno, strerror(errno));
            set_errcode(LIBEV_E_SYSTEM);
            return -1;
        }else{
            write_n += ret;
            if(write_n > sizeof(event)){
                LOG("write failed! write data is over%d, %u", write_n, sizeof(event));
                return -1;
            }else if(write_n == sizeof(event)){
                break;
            }
        }
        
    }while(1);
    
    return 0;
}

/*
    called inside the loop
*/
static void libev_stoploop_self(void *data)
{
    libev_inst_st *ev = (libev_inst_st *)data;
    
    ev_break(ev->ev, EVBREAK_ALL);
}

/*
    should be called outside ev loop thread
*/
static void libev_thread_destroy(libev_inst_st *ev)
{
    if(ev){
        if(ev->thd_id != INVALID_THREAD_ID){
            libev_api_add_event(ev, libev_stoploop_self, ev);
        }else{
            thread_clean(ev);
        }
    }
}


libev_inst_st * libev_api_inst_create(void (*cb)(void *data), void *data)
{
    int ret = 0;
    libev_inst_st *ev = NULL;

    ev = libev_alloc();
    if(!ev){
        return NULL;
    }

    ev->ev = event_init();
    if(ev->ev == NULL){
        set_errcode(LIBEV_E_EVLIB);
        goto Error;
    }

    ret = pipe(ev->pipefd);
    if(ret){
        set_errcode(LIBEV_E_SYSTEM);
        goto Error;
    }
	
	fcntl(ev->read_fd,F_SETFL,fcntl(ev->read_fd,F_GETFL)|O_NONBLOCK);
	fcntl(ev->write_fd,F_SETFL,fcntl(ev->write_fd,F_GETFL)|O_NONBLOCK);

    ev->init.fun = cb;
    ev->init.data = data;

    return ev;
Error:
    libev_thread_destroy(ev);

    return NULL;    
}



int libev_api_inst_start(libev_inst_st **ev_st, int flag)
{
    int ret = 0;
    libev_inst_st *pev = *ev_st;

    if(flag){
        ret = pthread_create(&pev->thd_id, NULL, thread_start, (void *)pev);
        if(ret) {
            libev_thread_destroy(pev);
            set_errcode(LIBEV_E_SYSTEM);
            *ev_st = NULL;
            return -1;
        }
        pthread_setname_np(pev->thd_id, "libev thread");
        pev->state = LIBEV_LOOP_RUN;
    }else{
        pev->state = LIBEV_LOOP_RUN;
        pev->thd_id = pthread_self();
        pthread_setname_np(pev->thd_id, "libev thread");
        thread_start(pev);
    }
    return 0;
}



void libev_api_inst_destroy(libev_inst_st **ev)
{
    libev_inst_st *pev = NULL;

    if(ev == NULL || *ev == NULL) return;

    pev = *ev;

    if(check_self_thread(pev)){
        libev_stoploop_self(pev);
    }else{
        libev_thread_destroy(pev);
    }
    *ev = NULL;
}

static void libev_timer_cb(struct ev_loop *loop, ev_timer *w, int revents)
{
	libev_timer_t *pt = container_of(w, libev_timer_t, timer);
	
	LOG("timeout cb");
	
	if(pt->event.fun) pt->event.fun(pt, pt->event.data);
}

static void libev_create_timer(libev_inst_st *ev_st, libev_timer_t *ptimer)
{
	ev_timer_init(&ptimer->timer, libev_timer_cb, ptimer->after, ptimer->repeat);
}

static void create_timer_send_cb(void *data)
{
    libev_timer_t *pt = (libev_timer_t *)data;

    if(pt->send_fun) pt->send_fun(pt->ev_st, pt);
}

libev_timer_t *libev_api_create_timer(libev_inst_st *ev_st,  void (*cb)(void * timer, void *data), void *data, double after, double repeat)
{
    //int ret = 0;
    
    if(ev_st == NULL || cb == NULL) {set_errcode(LIBEV_E_PARAM); return NULL; }

    if(after == 0.) {set_errcode(LIBEV_E_PARAM); return NULL; }
    
    libev_timer_t *pt = malloc(sizeof(libev_timer_t));
	
	if(!pt) {
        set_errcode(LIBEV_E_OUT_OF_MEM);
        return NULL;
	}

    pt->event.fun = cb;
    pt->event.data = data;
    pt->after = after;
    pt->repeat = repeat;
    pt->ev_st = ev_st;
    pt->send_fun = NULL;

#if 1
    libev_create_timer(ev_st, pt);
#else
    if(check_self_thread(ev_st)){
        libev_create_timer(ev_st, pt);
    }else{
        pt->send_fun = libev_create_timer;
        ret = libev_api_add_event(ev_st, create_timer_send_cb, pt);
        if(ret){
            free(pt);
            return NULL;
        }
    }
#endif
    return pt;
}

static void libev_start_timer(libev_inst_st *ev_st, libev_timer_t *timer)
{
    ev_timer_start(ev_st->ev, &timer->timer);
}

static void libev_stop_timer(libev_inst_st *ev_st, libev_timer_t *timer)
{
	ev_timer_stop(ev_st->ev, &timer->timer);
}

static void libev_restart_timer(libev_inst_st *ev_st, libev_timer_t *timer)
{
	//ev_timer_again(ev_st->ev, &timer->timer);
	ev_timer_stop(ev_st->ev, &timer->timer);
    ev_timer_init(&timer->timer, libev_timer_cb, timer->after, timer->repeat);
    ev_timer_start(ev_st->ev, &timer->timer);
}

static double libev_get_timer_remain(libev_inst_st *ev_st, libev_timer_t *timer)
{
	return ev_timer_remaining(ev_st->ev, &timer->timer);
}

static void libev_destroy_timer(libev_inst_st *ev_st, libev_timer_t *timer)
{
    libev_stop_timer(ev_st, timer);
    free(timer);
}

int libev_api_start_timer(libev_timer_t *timer)
{
    if(!timer) {set_errcode(LIBEV_E_PARAM); return -1; }

    if(!(timer->ev_st)) {set_errcode(LIBEV_E_INST_NOEXIST); return -1; }
    
    if(check_self_thread(timer->ev_st)){
        libev_start_timer(timer->ev_st, timer);
    }else{
        timer->send_fun = libev_start_timer;
        return libev_api_add_event(timer->ev_st, create_timer_send_cb, timer);
    }
    return 0;
}


int libev_api_stop_timer(libev_timer_t *timer)
{
    if(!timer) {set_errcode(LIBEV_E_PARAM); return -1; }

    if(!(timer->ev_st)) {set_errcode(LIBEV_E_INST_NOEXIST); return -1; }
    
    if(check_self_thread(timer->ev_st)){
        libev_stop_timer(timer->ev_st, timer);
    }else{
        timer->send_fun = libev_stop_timer;
        return libev_api_add_event(timer->ev_st, create_timer_send_cb, timer);
    }
    return 0;
}

int libev_api_restart_timer(libev_timer_t *timer)
{
    if(!timer) {set_errcode(LIBEV_E_PARAM); return -1; }

    if(!(timer->ev_st)) {set_errcode(LIBEV_E_INST_NOEXIST); return -1; }
    
    if(check_self_thread(timer->ev_st)){
        libev_restart_timer(timer->ev_st, timer);
    }else{
        timer->send_fun = libev_restart_timer;
        return libev_api_add_event(timer->ev_st, create_timer_send_cb, timer);
    }
    return 0;
}

int libev_api_restart_timer_with_param(libev_timer_t *timer, 
                                                        double after, double repeat)
{
    if(!timer || after == 0.) {set_errcode(LIBEV_E_PARAM); return -1; }

    if(!(timer->ev_st)) {set_errcode(LIBEV_E_INST_NOEXIST); return -1; }

    timer->after = after;
    timer->repeat = repeat;

    return libev_api_restart_timer(timer);
}


double libev_api_get_timer_remain(libev_timer_t *timer)
{
    if(!timer) {set_errcode(LIBEV_E_PARAM); return -1; }

    if(!(timer->ev_st)) {set_errcode(LIBEV_E_INST_NOEXIST); return -1; }
    
    return libev_get_timer_remain(timer->ev_st, timer);
}


int libev_api_timer_is_running(libev_timer_t *timer)
{
    if(!timer) {set_errcode(LIBEV_E_PARAM); return -1; }

    return (ev_is_active(&timer->timer) ? 1 : 0);
}


int libev_api_destroy_timer(libev_timer_t **timer)
{
    if(!timer || !(*timer)) {set_errcode(LIBEV_E_PARAM); return -1; }

    if(!((*timer)->ev_st)) {set_errcode(LIBEV_E_INST_NOEXIST); return -1; }
    
    libev_timer_t *pt = *timer;
    
    if(check_self_thread((*timer)->ev_st)){
        libev_destroy_timer((*timer)->ev_st, pt);
    }else{
        pt->send_fun = libev_destroy_timer;
        return libev_api_add_event((*timer)->ev_st, create_timer_send_cb, pt);
    }
    
    *timer = NULL;

    return 0;
}

static void libev_io_cb(struct ev_loop *loop, ev_io *io, int e)
{
    libev_io_t *ev_io = NULL;

    ev_io = container_of(io, libev_io_t, io);

    if(ev_io->event.fun) ev_io->event.fun(ev_io, ev_io->event.data);

    return ;
}

static void libev_watch_io_init(libev_inst_st * ev_st, libev_io_t *io)
{
    int event = (io->act == IO_ACT_READ) ? EV_READ : EV_WRITE;

    ev_io_init(&io->io, libev_io_cb, io->fd, event);
	
	ev_io_start(ev_st->ev, &io->io);
}

static void libev_watch_io_start(libev_inst_st *ev_st, libev_io_t *io)
{
    ev_io_start(ev_st->ev, &io->io);
}


static void libev_watch_io_stop(libev_inst_st *ev_st, libev_io_t *io)
{
    ev_io_stop(ev_st->ev, &io->io);
}

static void libev_watch_io_destroy(libev_inst_st *ev_st, libev_io_t *io)
{
    ev_io_stop(ev_st->ev, &io->io);
    free(io);
}


static void watch_io_send_cb(void *data)
{
    libev_io_t *io = (libev_io_t *)data;

    if(io->send_fun) io->send_fun(io->ev_st, io);
}

/*
    fd must be unblock
*/
libev_io_t *libev_api_watchio_create(libev_inst_st *ev_st, int fd, 
                        io_act_e act, void (*cb)(void *io, void *data), void *data)
{
    if(!ev_st || fd == INVALID_FD) {set_errcode(LIBEV_E_PARAM); return NULL; }
    
    libev_io_t *io = malloc(sizeof(libev_io_t));

    if(io == NULL) {
        set_errcode(LIBEV_E_OUT_OF_MEM);
        return NULL;
    }

    io->event.fun = cb;
    io->event.data = data;
    io->act = act;
    io->fd = fd;
    io->ev_st = ev_st;

    if(check_self_thread(ev_st)){
        libev_watch_io_init(ev_st, io);
    }else{
        io->send_fun = libev_watch_io_init;
        libev_api_add_event(ev_st, watch_io_send_cb, io);
    }

    return io;
}

int libev_api_watchio_start(libev_io_t *io)
{
    if(!io) {set_errcode(LIBEV_E_PARAM); return -1; }

    if(!(io->ev_st)) {set_errcode(LIBEV_E_INST_NOEXIST); return -1; }
    
    if(check_self_thread(io->ev_st)){
        libev_watch_io_start(io->ev_st, io);
    }else{
        io->send_fun = libev_watch_io_start;
        libev_api_add_event(io->ev_st, watch_io_send_cb, io);
    }
    return 0;
}

int libev_api_watchio_get_fd(libev_io_t *io)
{
    if(!io) {set_errcode(LIBEV_E_PARAM); return -1; }

    return io->fd;
}

int libev_api_watchio_stop(libev_io_t *io)
{
    if(!io) {set_errcode(LIBEV_E_PARAM); return -1; }

    if(!(io->ev_st)) {set_errcode(LIBEV_E_INST_NOEXIST); return -1; }
    
    if(check_self_thread(io->ev_st)){
        libev_watch_io_stop(io->ev_st, io);
    }else{
        io->send_fun = libev_watch_io_stop;
        libev_api_add_event(io->ev_st, watch_io_send_cb, io);
    }
    return 0;
}

int libev_api_watchio_destroy(libev_io_t **io)
{
    if(!io || !(*io)) {set_errcode(LIBEV_E_PARAM); return -1; }

    if(!((*io)->ev_st)) {set_errcode(LIBEV_E_INST_NOEXIST); return -1; }
    
    libev_io_t *pio = *io;
    
    if(check_self_thread((*io)->ev_st)){
        libev_watch_io_destroy((*io)->ev_st, pio);
    }else{
        pio->send_fun = libev_watch_io_destroy;
        libev_api_add_event((*io)->ev_st, watch_io_send_cb, pio);
    }
    *io = NULL;
    
    return 0;
}


#endif


#if 1
static libev_inst_st *s_inst_ev_single = NULL;
//static int s_sinst_run = 0;

int libev_api_sinst_start(void (*cb)(void *data), void *data, int flag)
{
    int ret = 0;

    if(s_inst_ev_single == NULL)
    {
        s_inst_ev_single = libev_api_inst_create(cb, data);
        if(s_inst_ev_single == NULL)
        {
            return -1;
        }

        ret = libev_api_inst_start(&s_inst_ev_single, flag);
    }

    return ret;
}

void libev_api_sinst_stop()
{
    if(s_inst_ev_single)
    {
        libev_api_inst_destroy(&s_inst_ev_single);
    }
}


int libev_api_sinst_add_event(void (*cb)(void *data), void *data)
{
    if(!s_inst_ev_single) { set_errcode(LIBEV_E_INST_NOEXIST); return -1; }

    return libev_api_add_event(s_inst_ev_single, cb, data);
}

libev_timer_t * libev_api_sinst_create_timer(void (*cb)(void *timer, void *data), 
                                                void *data, double after, double repeat)
{
    if(!s_inst_ev_single) { set_errcode(LIBEV_E_INST_NOEXIST); return NULL; }

    return libev_api_create_timer(s_inst_ev_single, cb, data, after, repeat);
}

int libev_api_sinst_start_timer(libev_timer_t *timer)
{
    if(!s_inst_ev_single) { set_errcode(LIBEV_E_INST_NOEXIST); return -1; }

    return libev_api_start_timer(timer);
}

int libev_api_sinst_stop_timer(libev_timer_t *timer)
{
    if(!s_inst_ev_single) { set_errcode(LIBEV_E_INST_NOEXIST); return -1; }

    return libev_api_stop_timer(timer);
}

int libev_api_sinst_restart_timer(libev_timer_t *timer)
{
    if(!s_inst_ev_single) { set_errcode(LIBEV_E_INST_NOEXIST); return -1; }

    return libev_api_restart_timer(timer);
}

double libev_api_sinst_get_timer_remain(libev_timer_t *timer)
{
    if(!s_inst_ev_single) { set_errcode(LIBEV_E_INST_NOEXIST); return 0.; }

    return libev_api_get_timer_remain(timer);
}

int libev_api_sinst_timer_is_running(libev_timer_t *timer)
{
    return libev_api_timer_is_running(timer);
}

int libev_api_sinst_destroy_timer(libev_timer_t **timer)
{
    if(!s_inst_ev_single) { set_errcode(LIBEV_E_INST_NOEXIST); return -1; }

    return libev_api_destroy_timer(timer);
}

libev_io_t * libev_api_sinst_io_create(int fd, io_act_e act, 
                                            void (*cb)(void *io, void *data), void *data)
{
    if(!s_inst_ev_single) { set_errcode(LIBEV_E_INST_NOEXIST); return NULL; }

    return libev_api_watchio_create(s_inst_ev_single, fd, act, cb, data);
}

int libev_api_sinst_io_start(libev_io_t *io)
{
     if(!s_inst_ev_single) { set_errcode(LIBEV_E_INST_NOEXIST); return -1; }

     return libev_api_watchio_start(io);
}

int libev_api_sinst_io_stop(libev_io_t *io)
{
    if(!s_inst_ev_single) { set_errcode(LIBEV_E_INST_NOEXIST); return -1; }

    return libev_api_watchio_stop(io);
}

int libev_api_sinst_io_destroy(libev_io_t **io)
{
    if(!s_inst_ev_single) { set_errcode(LIBEV_E_INST_NOEXIST); return -1; }
    
    return libev_api_watchio_destroy(io);
}


#endif


