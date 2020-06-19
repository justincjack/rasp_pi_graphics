/**
 * Copyright (c) 2020 Justin Jack
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 **/
#include "video.h"

#define FB_FS_LOCATION  "/dev/fb%d"

#define _VWHITE64_      0xFFFFFFFFFFFFFFFF

union px_pointer {
    uint32_t    *ptr32;
    uint64_t    *ptr64;
    void        *ptr;
};

/**
 * Video mutex implementation
 * 
 **/ 
typedef struct video_mutex {
    pid_t           owner;
    pid_t           lock_count;
    pthread_mutex_t mutex;
}                       VMUTEX, *PVMUTEX;

struct video_setup {
    pid_t               pid;
    int                 fbid;

    int                 active;

    struct termios      term_prev,
                        term_curr;

    size_t              px_count,
                        px_count64,
                        width,
                        height;

    union px_pointer    ptr,
                        clrb;

    PVMUTEX             mtx_prerender;          /* Used so that only one thread at a time may access the pre-
                                                 * rendering buffer for this video display.
                                                 **/ 

    struct fb_var_screeninfo 
                        var_info;

    struct fb_fix_screeninfo 
                        fix_info;

};

/**
 * Using O(n) because this is
 * infrequently used.
 **/
static struct {
    VIDEO       video_list;
    size_t      count;
    size_t      used;
    PVMUTEX     vmutex;
}                       video_monitor;

/**
 * \return ZERO on success.
 **/ 
static int video_lock( PVMUTEX pvm ) {
    int pid = getpid(),
        rv;

    if (pvm->owner == pid) {
        pvm->lock_count++;
        return 0;
    }
    rv = pthread_mutex_lock(&pvm->mutex);
    pvm->owner = pid;
    return rv;
}

/**
 * \return ZERO on success.  Same as pthread_mutex_unlock(3)
 * only it returns EAGAIN if the mutex is locked by the calling
 * thread, but has been locked recursively.  The owning thread 
 * _can_ call video_unlock_list() until this function returns
 * zero if it wants to...
 **/ 
static int video_unlock( PVMUTEX pvm ) {
    int pid = getpid();

    if ( pvm->owner != pid) return EPERM;
    if (--pvm->lock_count == 0) {
        pvm->owner = 0;
        return pthread_mutex_unlock(&pvm->mutex);
    }
    return EAGAIN;
}

static PVMUTEX video_mutex_create() {
    PVMUTEX m = (PVMUTEX)calloc(1, sizeof(VMUTEX));
    if (m) {
        pthread_mutex_init(&m->mutex, 0);
    }
    return m;
}

static void video_mutex_destroy( PVMUTEX *ppvm ) {
    PVMUTEX pvm = *ppvm;
    pthread_mutex_destroy(&pvm->mutex);
    free(pvm);
    *ppvm = 0;
    return;
}

/**
 * Yes, we're locking a mutex in a signal handler.
 **/ 
void vtsig( int signum ) {
    int     i;
    pid_t   pid;

    if (signum == SIGHUP) {
        pid = getpid();
        video_lock(video_monitor.vmutex);
        for (i = 0; i < video_monitor.count; i++ ) {
            if (video_monitor.video_list[i].pid == pid) {
                video_monitor.video_list[i].active = 0;
                break;
            }
        }
        video_unlock(video_monitor.vmutex);
    }
}

void video_stop( VIDEO v ) {
    size_t  i = 0;

    if (v->fbid <= 0 || !v->active) return;

    video_lock(video_monitor.vmutex);
    for (; i < video_monitor.count; i++) {
        if (v->active == 1 && 
            v->fbid == video_monitor.video_list[i].fbid)
        {
            goto vsid_found;
        }
    }
    video_unlock(video_monitor.vmutex);
    return;

    vsid_found:

    free(v->clrb.ptr);

    /**
     * Shut down rendering/timing thread.
     **/ 
    v->active = 0;

    /** 
     * Rendering thread for this display is down.  
     * clear everything up.
     * 
     **/ 
    munmap(v->ptr.ptr, v->fix_info.smem_len);

    close(v->fbid);

	tcsetattr(STDIN_FILENO, TCSANOW, &v->term_prev);

    video_mutex_destroy(&v->mtx_prerender);

    /* Clear the structure */
    memset(v, 0, sizeof(struct video_setup));

    video_monitor.used--;

    video_unlock(video_monitor.vmutex);
}

VIDEO video_start( int nframebuffer ) {
    int     i;
    VIDEO   swap,
            v;
    char    fb_file_path[50];   /* <--- Any changes, pay attention to this.  Only _50_ */

    if (nframebuffer < 0 || nframebuffer > 100) {
        /**
         * Sorry, I'm not writing this for over
         * 100 screens or for if you don't have
         * a screen but intend to buy one later...
         **/ 
        errno = EINVAL;
        return 0;
    }

    sprintf(fb_file_path, FB_FS_LOCATION, nframebuffer);

    video_lock(video_monitor.vmutex);
    if (video_monitor.used == video_monitor.count) {
        swap = (VIDEO) realloc(video_monitor.video_list,
                                sizeof(struct video_setup) * (video_monitor.count + 10));
        if (!swap) {
            errno = ENOMEM;
            video_unlock(video_monitor.vmutex);
            return 0;
        }
        memset(&swap[video_monitor.count], 0, sizeof(struct video_setup) * 10 );
        video_monitor.video_list = swap;
        video_monitor.count+=10;
        v = &video_monitor.video_list[video_monitor.used];
    } else {
        v = 0;
        for (i = 0; i < video_monitor.count; i++) {
            if (video_monitor.video_list[i].pid == 0) {
                v = &video_monitor.video_list[i];
                break;
            }
        }
        if (v == 0) {
            video_unlock(video_monitor.vmutex);
            return 0;
        }
    }

    v->fbid = open(fb_file_path, O_RDWR);

    if (v->fbid < 0) {
        goto vs_fail;
    }

    if (ioctl(v->fbid, FBIOGET_FSCREENINFO, &v->fix_info)) goto vs_fail;

    if (ioctl(v->fbid, FBIOGET_VSCREENINFO, &v->var_info)) goto vs_fail;

	if (tcgetattr(STDIN_FILENO, &v->term_prev) != 0) goto vs_fail;

    v->term_curr = v->term_prev;

	v->term_curr.c_lflag &= (~ICANON & ~ECHO);

	if (tcsetattr(STDIN_FILENO, TCSANOW, &v->term_curr) != 0) goto vs_fail;

    v->width  = v->var_info.xres_virtual;
    v->height = v->var_info.yres_virtual;
    v->px_count = v->width*v->height;

    if (v->px_count % 2 == 0) v->px_count64 = v->px_count/2;

    if ( ((int)(v->ptr.ptr = 
                mmap(0, 
                    v->fix_info.smem_len, 
                    PROT_READ | PROT_WRITE, MAP_SHARED, 
                    v->fbid, 0))) == -1 ) 
    {
        goto vs_fail;
    }
    v->pid = getpid();
    video_monitor.used++;

    if ( !(v->mtx_prerender = video_mutex_create()) ) goto vs_fail_rstty;

    v->clrb.ptr = video_get_empty_buffer(v);

    v->active = 1;

    goto vsdone;                /* Success, jump past error crap and be done. */
    
    /*------------------------ Error handling --------------------------------*/

    vs_fail_rstty:
	tcsetattr(STDIN_FILENO, TCSANOW, &v->term_prev);

    vs_fail:
    memset(v, 0, sizeof(struct video_setup));
    v = 0;      /* The last statement of our error handling sections. */

    vsdone:
    video_unlock(video_monitor.vmutex);
    return v;
}

int video_get_width( VIDEO v ) {
    return v->var_info.xres_virtual;
}

int video_get_height( VIDEO v ) {
    return v->var_info.yres_virtual;
}

/**
 * This function is called when a buffer has pixel
 * color data i.e. has been drawn on by an application,
 * and is ready to be rendered to the monitor.
 * 
 * It copies the given data to the video system's
 * "pre-render" buffer and signals the video system
 * that it has fresh data to render.
 * 
 **/ 
void video_submit_frame( VIDEO v, void *buf_pixels ) {
    int                 i = 0;
    union px_pointer    src;
    
    if (!v->active) {
        fprintf(stderr, "libvideo/video_submit_frame(): ERROR - Video not active\n");
        return;
    }

    src.ptr = buf_pixels;

    video_lock(v->mtx_prerender);

    if (ioctl(v->fbid, FBIO_WAITFORVSYNC, &v->var_info) != 0) {
        fprintf(stderr, "libvideo/video_submit_frame(): ERROR - FBIO_WAITFORVSYNC failed.\n");
        video_unlock(v->mtx_prerender);
        return;
    }

    if (v->px_count64) {
        for (i = 0; i < v->px_count64; i++)
            v->ptr.ptr64[i] = src.ptr64[i];
    } else {
        for (i = 0; i < v->px_count; i++)
            v->ptr.ptr32[i] = src.ptr32[i];
    }
    video_unlock(v->mtx_prerender);
}

size_t video_get_req_buffer_size( VIDEO v ) {
    return v->fix_info.smem_len;
}

size_t video_get_stride_pitch( VIDEO v ) {
    return v->fix_info.line_length;
}

void *video_get_empty_buffer( VIDEO v ) {
    return calloc(1, v->fix_info.smem_len);
}

void video_set_screen_color( VIDEO v, uint32_t color) {
    int         i = 0;
    uint64_t    c;
    if (v->px_count64 > 0) {
        c = ((((uint64_t)color) << 32) | color);
        for (; i < v->px_count64; i++)
            v->clrb.ptr64[i] = c;
    } else {
        for (; i < v->px_count; i++)
            v->clrb.ptr32[i] = color;
    }
    video_submit_frame(v, v->clrb.ptr);
}

void video_screen_white( VIDEO v ) {
    video_set_screen_color(v, 0xFFFFFFFF);
}

void video_clear_screen( VIDEO v ) {
    int i = 0;
    if (v->px_count64 > 0) {
        for (; i < v->px_count64; i++)
            v->clrb.ptr64[i] = 0;
    } else {
        for (; i < v->px_count; i++)
            v->clrb.ptr32[i] = 0;
    }
    video_submit_frame(v, v->clrb.ptr);
}

int video_is_active( VIDEO v ) {
    if (!v) return 0;
    if (v->active == 1) return 1;
    return 0;
}

size_t video_get_pixel_count( VIDEO v ) {
    if (v && video_is_active(v)) {
        return v->px_count;
    }
    return 0;
}

int video_get_fb_fix_screeninfo( VIDEO v, void *pdest, size_t buf_len ) {
    const size_t len = sizeof(struct fb_fix_screeninfo);
    if (!pdest || !v->active) return -1;
    if (buf_len < len) return len;
    memcpy(pdest, &v->fix_info, sizeof(struct fb_fix_screeninfo));
    return 0;
}

int video_get_fb_var_screeninfo( VIDEO v, void *pdest, size_t buf_len ) {
    const size_t len = sizeof(struct fb_fix_screeninfo);
    if (!pdest || !v->active) return -1;
    if (buf_len < len) return len;
    memcpy(pdest, &v->var_info, len);
    return 0;
}


int video_get_current_pixel_data( VIDEO v, void *pdest, size_t buf_len ) {
    union px_pointer    d;
    int                 i;
    
    if (!pdest || !v->active) return -1;

    if (buf_len < v->fix_info.smem_len) return v->fix_info.smem_len;

    d.ptr = pdest;

    if (v->px_count64) {
        for (i = 0; i < v->px_count64; i++)
            d.ptr64[i] = v->ptr.ptr64[i];
    } else {
        for (i = 0; i < v->px_count; i++)
            d.ptr32[i] = v->ptr.ptr32[i];
    }
    return 0;
}


/*
 +================================================================================+
 |                        Library load / unload routines                          |
 +================================================================================+
*/ 


void __attribute__((constructor)) initLibrary(void) {
    video_monitor.video_list    = (VIDEO)calloc(10, sizeof(struct video_setup));
    video_monitor.used          = 0;
    if (!video_monitor.video_list) {
        video_monitor.count         = 0;
        return;
    }
    video_monitor.count         = 10;
    video_monitor.vmutex        = video_mutex_create();
}

void __attribute__((destructor)) cleanUpLibrary(void) {
    int i = 0;
    video_lock(video_monitor.vmutex);
    for (; i < video_monitor.count; i++) {
        if (video_monitor.video_list[i].active) {
            video_stop(&video_monitor.video_list[i]);
        }
    }
    video_unlock(video_monitor.vmutex);
    video_mutex_destroy(&video_monitor.vmutex);
}

