gcc -shared -fpic -O -I. -I/path/to/luajitheaders -L. -L/path/to/luajithlib -L/usr/lib/capn -lcapn apns.c -o apns.so
