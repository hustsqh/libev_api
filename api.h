#ifndef __API_H__
#define __API_H__

#define INVALID_THREAD_ID (-1)
#define INVALID_FD (-1)

typedef enum
{
    LIBEV_E_OK,
    LIBEV_E_OUT_OF_MEM,
    LIBEV_E_SYSTEM,
    LIBEV_E_EVLIB,
    LIBEV_E_PARAM,
    LIBEV_E_INST_NOEXIST,
    LIBEV_E_MAX
}LIBEV_API_ERR_CODE_E;

typedef enum
{
    IO_ACT_READ,
    IO_ACT_WRITE,
}io_act_e;

typedef enum
{
    LIBEV_LOOP_INVALID,
    LIBEV_LOOP_RUN,
    LIBEV_LOOP_DONE
}LIBEV_LOOP_STATE;

typedef struct
{
    void (*fun)(void *data);
    void *data;
}libev_sfun_t;

typedef struct
{
    void (*fun)(void *ev, void *data);
    void *data;
}libev_data_t;

typedef struct
{
    void *ev;
    pthread_t thd_id;
    int pipefd[2];
#define read_fd  pipefd[0]
#define write_fd pipefd[1]
    ev_io io;
    LIBEV_LOOP_STATE state;
    libev_sfun_t init;
}libev_inst_st;


typedef struct libev_timer_s
{
	ev_timer timer;
	libev_data_t event;
    double after;
    double repeat;
    libev_inst_st *ev_st;
    void (*send_fun)(libev_inst_st *, struct libev_timer_s *);
}libev_timer_t;

typedef struct libev_io_s
{
    ev_io io;
    libev_data_t event;
    io_act_e act;
    int fd;
    libev_inst_st *ev_st;
    void (*send_fun)(libev_inst_st *, struct libev_io_s *);
}libev_io_t;

#define container_of(ptr, type, member) ({ \
	const typeof(((type *)0)->member) *__mptr = (ptr); \
	(type *)((char *)__mptr - offsetof(type, member));})

#ifdef EN_DEBUG
#include <stdarg.h>
//void log_msg(const char *file_name, size_t lineno, const char *fmt, ...);
#define LOG(...) \
    do{\
	printf("[%-30s:%5d]", __FILE__, __LINE__); \
        printf( __VA_ARGS__); \
    }while(0)
#else
#define LOG printf
#endif

#endif
