#ifndef _TASK_MGR_H_
#define _TASK_MGR_H_

#include "var_type.h"

/*********************************************************************
 * TYPEDEFS
 */
#define ZSUCCESS 0
#define INVALID_TASK 1
#define MSG_BUFFER_NOT_AVAIL 2
#define INVALID_MSG_POINTER 3

/*
 * Event handler function prototype
 */

typedef event32_t (*pTaskEventHandlerFn)(uint8 taskId, event32_t event);

/*********************************************************************
 * GLOBAL VARIABLES
 */

extern const pTaskEventHandlerFn TasksArray[];
extern const uint8 TasksTotalCounts;
extern event32_t *TasksEvents;

void TASKMGR_init(void);
void TASKMGR_run(void);

#endif // _TASK_MGR_H_
