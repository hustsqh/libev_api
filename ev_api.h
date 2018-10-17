#ifndef __EV_API_H__
#define __EV_API_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    LIBEV_IO_READ,
    LIBEV_IO_WRITE,
}LIBEV_IO_ACTION;


extern const char *libev_get_err_str();

typedef void * ev_inst_st;
typedef void * ev_timer_id;
typedef void * ev_io_id;

extern void libev_set_log(void (*fun)(const char *));


extern int libev_api_add_event(ev_inst_st ev_st, void (*cb)(ev_inst_st inst, void *data), void *data);
extern ev_inst_st  libev_api_inst_create(void (*cb)(ev_inst_st inst, void *data), void *data); 
/* flag=0,use self thread, other create a new thread*/
extern int libev_api_inst_start(ev_inst_st *ev_st, int flag);
extern void libev_api_inst_destroy(ev_inst_st ev);
extern ev_timer_id libev_api_create_timer(ev_inst_st ev_st,  void (*cb)(ev_timer_id timer, void *data), void *data, double after, double repeat);
extern int libev_api_start_timer(ev_timer_id timer);
extern int libev_api_stop_timer(ev_timer_id timer);
extern int libev_api_restart_timer(ev_timer_id timer);
extern int libev_api_restart_timer_with_param(ev_timer_id timer, double after, double repeat);
extern int libev_api_get_timer_remain(ev_timer_id timer);
extern int libev_api_timer_is_running(ev_timer_id timer);
extern int libev_api_destroy_timer(ev_timer_id *timer);

extern ev_io_id libev_api_watchio_create(ev_inst_st ev_st, int fd, 
                        LIBEV_IO_ACTION act, void (*cb)(ev_io_id io, void *data), void *data);
extern int libev_api_watchio_start(ev_io_id io);
extern int libev_api_watchio_stop(ev_io_id io);
extern int libev_api_watchio_destroy(ev_io_id *io);
extern int libev_api_watchio_get_fd(ev_io_id *io);
extern ev_inst_st *libev_api_watchio_get_inst(ev_io_id *io);
/**
 * data must be destory in destory function, if data is null, destory can be null
 */
extern int libev_api_send_event_delay(libev_inst_st * ev_st, void(* cb)(ev_inst_st inst, void *data), 
                                        void(* destory)(void *data), void * data, double delay);
extern int libev_api_delete_event(libev_inst_st *ev_st, void (*cb)(ev_inst_st inst, void *data));


/* flag=0,use self thread, other create a new thread*/
extern int libev_api_sinst_start(void (*cb)(void *data), void *data, int flag);
extern void libev_api_sinst_stop();
extern int libev_api_sinst_add_event(void (*cb)(void *data), void *data);
extern ev_timer_id libev_api_sinst_create_timer(void (*cb)(ev_timer_id timer, void *data), 
                                                void *data, double after, double repeat);
extern int libev_api_sinst_start_timer(ev_timer_id *timer);
extern int libev_api_sinst_stop_timer(ev_timer_id timer);
extern int libev_api_sinst_restart_timer(ev_timer_id timer);
extern double libev_api_sinst_get_timer_remain(ev_timer_id timer);
extern int libev_api_sinst_timer_is_running(ev_timer_id timer);
extern int libev_api_sinst_destroy_timer(ev_timer_id *timer);
extern ev_io_id  libev_api_sinst_io_create(int fd, LIBEV_IO_ACTION act, 
                                            void (*cb)(ev_io_id io, void *data), void *data);
extern int libev_api_sinst_io_start(ev_io_id io);
extern int libev_api_sinst_io_stop(ev_io_id io);
extern int libev_api_sinst_io_destroy(ev_io_id *io);

#ifdef __cplusplus
}
#endif


#endif

