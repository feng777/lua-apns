# lua-apns
Lua binding for libcapns . Now it is possible to work with Apple Push Notification Service for your Lua / LuaJIT server

First of all, install **libcapn** library https://github.com/adobkin/libcapn

Then build `apns.c` something like this:

`gcc -shared -fpic -O -I. -I/path/to/luajitheaders -L. -L/path/to/luajithlib -L/usr/lib/capn -lcapn apns.c -o apns.so `

then read and execute the sample: `luajit example_apns.lua`
DO NOT FORGET TO CREATE p12 FILE and USE CORRECT TOKENS.
