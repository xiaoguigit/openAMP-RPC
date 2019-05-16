
#define LOG_TAG    "logger.c"

#include <unistd.h>
#include <stdlib.h>
#include "../inc/elog.h"
#include <sys/stat.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#define REDEF_O_CREAT   0000100
#define REDEF_O_EXCL    0000200
#define REDEF_O_RDONLY  0000000
#define REDEF_O_WRONLY  0000001
#define REDEF_O_RDWR    0000002
#define REDEF_O_APPEND  0002000
#define REDEF_O_ACCMODE 0000003
static int fd;
static int logger_lock = 0;


/* logger file name */
static char fname[] = "/home/rpu1.log";

/**
 * EasyLogger port initialize
 *
 * @return result
 */
ElogErrCode elog_port_init(void) {
    ElogErrCode result = ELOG_NO_ERR;

    /* add your code here */
	fd = open(fname, REDEF_O_CREAT | REDEF_O_WRONLY | REDEF_O_APPEND, S_IRUSR | S_IWUSR);
	if(fd < 0){
		result = ELOG_FILE_ERR;
	}

    return result;
}

/**
 * EasyLogger port exit
 *
 * @return result
 */
ElogErrCode elog_port_exit(void) {
    ElogErrCode result = ELOG_NO_ERR;

    /* add your code here */
    close(fd);

    return result;
}


/**
 * output log port interface
 *
 * @param log output of log
 * @param size log size
 */
void elog_port_output(const char *log, size_t size) {
    /* add your code here */
	#if defined(ELOG_OUTPUT_TO_FILE)
		write(fd, log, size);
	#elif defined(ELOG_OUTPUT_CONSOLE)
		printf("%s.\n", log);
	#endif
}


/**
 * output lock
 */
void elog_port_output_lock(void) {

    /* add your code here */
	logger_lock = 1;
}

/**
 * output unlock
 */
void elog_port_output_unlock(void) {

    /* add your code here */
	logger_lock = 0;
}

/**
 * get current time interface
 *
 * @return current time
 */
const char *elog_port_get_time(void) {

    /* add your code here */
	static char tick[8];
	itoa(xTaskGetTickCount(), tick, 10);
	return tick;
}

/**
 * get current process name interface
 *
 * @return current process name
 */
const char *elog_port_get_p_info(void) {
    /* add your code here */
	return NULL;
}

/**
 * get current thread name interface
 *
 * @return current thread name
 */
const char *elog_port_get_t_info(void) {

    /* add your code here */
	return NULL;
}




