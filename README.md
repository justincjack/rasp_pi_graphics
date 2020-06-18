# Raspberry Pi Graphics
Simple VBLANK synchronized Raspberry Pi 4 Framebuffer Rendering
Simple and highly functional graphics rendering foundation for Raspberry Pi 4B.  It *may* work on other Raspberry Pis.  I haven't tested it on any others since I'm using the 4B for the projects I'm working on.  I suspect it will work just fine.

### **To install:**
Download or clone this repo.

You will need gcc to compile this.
```bash
make
sudo make install
```
and then, optionally (to view the library in action)

```bash
make test
```

That's it.

You can run the test program (called "vidtest") with
```bash
$ ./vidtest
```

# Usage
Using the library.
You initialize the library by calling: **video_start( int nframebuffer )**.  If you're unsure which "framebuffer" to use or don't know which are available, type: 
```bash
ls /dev/fb*
```
To obtain a buffer to draw in, call **video_get_empty_buffer( VIDEO v )**.

To display your buffer on the next scanout, call **video_submit_frame( VIDEO v, void *buf_pixels )**.

To get a copy of what's currently displayed on the screen, call **video_get_current_pixel_data( VIDEO v, void *pdest, size_t buf_len )**

### ***_Important_***
When you're done using the library, don't forget to call **video_stop( VIDEO v )** to restore the way the terminal works correctly.

### Notes:
This can be used with other graphics libraries such as Cairo or Pango

### **More Info**
There's just not a lot of good information in one place about how to render your own graphics smoothly using a Raspberry Pi.  Before this, I had tried libdri and build my own kms buffer implementation based on info that I found online.  I used multiple buffers and "page flip" events.  It worked _okay_, but I thought things could go smoother.

This simple library encompasses everything you'll need to get started and render your graphics smoothly.  It's a foundation only.  I suspect a lot of people using this library will want to implement double-buffering.  It's as simple as creating a main "pre-render buffer" with a call to **video_get_empty_buffer()**.  Do all your drawing on that and call **video_submit_frame()**.  You can stack buffers, blit them together in order...however you want.  

### **Things to watch out for and think about**
The screen only refreshes so fast.  When you call **video_submit_frame()**, _at that point_ the execution of your program will wait until the display is ready to scan out the buffer you've passed it.  So in theory, if your display refreshes at 60Hz, and your code is running in a loop wherein it's submitting a buffer every loop iteration, **_your code will only execute at 60Hz_**.  It may be a good idea to launch a separate thread to receive buffers - signal it with a semaphore or send it a signal.  

If you do this, have a plan for what should happen if you get multiple submissions between scanouts.  Also, if this thread we're talking about has queued the buffer you've given it and your code elsewhere is blting to the same buffer on scanout **_you could end up with screen tearing_** and negate the efficacy of the library.

But good luck on your project and I hope this helps!







