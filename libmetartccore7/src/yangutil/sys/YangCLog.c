//
// Copyright (c) 2019-2025 yanggaofeng
//

#include <stdarg.h>
#include <assert.h>
#include <time.h>
#include <yangutil/sys/YangLog.h>

#if Yang_OS_ANDROID || Yang_OS_IOS
#define Yang_Enable_Logfile 0
#else
#define Yang_Enable_Logfile 1
#endif

#if Yang_OS_WIN
#include <io.h>
#ifdef _MSC_VER
#include <direct.h>
#endif
#else
#include <sys/time.h>
#endif

#define Yang_Log_Cachesize 1024 * 12
#define Yang_Log_Cachesize2 Yang_Log_Cachesize + 256

static int32_t g_hasLogFile = 0;
static int32_t g_logLevel = YANG_LOG_ERROR;

#if Yang_Enable_Logfile
static FILE* g_fmsg = NULL;
#endif

static char const* YANG_LOG_LEVEL_NAME[] = { 
	"FATAL", 
	"ERROR", 
	"WARNING",
	"INFO", 
	"DEBUG", 
	"TRACE" 
};

void yang_setCLogFile(int32_t isSetLogFile, char* logPath) {
#if Yang_Enable_Logfile
	char file_path_getcwd[255];

	if (g_hasLogFile) {
        return;
	}

    g_hasLogFile = isSetLogFile;

	if (g_hasLogFile && g_fmsg == NULL) {
		char file[300];
		yang_memset(file, 0, 300);

		if (logPath == NULL) {
			char file_path_getcwd[255];
			yang_memset(file_path_getcwd, 0, 255);

#if Yang_OS_WIN
			if (_getcwd(file_path_getcwd, 255)) {
                yang_sprintf(file, "%s/yang_log.log", file_path_getcwd);   
			}
#else
			if (getcwd(file_path_getcwd, 255)) {
				yang_sprintf(file, "%s/yang_log.log", file_path_getcwd);
			}
#endif
		}
		else {
			yang_sprintf(file, "%s/yang_log.log", logPath);
		}
		 
	    if (g_fmsg == NULL) {
	    	g_fmsg = fopen(file, "wb+");
	    }
	}
#endif
}

#if Yang_Enable_Logfile

static void yang_writeFile(int32_t level, char* buf) {
	int32_t sfLen;

	char logStr[Yang_Log_Cachesize2];
	yang_memset(logStr, 0, Yang_Log_Cachesize2);

	sfLen = yang_sprintf(logStr, "%s", buf);

	if (g_fmsg) {
		fwrite(logStr, sfLen, 1, g_fmsg);
		fflush(g_fmsg);
	}
}

#endif

void yang_closeCLogFile() {
#if Yang_Enable_Logfile
	if (g_fmsg) {
        fclose(g_fmsg);
		g_fmsg = NULL;
	}

	g_hasLogFile = yangfalse;
#endif
}

void yang_clog(int32_t level, const char* fmt, ...) {
	if (level > g_logLevel) {
        return;
	}
    
	char buf[4096];
	yang_memset(buf, 0, 4096);
    
	va_list args;

	va_start(args, fmt);
	yang_vsnprintf(buf, 4095, fmt, args);
	va_end(args);

    time_t t_now = time(NULL);
    struct tm* ntm = localtime(&t_now);

    yang_printf(
		"[%02d:%02d:%02d] [%s] %s\n",
		ntm->tm_hour,
		ntm->tm_min,
		ntm->tm_sec,
		YANG_LOG_LEVEL_NAME[level], 
		buf
	);

#if Yang_Enable_Logfile
	if (g_hasLogFile) {
		yang_writeFile(level, buf);
	}
#endif
}

int32_t yang_error_wrap(int32_t errcode, const char* fmt, ...) {
	if (YANG_LOG_ERROR > g_logLevel) {
        return errcode;
	}

	char buf[4096];
	yang_memset(buf, 0, 4096);
    
	va_list args;

	va_start(args, fmt);
	yang_vsnprintf(buf, 4095, fmt, args);
	va_end(args);

	time_t t_now = time(NULL);
    struct tm* ntm = localtime(&t_now);

    yang_printf(
		"[%02d:%02d:%02d] [%s][%d]: %s\n",
		ntm->tm_hour,
		ntm->tm_min,
		ntm->tm_sec,
		YANG_LOG_LEVEL_NAME[YANG_LOG_ERROR], 
		errcode,
		buf
	);

#if Yang_Enable_Logfile
	if (g_hasLogFile) {
		yang_writeFile(YANG_LOG_ERROR, buf);
	}
#endif

	return errcode;
}

void yang_setCLogLevel(int32_t level) {
	g_logLevel = level;

	if (g_logLevel > YANG_LOG_TRACE) {
		g_logLevel = YANG_LOG_TRACE;
	}
}

