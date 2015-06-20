/*!																www.mycal.net			
 *---------------------------------------------------------------------------
 *! \yselect.c
 *  \brief Event Loop Code
 *																			
 *---------------------------------------------------------------------------
 * Version                                                                  -
 *		0.1 Original Version April 6, 2006									-        
 *
 *---------------------------------------------------------------------------    
 *                                                             				-
 * Copyright (C) 2006, Mycal.net							-
 *                                                                         	-
 * $Date: mwj 2006/04/06 20:35:55 $
 *
 *---------------------------------------------------------------------------
 *
 * Notes:
 *
 *
 *
*/
#if 0

#include "mytypes.h"
#include "config.h"
#include "yselect.h"
#include "yevent.h"
#include "debug.h"

#define YEVENT_READABLE 1
#define YEVENT_WRITABLE 2

typedef struct event_loop_data {


    EVENT_TIMER *timer_list;
}EVENT;

typedef struct event_handler {
    int fd;


}EVENT_HANDLER;


typedef struct event_handler {
    int     timeout;



}EVENT_TIMER;



/* Time event structure */
typedef struct aeTimeEvent {
    long long id; /* time event identifier. */
    long when_sec; /* seconds */
    long when_ms; /* milliseconds */
    aeTimeProc *timeProc;
    aeEventFinalizerProc *finalizerProc;
    void *clientData;
    struct aeTimeEvent *next;
} aeTimeEvent;


//aeCreateTimeEvent(loop, i*1000, print, NULL, NULL);

y_Create_Event()
{
}

int
y_event_loop(YEVENT *event)
{
    int ret=0;

    while(go)
    {
        // Calculate timeout

        ret=Y_Select(0);
    }
}


struct epoll_event_handler {
    int fd;
    void (*handle)(struct epoll_event_handler*, uint32_t);
    void* closure;
};


void epoll_do_reactor_loop()
{
    struct epoll_event current_epoll_event;

    while (1) {
        struct epoll_event_handler* handler;

        epoll_wait(epoll_fd, &current_epoll_event, 1, -1);
        handler = (struct epoll_event_handler*) current_epoll_event.data.ptr;
        handler->handle(handler, current_epoll_event.events);

        struct free_list_entry* temp;
        while (free_list != NULL) {
            free(free_list->block);
            temp = free_list->next;
            free(free_list);
            free_list = temp;
        }
    }

}

#endif