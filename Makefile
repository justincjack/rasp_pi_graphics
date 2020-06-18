default: vid

vid:
        @echo "\033[0;36m"
        @echo "Building video render libraries"
        @echo "-------------------------------"
        @echo "\033[0m"
        @echo "Compiling video.c for STATIC linkage..."
        @gcc -c -Wall -Werror \
        video.c \
        -o lib/video.o \
        -pthread
        @echo "    \033[1;32mSuccess!"
        @echo "\033[0m"
        @echo "Creating static library: libvideo.a";
        @ar rcs lib/libvideo.a lib/video.o
        @echo "    \033[1;32mSuccess!"
        @echo "\033[0m"
        @rm -rf lib/video.o
        @echo "    Compiling video.c for DYNAMIC linkage..."
        @gcc -c -fPIC -Wall -Werror \
        video.c \
        -o shared/video.o \
        -pthread
        @echo "    \033[1;32mSuccess!"
        @echo "\033[0m"
        @echo "Creating shared library: libvideo.so";
        @gcc -shared shared/video.o -o shared/libvideo.so
        @echo "    \033[1;32mSuccess!"
        @echo "\033[0m"
        @rm shared/video.o
        @echo "";
        @echo "\033[0;36m"
        @echo "Done!"
        @echo "\033[0m"
        @echo "";
        @echo "";


install:
        @echo "\033[0;36m"
        @echo "Installing libraries."
        @echo "---------------------\033[0m"
        @cp video.h /usr/local/include
        @cp lib/libvideo.a /usr/local/lib
        @cp shared/libvideo.so /usr/local/lib
        @ldconfig -n /usr/local/lib
        @ln -s /usr/local/lib/libvideo.so /usr/lib/libvideo.so
        @ln -s /usr/local/lib/libvideo.a /usr/lib/libvideo.a
        @ln -s /usr/local/include/video.h /usr/inclide/video.h
        @echo "";
        @echo "    \033[1;32mSuccess!\033[0m"
        @echo "";
        @echo "Installed:"
        @echo "     Header: /usr/local/include/video.h"
        @echo "     Static: /usr/local/lib/libvideo.a"
        @echo "    Dynamic: /usr/local/lib/libvideo.so"
        @echo "";
        @echo "Link to this library in your code by"
        @echo "including <video.h> and use this linker"
        @echo "flag: -lvideo"
        @echo "";
