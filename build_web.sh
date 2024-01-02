emcc -o webout/game.html main.c -Wall ./libraylib.a -I. -s USE_GLFW=3 -s ASYNCIFY -s ASSERTIONS -s ALLOW_MEMORY_GROWTH -I/usr/include/ --preload-file resources/
