/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * Copyright (C) 2015 Xilinx, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of Mentor Graphics Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**************************************************************************************
* This is a sample demonstration application that showcases usage of RPU unit
* from the remote core. This application is meant to run on the remote CPU running
* bare-metal. It can print to master console and perform file I/O for log
* using RPMSG mechanism.
*
* The log module uses the EasyLogger open source code and you can modify it if in need.
*
* Author	:	mr_xiaogui@163.com
* Date		:	2019/04/29
* Version	:	V1.0.00
* Copyright	:	ZHT Communication.
* Note		:
* 		1.Add the EasyLogger functions.
*
**************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "xil_printf.h"
#include "xil_exception.h"
#include "openamp/open_amp.h"
#include "openamp/rpmsg_retarget.h"
#include "rsc_table.h"
#include "platform_info.h"

#include "FreeRTOS.h"
#include "task.h"

#include "queue.h"
#include "timers.h"
#include "../logger/inc/elog.h"

#define REDEF_O_CREAT   0000100
#define REDEF_O_EXCL    0000200
#define REDEF_O_RDONLY  0000000
#define REDEF_O_WRONLY  0000001
#define REDEF_O_RDWR    0000002
#define REDEF_O_APPEND  0002000
#define REDEF_O_ACCMODE 0000003


/* xil_printf goes directly to serial port */
#define LPRINTF(format, ...) xil_printf(format, ##__VA_ARGS__)
#define LPERROR(format, ...) LPRINTF("ERROR: " format, ##__VA_ARGS__)

/* Global functions and variables */
extern int init_system(void);
extern void cleanup_system(void);
extern int is_logger_init_finish(void);

/* Local variables */
static struct rpmsg_channel *app_rp_chnl;
static volatile int chnl_is_alive = 0;
static struct remote_proc *proc = NULL;
static struct rsc_table_info rsc_info;


static TaskHandle_t comm_task;


#define TIMER_ID	1
#define DELAY_10_SECONDS	10000UL
#define DELAY_1_SECOND		1000UL
#define TIMER_CHECK_THRESHOLD	9
/*-----------------------------------------------------------*/

/* The Tx and Rx tasks as described at the top of this file. */
static void prvTxTask( void *pvParameters );
static void prvRxTask( void *pvParameters );
static void vTimerCallback( TimerHandle_t pxTimer );
/*-----------------------------------------------------------*/
/* The queue used by the Tx and Rx tasks, as described at the top of this
file. */
static TaskHandle_t xTxTask;
static TaskHandle_t xRxTask;
static QueueHandle_t xQueue = NULL;
static TimerHandle_t xTimer = NULL;
char HWstring[15] = "Hello World";
long RxtaskCntr = 0;

/* for ioctl cmd, size must be less than 512 Byte */
struct rpu_cmd{
	int cmd_id;
	char data[256];
};

/* for rpu cmd */
#define RPU_SET_PARAMS		0x8001		// for set
#define RPU_GET_PARAMS		0x0001		// for get

static volatile unsigned long config_ept = 0;	// FOR SOFTWARE CONFIG


/*-----------------------------------------------------------------------------*
 *  RPMSG callbacks setup by remoteproc_resource_init()
 *-----------------------------------------------------------------------------*/
static void rpmsg_read_cb(struct rpmsg_channel *rp_chnl, void *data, int len,
			  void *priv, unsigned long src)
{
	(void)rp_chnl;
	(void)data;
	(void)len;
	(void)priv;
	(void)src;


	struct rpu_cmd response;
	struct rpu_cmd *rpu_cmd = (struct rpu_cmd *)data;
	static int cnt = 0;


	if(src != PROXY_ENDPOINT){
		config_ept = src;
		/* send back for in need */
		//rpmsg_sendto(rp_chnl, "welcome to use get_cmd\n", sizeof("welcome to use get_cmd\n"), config_ept);

		/* you can get params and do your work here */
		/* Always read register here and send back to APU */
		if(rpu_cmd->cmd_id == RPU_GET_PARAMS){
			memset(&response, 0, sizeof(response));
			response.cmd_id = RPU_GET_PARAMS;
			sprintf(response.data, "%d:%s", cnt++, "I am RPU1, welcome to use get_cmd\n");
			rpmsg_sendto(rp_chnl, &response, sizeof(response), config_ept);
		}else{
			/* you  should always send back something to consume vring buffer */
			/* Usually need to return ACK as the response signal */

			/*if(is_logger_init_finish()){
				log_i("[%d] %s", cnt, rpu_cmd->data);
			}*/

			memset(&response, 0, sizeof(response));
			response.cmd_id = RPU_SET_PARAMS;
			sprintf(response.data, "%d:%s", cnt++, "I am RPU1, here is set_cmd\n");
			rpmsg_sendto(rp_chnl, &response, sizeof(response), config_ept);
		}
	}

}

static void rpmsg_channel_created(struct rpmsg_channel *rp_chnl)
{
	app_rp_chnl = rp_chnl;
	chnl_is_alive = 1;
}

static void rpmsg_channel_deleted(struct rpmsg_channel *rp_chnl)
{
	(void)rp_chnl;
	app_rp_chnl = NULL;
}

static void shutdown_cb(struct rpmsg_channel *rp_chnl)
{
	(void)rp_chnl;
	chnl_is_alive = 0;
}

/*-----------------------------------------------------------------------------*
 *  Application specific
 *-----------------------------------------------------------------------------*/
int app (struct hil_proc *hproc)
{
	int ret;
	int status;

	/* Initialize framework */
	status = remoteproc_resource_init(&rsc_info, hproc,
					  rpmsg_channel_created,
					  rpmsg_channel_deleted, rpmsg_read_cb,
					  &proc, 0);
	if (RPROC_SUCCESS != status) {
		LPERROR("Failed  to initialize remoteproc resource.\n");
		return -1;
	}

	while (!chnl_is_alive) {
		hil_poll(proc->proc, 0);
	}

	/* redirect I/Os */
	rpmsg_retarget_init(app_rp_chnl, shutdown_cb);


	/* logger system init. must be invoke after rpmsg_retarget_init */
	ret = elog_init();
	if(!ret){
		/* set log format */
		elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);
		elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
		elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
		elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
		elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
		elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_LVL | ELOG_FMT_TAG  | ELOG_FMT_TIME);
		/* start logger */
		elog_start();
	}

	/* Task will always poll here until channel close */
	while (chnl_is_alive) {
		hil_poll(proc->proc, 0);
	}

	/* logger exit */
	elog_exit();

	rpmsg_retarget_deinit(app_rp_chnl);

	/* disable interrupts and free resources */
	remoteproc_resource_deinit(proc);

	return 0;
}

/*-----------------------------------------------------------------------------*
 *  Processing Task
 *-----------------------------------------------------------------------------*/
static void processing(void *unused_arg)
{
	unsigned long proc_id = 0;
	unsigned long rsc_id = 0;
	struct hil_proc *hproc;

	(void)unused_arg;

	LPRINTF("Starting RPMSG channel...\n");
	/* Initialize HW system components */
	init_system();

	/* Create HIL proc */
	hproc = platform_create_proc(proc_id);
	if (!hproc) {
		LPERROR("Failed to create hil proc.\n");
	} else {
		rsc_info.rsc_tab =
			get_resource_table((int)rsc_id, &rsc_info.size);
		if (!rsc_info.rsc_tab) {
			LPERROR("Failed to get resource table data.\n");
		} else {
			(void) app(hproc);
		}
	}

	LPRINTF("Stopping RPMSG channel...\n");
	cleanup_system();

	/* Terminate this task */
	vTaskDelete(NULL);
}



/*-----------------------------------------------------------*/
static void prvTxTask( void *pvParameters )
{
	const TickType_t x1second = pdMS_TO_TICKS( DELAY_1_SECOND );

	/* Waiting for logger channel create done */
	while(!is_logger_init_finish()){
		vTaskDelay( x1second );
	}

	for( ;; )
	{
		/* Delay for 1 second. */
		vTaskDelay( x1second );

		/* Send the next value on the queue.  The queue should always be
		empty at this point so a block time of 0 is used. */
		xQueueSend( xQueue,			/* The queue being written to. */
					HWstring, /* The address of the data being sent. */
					0UL );			/* The block time. */

	}
}

/*-----------------------------------------------------------*/
static void prvRxTask( void *pvParameters )
{
	char Recdstring[15] = "";
	const TickType_t x1second = pdMS_TO_TICKS( 300 );

	/* Waiting for logger channel create done */
	while(!is_logger_init_finish()){
		vTaskDelay( x1second );
	}

	for( ;; )
	{
		/* Block to wait for data arriving on the queue. */
		xQueueReceive( 	xQueue,				/* The queue being read. */
						Recdstring,	/* Data is read into this address. */
						portMAX_DELAY );	/* Wait without a timeout for data. */

		/* Print the received data. */
		xil_printf( "RPU1 Rx task received string from Tx task: %s\r\n", Recdstring );

		/* invoke logger API to log something in the file */
		log_i("[%d] Hello EasyLogger!", RxtaskCntr);
		RxtaskCntr++;
		/* Delay for 1 second. */
		vTaskDelay( x1second );
	}
}



/*-----------------------------------------------------------*/
static void vTimerCallback( TimerHandle_t pxTimer )
{
	long lTimerId;
	configASSERT( pxTimer );

	lTimerId = ( long ) pvTimerGetTimerID( pxTimer );

	if (lTimerId != TIMER_ID) {
		xil_printf("FreeRTOS Hello World Example FAILED");
	}

	/* If the RxtaskCntr is updated every time the Rx task is called. The
	 Rx task is called every time the Tx task sends a message. The Tx task
	 sends a message every 1 second.
	 The timer expires after 10 seconds. We expect the RxtaskCntr to at least
	 have a value of 9 (TIMER_CHECK_THRESHOLD) when the timer expires. */
	if (RxtaskCntr >= TIMER_CHECK_THRESHOLD) {
		xil_printf("FreeRTOS Hello World Example PASSED");
	} else {
		xil_printf("FreeRTOS Hello World Example FAILED");
	}

	vTaskDelete( xRxTask );
	vTaskDelete( xTxTask );
}




/*-----------------------------------------------------------------------------*
 *  Application entry point
 *-----------------------------------------------------------------------------*/
int main(void)
{
	BaseType_t stat;
	const TickType_t x10seconds = pdMS_TO_TICKS( DELAY_10_SECONDS );

	/* Create the tasks */
	stat = xTaskCreate(processing, ( const char * ) "RPMSG",
				1024, NULL, 2, &comm_task);
	if (stat != pdPASS) {
		LPERROR("cannot create RPMSG task\n");
	} else {

		/* Create the two tasks.  The Tx task is given a lower priority than the
		Rx task, so the Rx task will leave the Blocked state and pre-empt the Tx
		task as soon as the Tx task places an item in the queue. */
		xTaskCreate( 	prvTxTask, 					/* The function that implements the task. */
						( const char * ) "Tx", 		/* Text name for the task, provided to assist debugging only. */
						configMINIMAL_STACK_SIZE, 	/* The stack allocated to the task. */
						NULL, 						/* The task parameter is not used, so set to NULL. */
						3,			/* The task runs at the idle priority. */
						&xTxTask );

		xTaskCreate( prvRxTask,
					 ( const char * ) "GB",
					 1024,
					 NULL,
					 4,
					 &xRxTask );

		/* Create the queue used by the tasks.  The Rx task has a higher priority
		than the Tx task, so will preempt the Tx task and remove values from the
		queue as soon as the Tx task writes to the queue - therefore the queue can
		never have more than one item in it. */
		xQueue = xQueueCreate( 	1,						/* There is only one space in the queue. */
								sizeof( HWstring ) );	/* Each space in the queue is large enough to hold a uint32_t. */

		/* Check the queue was created. */
		configASSERT( xQueue );

		/* Create a timer with a timer expiry of 10 seconds. The timer would expire
		 after 10 seconds and the timer call back would get called. In the timer call back
		 checks are done to ensure that the tasks have been running properly till then.
		 The tasks are deleted in the timer call back and a message is printed to convey that
		 the example has run successfully.
		 The timer expiry is set to 10 seconds and the timer set to not auto reload. */
		xTimer = xTimerCreate( (const char *) "Timer",
								x10seconds,
								pdFALSE,
								(void *) TIMER_ID,
								vTimerCallback);
		/* Check the timer was created. */
		configASSERT( xTimer );

		/* start the timer with a block time of 0 ticks. This means as soon
		   as the schedule starts the timer will start running and will expire after
		   10 seconds */
		xTimerStart( xTimer, 0 );

		/* Start running FreeRTOS tasks */
		vTaskStartScheduler();
	}

	/* Will not get here, unless a call is made to vTaskEndScheduler() */
	while (1) ;

	/* suppress compilation warnings*/
	return 0;
}

