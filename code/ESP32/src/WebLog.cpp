#include "WebLog.h"

int weblog_log_printfv(const char *format, va_list arg)
{
    static char loc_buf[255];
    uint32_t len;
    va_list copy;
    va_copy(copy, arg);
    len = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if(len >= sizeof(loc_buf)){
        return 0;
    }
    vsnprintf(loc_buf, len+1, format, arg);
	webSocket.broadcastTXT(loc_buf);
    int rVal = ets_printf("%s", loc_buf);
    return rVal;
}

int weblog(const char *format, ...)
{
	int len;
    va_list arg;
    va_start(arg, format);
    len = weblog_log_printfv(format, arg);
    va_end(arg);
    return len;
}