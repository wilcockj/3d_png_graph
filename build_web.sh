emcc -o game.html main.c -g3 -Wall ./libraylib.a -I. -s USE_GLFW=3 -s ASYNCIFY -s ASSERTIONS -s ALLOW_MEMORY_GROWTH -I/usr/include/ --preload-file resources/
