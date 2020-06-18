# rasp_pi_graphics
Simple VBLANK synchronized Raspberry Pi 4 Framebuffer Rendering
Simple and highly functional graphics rendering foundation for Raspberry Pi 4B.  It *may* work on other Raspberry Pis.  I haven't tested it on any others since I'm using the 4B for the projects I'm working on.  I suspect it will work just fine.

To use:
Download or clone this repo.

You will need gcc to compile this.
```bash
make
sudo make install
```

That's it.

# usage
Using the library.
You initialize the library by calling: **video_start( int nframebuffer )**.  If you're unsure which "framebuffer" to use or don't know which are available, type: 
```bash
ls /dev/fb*
```
To obtain a buffer to draw in, call ***video_get_empty_buffer( VIDEO v )***.

To display your buffer on the next scanout, call ***video_submit_frame( VIDEO v, void *buf_pixels )***.

To get a copy of what's currently displayed on the screen, call ***video_get_current_pixel_data( VIDEO v, void *pdest, size_t buf_len )***

###***_Important_***
When you're done using the library, don't forget to call ***video_stop( VIDEO v )*** to restore the way the terminal works correctly.

###Notes:
This can be used with other graphics libraries such as Cairo or Pango



