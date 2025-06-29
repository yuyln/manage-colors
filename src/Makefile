../thirdparty/stb_image_write.o: ../thirdparty/stb_image_write.h
	gcc -O3 -c -x c -DSTB_IMAGE_WRITE_IMPLEMENTATION -o ../thirdparty/stb_image_write.o ../thirdparty/stb_image_write.h -lm

../thirdparty/stb_image.o: ../thirdparty/stb_image.h
	gcc -O3 -c -x c -DSTB_IMAGE_IMPLEMENTATION -o ../thirdparty/stb_image.o ../thirdparty/stb_image.h -lm

color-sub: color-sub.c
	gcc -fsanitize=address -fsanitize=undefined -Wall -Wextra -pedantic -O0 -g3 -ggdb -o ../bin/color-sub color-sub.c

color-get: color-get.c ../thirdparty/stb_image_write.o ../thirdparty/stb_image.o
	gcc -I../thirdparty -fsanitize=address -fsanitize=undefined -Wall -Wextra -pedantic -O0 -g3 -ggdb -o ../bin/color-get color-get.c ../thirdparty/stb_image_write.o ../thirdparty/stb_image.o -lm

all: color-sub color-get
