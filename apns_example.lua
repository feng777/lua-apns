--do not forget to build apns.c
require("apns")

local debug_ = false
local apnsdebstate = debug_ and 0 or 1
local p12path = debug_ and "/path/to/key/debug_cert.p12" or "/path/to/key/release_cert.p12"
local p12pwd = debug_ and "debug_yourpassword" or "release_yourpassword"

local function logcallback (type_of_log, msg, index)
	if (type_of_log == 0) then
		print(string.format("======> %s", msg))
	elseif (type_of_log == 1) then
		print(string.format("======> Invalid token: %s (index: %d)", msg, index)) -- msg is token here
	else
		print(string.format("======> %s >>> %s >>> %s ", tostring(type_of_log), msg, tostring(index)))
	end		
end

--use this method one one time inside the code
apnsopen(apnsdebstate, p12path, p12pwd, logcallback)

local NUMBADGES = 1
local AWAITINGTIME = 3600 * 24 -- 24 hours
local txt = "EXAMPLE"

--you can loop this method for message ad infinitum
--for i = 1, 10 do
	apns(NUMBADGES, AWAITINGTIME, txt, {"token1","token2","token3"})
--end

--use this method only if you will not use apns inside this script in future
apnsclose()
	
