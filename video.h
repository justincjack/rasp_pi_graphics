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
#ifndef _VIDEO_H_
#define _VIDEO_H_

/**
 * Library to prepare a screen for graphics rendering
 * and provide buffers that an application can draw on
 * to be rendered on that screen without tearing.
 * 
 * You can build on top of this, thread it or whatever
 * you want to tailor it to your specific needs.
 * 
 * Tested on Raspberry Pi 4B 4GB RAM running Raspian Lite -
 * Buster.
 * 
 * Quick Function list (See actual definitions below comments for more info):
 * 
 * VIDEO       video_start( int framebuffer );
 * void        video_stop( VIDEO v );
 * void        video_submit_frame( VIDEO v, void *buf_pixels );
 * int         video_get_width( VIDEO v );
 * int         video_get_height( VIDEO v );
 * int         video_get_bpp( VIDEO v );
 * size_t      video_get_req_buffer_size( VIDEO v );
 * void        *video_get_empty_buffer( VIDEO v );
 * int         video_get_current_pixel_data( VIDEO v, void *pdest, size_t buf_len );
 * void        video_clear_screen( VIDEO v );
 * void        video_screen_white( VIDEO v );
 * void        video_set_screen_color( VIDEO v, uint32_t color);
 * int         video_is_active( VIDEO v );
 * void        *video_get_raw_ptr( VIDEO v );
 * size_t      video_get_stride_pitch( VIDEO v );
 * size_t      video_get_pixel_count( VIDEO v );
 * int         video_get_fb_var_screeninfo( VIDEO v, void *pdest, size_t buf_len );
 * int         video_get_fb_fix_screeninfo( VIDEO v, void *pdest, size_t buf_len );
 * 
 **/ 

#include <stdio.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <linux/kd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>


typedef struct video_setup              *VIDEO;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Prepares a screen for drawing operations.
 * 
 * \param int framebuffer
 * The /dev/fbX device representing video
 * memory on which we'll be drawing.
 * 
 * \return VIDEO
 * A VIDEO handle to be used when performing 
 * operations on the initialized video output
 * device represented by param 1.  On error,
 * NULL is returned and errno is set.
 * 
 * EINVAL means a parameter was invalid.
 * 
 * 
 **/ 
VIDEO       video_start( int framebuffer );

/**
 * Shut down the video display.  After this
 * call, the VIDEO handle is no longer valid
 * and MUST NOT be used.
 * 
 **/ 
void        video_stop( VIDEO v );

/**
 * Submit a buffer containing pixel color data to
 * be displayed on the screen at the next scan/
 * refresh (VBLANK)
 * 
 * \param VIDEO v
 * A handle to the VIDEO display returned by a
 * call to "video_start()"
 * 
 * \param void *buf_pixels
 * A pointer to a buffer large enough to contain
 * the pixel color bits for the entire screen.  You
 * may obtain a properly-sized buffer for drawing by
 * calling video_get_empty_buffer().
 * 
 **/ 
void        video_submit_frame( VIDEO v, void *buf_pixels );

/**
 * \return The width of the screen in pixels
 **/ 
int         video_get_width( VIDEO v );

/**
 * \return The height of the screen in pixels
 **/ 
int         video_get_height( VIDEO v );

/**
 * \return The number of BITS per pixel.
 **/ 
int         video_get_bpp( VIDEO v );

/**
 * \return The size in bytes of a buffer required
 * to hold enough pixel color data to display on
 * the screen.
 **/ 
size_t      video_get_req_buffer_size( VIDEO v );

/**
 * Get an empty buffer for drawing on.  The
 * buffer returned by a call to this function
 * may be passed to video_submit_frame().
 * 
 * \return On success, a memory buffer sized
 * to represent all pixels comprising the video
 * device.  On failure, NULL/
 **/ 
void        *video_get_empty_buffer( VIDEO v );

/**
 * Colors the screen BLACK i.e. sets video
 * buffer to all zeros.
 * 
 **/ 
void        video_clear_screen( VIDEO v );

/**
 * Show a white screen.
 **/
void        video_screen_white( VIDEO v );

/**
 * Set the screen to a solid color given by "color"
 *
 * \param uint32_t color
 * An RGB color to which the screen will be set.
 **/
void        video_set_screen_color( VIDEO v, uint32_t color);
  
/**
 * \return ONE if the VIDEO handle is valid,
 * ZERO otherwise.
 **/ 
int         video_is_active( VIDEO v );

/**
 * Returns the raw pointer directly to
 * video memory.  This should NOT be the
 * go-to for drawing.  Do NOT free this
 * pointer, try to realloc() it, or perform
 * any other operations on the pointer except
 * for dereferencing it while the VIDEO handle
 * is active.
 **/ 
void        *video_get_raw_ptr( VIDEO v );

/**
 * Returns the stride (pitch) of the current
 * video mode.
 **/ 
size_t      video_get_stride_pitch( VIDEO v );

/**
 * \return size_t
 * The number of pixels contained by the display
 * in its current video mode.  The VIDEO handle
 * must be valid or ZERO will be returned.
 **/
size_t      video_get_pixel_count( VIDEO v );
    
/**
 * Get the underlying fb_var_screeninfo struct information.
 * 
 * \note This data should NOT BE CHANGED while a valid VIDEO
 * handle exists for the display in question.  If your plan
 * is to change the screen mode with an ioctl() call, first
 * call video_stop().  Do your dirty work, then call
 * video_start() again.
 * 
 * \return int
 * On success, this function returns ZERO.  If a NEGATIVE number
 * is returned an error occured e.g. a NULL pointer was passed, 
 * the VIDEO object wasn't initialized.  If a POSITIVE number
 * is returned, it means the buffer given was too small and
 * you must have at least as many bytes allocated as is returned
 * by the function.
 * 
 **/ 
int         video_get_fb_var_screeninfo( VIDEO v, void *pdest, size_t buf_len );


/**
 * Get the underlying fb_fix_screeninfo struct information.
 * 
 * \return int
 * On success, this function returns ZERO.  If a NEGATIVE number
 * is returned an error occured e.g. a NULL pointer was passed, 
 * the VIDEO object wasn't initialized.  If a POSITIVE number
 * is returned, it means the buffer given was too small and
 * you must have at least as many bytes allocated as is returned
 * by the function.
 * 
 **/ 
int         video_get_fb_fix_screeninfo( VIDEO v, void *pdest, size_t buf_len );


/**
 * Copies the current pixel/color bits displayed on the 
 * display into the buffer "pdest"
 * 
 * \return int
 * On success, this function returns ZERO.  If a NEGATIVE number
 * is returned an error occured e.g. a NULL pointer was passed, 
 * the VIDEO object wasn't initialized.  If a POSITIVE number
 * is returned, it means the buffer given was too small and
 * you must have at least as many bytes allocated as is returned
 * by the function.
 * 
 **/ 
int         video_get_current_pixel_data( VIDEO v, void *pdest, size_t buf_len );
  
  
#ifdef __cplusplus
}
#endif


#endif
