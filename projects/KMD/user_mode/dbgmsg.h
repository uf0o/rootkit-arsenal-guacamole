#pragma once

#ifdef LOG_OFF
#define DBG_TRACE(src,msg)
#define DBG_PRINT1(arg1)
#define DBG_PRINT2(fmt,arg1)
#define DBG_PRINT3(fmt,arg1,arg2) 
#define DBG_PRINT4(fmt,argl,arg2,arg3)
#else
#define DBG_TRACE(src,msg)					printf("[%s] : %s\n", src, msg)
#define DBG_PRINT1(arg1)					printf("%s",arg1)
#define DBG_PRINT2(fmt,arg1)				printf(fmt,arg1)
#define DBG_PRINT3(fmt,arg1,arg2)			printf(fmt,arg1,arg2)
#define DBG_PRINT4(fmt,arg1,arg2,arg3)  	printf(fmt,arg1,arg2,arg3)
#endif