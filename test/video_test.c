#include <video.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <signal.h>

/**
 * Press CTRL-C to exit program
 * 
 * gcc video_test.c -o vidtest -lvideo
 * 
 **/ 

#define WHITE                                   0xFFFFFFFF
#define BLACK                                   0xFF000000
#define YELLOW                                  0xFFFFFF00
#define BLUE                                    0xFF0000FF
#define GREEN                                   0xFF00FF00
#define RED                                     0xFFFF0000
#define PURPLE                                  0xFFA32999

#define BOX_WIDTH                               150
#define BOX_HEIGHT                              150
#define BOX_PIXEL_COUNT                         (BOX_WIDTH*BOX_HEIGHT)

#define BOX1_COLOR                              RED
#define BOX2_COLOR                              GREEN

#define BOX_STEP                                2

/** Alpha is 0xFF **/
#define RGB( red, green, blue )                 ((0xFF000000 | ((uint8_t)red << 16) | ((uint8_t)green << 8)) | (uint8_t)blue) 

#define RGBA( red, green, blue, alpha )         (((alpha << 24) | ((uint8_t)red << 16) | ((uint8_t)green << 8)) | (uint8_t)blue) 

#define BLEND( dst, src )                       blend_colors((PURGBA)dst, (PURGBA)src)

#define SET_ALPHA( alpha, color )               ((((uint8_t)alpha) << 24) | (color & 0x00FFFFFF));

typedef struct _u_rgba_ {
    union {
        uint32_t        color;
        struct {
            uint8_t     b;
            uint8_t     g;
            uint8_t     r;
            uint8_t     a;
        }__attribute__((packed)) vals;
    };
}__attribute__((packed))                        URGBA, *PURGBA;

volatile int program_running = 1;


/*+=====================================================================================+
  |                       Quick little color blending func                              |
  +=====================================================================================+*/


static inline void blend_colors( PURGBA dst, PURGBA src ) {
    int     ia  = 0;
    int     a   = 0;

    if (src->vals.a == 0) return;     /* It's invisible */
    if (src->vals.a == 0xFF) {        /* It's a solid color, just replace it. */
        dst->color = src->color;
        return;
    }
    a = dst->vals.a + src->vals.a;
    ia = (255 - src->vals.a);
    dst->vals.b = ((src->vals.a * src->vals.b + ia * dst->vals.b) >> 8);
    dst->vals.g = ((src->vals.a * src->vals.g + ia * dst->vals.g) >> 8);
    dst->vals.r = ((src->vals.a * src->vals.r + ia * dst->vals.r) >> 8);
    dst->vals.a = ((a > 0xFF)?0xFF:(uint8_t)a);
}

/*=======================================================================================*/


void sighand(int sig ) {
    if (sig == SIGINT)
        program_running = 0;
}

struct box {
    int         x,
                y,
                width,
                height,
                xdir,
                ydir,
                alpha_dir;

    uint8_t     alpha;

    uint32_t    color;

};

int main( int argc, char **argv) {
    int         i,
                nframebuffer            = 0,
                x,
                y,
                screen_width,
                screen_height,
                screen_px_count,
                screen_stride;
    void        *original_screen_image;
    uint32_t    *dst,
                *prerender_buffer,
                color;
    struct box  boxes[3];
    int         box_count               = 3,
                nsteps                  = 0;
    VIDEO       v;


    signal(SIGINT, &sighand);

    if (argc > 1) {
        nframebuffer = atoi(argv[1]);
    }
    
    if (argc > 2) {
        nsteps = atoi(argv[2]);
    }

    v = video_start(nframebuffer);

    if (!v) {
        fprintf(stderr, "\nERROR: video_start() failed: %s\n", strerror(errno));
        return 1;
    }


    /*+=============================================================================+
      |                         Get our screen dimensions                           |
      +=============================================================================+*/

    screen_width = video_get_width(v);

    screen_height = video_get_height(v);

    screen_px_count = video_get_pixel_count(v);

    screen_stride = video_get_stride_pitch(v);




    /* ------------------ Define our boxes ------------------ */

    /* Top-Left */
    boxes[0].alpha      = 0xFF;
    boxes[0].alpha_dir  = -1;
    boxes[0].color      = RED;
    boxes[0].width      = BOX_WIDTH;
    boxes[0].height     = BOX_HEIGHT;
    boxes[0].x          = 0;
    boxes[0].y          = 0;
    boxes[0].xdir       = 2+nsteps;
    boxes[0].ydir       = 2+nsteps;

    /* Center */
    boxes[1].alpha      = 0x00;
    boxes[1].alpha_dir  = 1;
    boxes[1].color      = GREEN;
    boxes[1].width      = BOX_WIDTH;
    boxes[1].height     = BOX_HEIGHT;
    boxes[1].x          = ((screen_width/2) - (boxes[1].width/2));
    boxes[1].y          = 0;
    boxes[1].xdir       = 0;
    boxes[1].ydir       = 3+nsteps;

    /* Top-Right */
    boxes[2].alpha      = 0x7F;
    boxes[2].alpha_dir  = 1;
    boxes[2].color      = BLUE;
    boxes[2].width      = BOX_WIDTH;
    boxes[2].height     = BOX_HEIGHT;
    boxes[2].x          = ((screen_width - boxes[2].width) - 1);
    boxes[2].y          = ((screen_height - boxes[2].height) - 1);
    boxes[2].xdir       = -(2+nsteps);
    boxes[2].ydir       = 2+nsteps;

    /** Allocate a buffer to hold what's on the screen now. **/ 
    original_screen_image = video_get_empty_buffer(v);

    /** 
     * Allocate our pre-render buffer for drawing operations
     * that we'll pass to "video_submit_frame()"
     **/ 
    prerender_buffer      = video_get_empty_buffer(v);

    /** Get a copy of what's on the screen now. **/ 
    if (video_get_current_pixel_data(v, original_screen_image, video_get_req_buffer_size(v))) {
        video_stop(v);
        fprintf(stderr, "ERROR: video_get_current_pixel_data() failed.\n");
        return 1;
    }

    video_clear_screen(v);

    /* Alternante red and blue a few times */
    for (i = 0; i < 12; i++) {
        if (i%2 == 0) {
            video_set_screen_color(v, RED);
        } else {
            video_set_screen_color(v, BLUE);
        }
        usleep(250000);
    }


    /** Quick check of user values (not that I don't trust you)....  **/
    for (i = 0; i < box_count; i++) {
        if ((boxes[i].alpha == 0xFF && 
            boxes[i].alpha_dir > 0) ||
            (boxes[i].alpha == 0 && 
            boxes[i].alpha_dir < 0)) 
        {
            boxes[i].alpha_dir = -boxes[i].alpha_dir;
            printf("NOTICE: Corrected alpha \"direction\" value for element %d to %d\n", i, boxes[i].alpha_dir);
        }
    }

    while (program_running) {

        /** Clear out our pre-render buffer **/
        for (i = 0; i < screen_px_count; i++) {
            prerender_buffer[i] = BLACK;
        }

        for (i = 0; i < box_count; i++) {
            color = SET_ALPHA(boxes[i].alpha, boxes[i].color);

            dst = &prerender_buffer[(boxes[i].y*screen_width) + boxes[i].x ];

            for (y = 0; y < boxes[i].height; y++) {
                for (x = 0; x < boxes[i].width; x++) {
                    if (dst < prerender_buffer || dst > &prerender_buffer[screen_px_count]) continue;
                    BLEND(dst++, &color);
                }
                dst+=(screen_width - boxes[i].width);
            }

            boxes[i].x+=boxes[i].xdir;
            boxes[i].y+=boxes[i].ydir;

            if ((boxes[i].x+boxes[i].width) >= screen_width) {
                boxes[i].x = screen_width-boxes[i].width-1;
                boxes[i].xdir = -boxes[i].xdir;
            } else if (boxes[i].x < 0) {
                boxes[i].x = 0;
                boxes[i].xdir = -boxes[i].xdir;
            }

            if ((boxes[i].y+boxes[i].height) >= screen_height) {
                boxes[i].y = screen_height-boxes[i].height-1;
                boxes[i].ydir = -boxes[i].ydir;
            } else if (boxes[i].y < 0) {
                boxes[i].y = 0;
                boxes[i].ydir = -boxes[i].ydir;
            }

            boxes[i].alpha+=boxes[i].alpha_dir;

            if (boxes[i].alpha == 0 || boxes[i].alpha == 0xFF) {
                boxes[i].alpha_dir = -boxes[i].alpha_dir;
            }
        }


        /* Queue the pre-render buffer for scanout */
        video_submit_frame(v, prerender_buffer);

    }

    /**
     * Restore the image that was on the screen.
     **/ 
    video_submit_frame(v, original_screen_image);

    video_stop(v);

    return 0;

}
