LIBNAME=/usr/local/lib/libvideo.so

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

.PHONY: test

test:
@echo "\033[0;36m"
@echo "Making video test."
@echo "------------------\033[0m";
@if [ -f $(LIBNAME) ]; then \
gcc test/video_test.c -o vidtest -lvideo;\
echo "    \033[1;32mSuccess!\033[0m";\
echo "";\
echo "Test executable has been built in this directory";\
echo "and is named: vidtest";\
echo "";\
else \
echo "\033[0;31mERROR: Video Render Libs Not Installed\033[0m"; \
echo ""; \
echo "Please run: "; \
echo "     \044 make"; \
echo "     \044 sudo make install"; \
echo ""; \
fi;
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
@ln -s /usr/local/include/video.h /usr/include/video.h
@echo "";
@echo "    \033[1;32mSuccess!\033[0m"
@echo "";
@echo "Installed:"
@echo "     Header: /usr/local/include/video.h"
@echo "     Static: /usr/local/lib/libvideo.a"
@echo "    Dynamic: /usr/local/lib/libvideo.so"
@echo "";
@echo "Symlinks created to /usr/lib and /usr/include also"
@echo "";
@echo "Link to this library in your code by"
@echo "including <video.h> and use this linker"
@echo "flag: -lvideo"
@echo "";

remove:
@echo "Uninstalling video rendering library."
@rm -f /usr/lib/libvideo.so
@rm -f /usr/lib/libvideo.a
@rm -f /usr/include/video.h
@rm -f /usr/local/lib/libvideo.so
@rm -f /usr/local/lib/libvideo.a
@rm -f /usr/local/include/video.h
@echo "";
@echo "    \033[1;32mSuccess!\033[0m"
@echo "Done."
@echo "";
