/* apns.c (C) 2016 by Vyacheslav Petrukhin
* At first, install http://libcapn.org/  or github project https://github.com/adobkin/libcapn
* gcc -shared -fpic -O -I/usr/include/lua5.1 -llua5.1 -L/usr/lib/capn -lcapn apns.c  -o apns.so
* for LuaJIT
* gcc -shared -fpic -O -I. -I/path/to/luajitheaders -L. -L/path/to/luajithlib -L/usr/lib/capn -lcapn apns.c -o apns.so 
*/

/*
libcapns
*/
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <capn/apn.h>

#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h> 
#include <lualib.h>

lua_State *L_; //pointer for callback calling
int ref_;//parameter of function for callback calling


static int collect_callback() {
   lua_unref(L_, ref_);
   L_ = NULL;
}
void __apn_logging(apn_log_levels level, const char * const message, uint32_t len) {

	lua_rawgeti(L_, LUA_REGISTRYINDEX, ref_);
	int nargs = 3;
	lua_pushnumber(L_, 0); 
	lua_pushstring(L_, message);
	lua_pushnumber(L_, 0);
	lua_pcall(L_, nargs,  0, 0);
	
}

void __apn_invalid_token(const char * const token, uint32_t index) {
	lua_rawgeti(L_, LUA_REGISTRYINDEX, ref_); 
	int nargs = 3;
	lua_pushnumber(L_, 1);
	lua_pushstring(L_, token);
	lua_pushnumber(L_, index);
	lua_pcall(L_, nargs, 0 , 0);
}
apn_ctx_t *ctx = NULL;
static int iapnsopen(lua_State *L) {
	
    assert(apn_library_init() == APN_SUCCESS);
	
	int ifrelease = (int)lua_tonumber(L,1);
	const char* p12path = lua_tostring(L,2);
	const char* p12pwd = lua_tostring(L,3);
	lua_pushvalue(L, 4);
	L_ = L;
	ref_ = luaL_ref(L, LUA_REGISTRYINDEX);
    if(NULL == (ctx = apn_init())) {
        apn_library_free();
		collect_callback();
		lua_pushnumber(L,1);
        return 1;
    }
	
	apn_set_pkcs12_file(ctx, p12path, p12pwd);
    apn_set_mode(ctx,  (ifrelease == 1)?APN_MODE_PRODUCTION:APN_MODE_SANDBOX); //APN_MODE_PRODUCTION or APN_MODE_SANDBOX
    apn_set_behavior(ctx, APN_OPTION_RECONNECT);
    apn_set_log_level(ctx, APN_LOG_LEVEL_INFO | APN_LOG_LEVEL_ERROR | APN_LOG_LEVEL_DEBUG);
    apn_set_log_callback(ctx, __apn_logging);
    apn_set_invalid_token_callback(ctx, __apn_invalid_token);
	
	lua_pushnumber(L,1);
    return 1;
}
static int iapnsclose(lua_State *L) {
	if(ctx == NULL) {
        apn_library_free();
		lua_pushnumber(L,2);
        return 1;
    }
	apn_free(ctx);
    apn_library_free();
	collect_callback();
	lua_pushnumber(L,1);
	return 1;
}
static int iapns(lua_State *L) {
	
	if (ctx == NULL) {
		lua_pushnumber(L,2);
        return 1;
	}
		
	apn_payload_t *payload = NULL;
    char *invalid_token = NULL;
	
	time_t time_now = 0;
    time(&time_now);

    if(NULL == (payload = apn_payload_init())) {

		lua_pushnumber(L,1); 
        return 1;
    }
	
	int badges_count = (int)lua_tonumber(L,1);
	int seconds = (int)lua_tonumber(L,2);
	size_t size = 1;
	const char* text = lua_tolstring(L,3,&size);
	
    apn_payload_set_badge(payload, badges_count); // Icon badge
    apn_payload_set_body(payload, text);  // Notification text
    apn_payload_set_expiry(payload, time_now + seconds); // Expires
    apn_payload_set_sound(payload, "default");
    apn_payload_set_priority(payload, APN_NOTIFICATION_PRIORITY_HIGH);  // Notification priority

    apn_array_t *tokens = apn_array_init(2, NULL, NULL);
    if(!tokens) {
        apn_payload_free(payload);
		lua_pushnumber(L,1);
        return 1;
    }
	int narg = 4;
	luaL_checktype(L, narg, LUA_TTABLE);
	int hash_len = lua_objlen(L, narg);
	int i = 0;
	
	for(i = 0; i < hash_len; i++) {
		lua_pushinteger(L, i+1);
        lua_gettable(L, -2);
        if(lua_isstring(L, -1)) {
			const char* hash = lua_tolstring(L, -1, &size);
			apn_array_insert(tokens,(char*) hash);
		} else {
			
		}
		lua_pop(L, 1);
	}
	
    if(APN_ERROR == apn_connect(ctx)) {
		lua_rawgeti(L_, LUA_REGISTRYINDEX, ref_); 
		int nargs = 3;
		lua_pushnumber(L_, 2);
		lua_pushstring(L_, apn_error_string(errno));
		lua_pushnumber(L_, errno);
		lua_pcall(L_, nargs, 0 , 0);
        apn_payload_free(payload);
        apn_array_free(tokens);
		lua_pushnumber(L,1);         
        return 1;
    }

    apn_array_t *invalid_tokens = NULL;
    int ret = 0;
	
    if (APN_ERROR == apn_send(ctx, payload, tokens, &invalid_tokens)) {
		lua_rawgeti(L_, LUA_REGISTRYINDEX, ref_);
		int nargs = 3;
		lua_pushnumber(L_, 3);
		lua_pushstring(L_, apn_error_string(errno));
		lua_pushnumber(L_, errno);
		lua_pcall(L_, nargs, 0 , 0);

        ret = -1;
    } else {
        if (invalid_tokens) {
			lua_rawgeti(L_, LUA_REGISTRYINDEX, ref_);
			int nargs = 3;
			lua_pushnumber(L_, 4);
			lua_pushstring(L_, apn_error_string(errno));
			lua_pushnumber(L_, 0);
			lua_pcall(L_, nargs, 0 , 0);
            uint32_t i = 0;
            for (; i < apn_array_count(invalid_tokens); i++) {
				lua_rawgeti(L_, LUA_REGISTRYINDEX, ref_); 
				int nargs = 3;
				lua_pushnumber(L_, 5);
				lua_pushstring(L_, apn_array_item_at_index(invalid_tokens, i));
				lua_pushnumber(L_, i);
				lua_pcall(L_, nargs, 0 , 0);
            }
            apn_array_free(invalid_tokens);
        }
    }

    
    apn_payload_free(payload);
    apn_array_free(tokens);
	
	lua_pushnumber(L,1);
	return 1;
}
// initialization. Have to work for all lua versions
int luaopen_apns(lua_State *L){
	srand(time(NULL));
	#if LUA_VERSION_NUM >= 502
		lua_pushcfunction(L, iapns);
		lua_setglobal(L, "apns");
		lua_pushcfunction(L, iapnsopen);
		lua_setglobal(L, "apnsopen");
		lua_pushcfunction(L, iapnsclose);
		lua_setglobal(L, "apnsclose");
	#else
		lua_register(L,"apns",iapns);
		lua_register(L,"apnsopen",iapnsopen);
		lua_register(L,"apnsclose",iapnsclose);
	#endif
	return 0;
}
