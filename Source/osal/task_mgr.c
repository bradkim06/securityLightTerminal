#include "task_mgr.h"
#include "timer.h"

#include "alarm.h"
#include "app.h"
#include "common.h"
#include "device.h"
#include "misc.h"
#include "modem.h"
#include "uart.h"

// The order in this table must be identical to the task initialization calls
// below in osalInitTask.

const pTaskEventHandlerFn TasksArray[] = {
	APP_tasks,
	MODEM_tasks,
};

const uint8 TasksTotalCounts = sizeof(TasksArray) / sizeof(TasksArray[0]);
event32_t *TasksEvents;

void TASKMGR_init(void)
{
	uint8 taskID = 0;

	TasksEvents = (event32_t *)malloc(sizeof(event32_t) * TasksTotalCounts);
	if (TasksEvents == NULL) {
		printf("malloc error - TASKMGR_init\n");
		return;
	}

	memset(TasksEvents, 0, (sizeof(event32_t) * TasksTotalCounts));

	APP_init(taskID++);
	MODEM_init(taskID++);
}

void TASKMGR_run(void)
{
	uint8 idx = 0;

	DeviceSleepState = FALSE;

	for (;;) { // Forever Loop
		WDTCTL = WDTPW | WDTCNTCL; // Clear watchdog timer
		WDTCTL = WDT_ARST_16SEC; // Start watchdog timer(16 sec)

		if (++idx >= TasksTotalCounts) {
			idx = 0;
		}

		do {
			if (TasksEvents[idx]) { // Task is highest priority that is ready.
				break;
			}
		} while (++idx < TasksTotalCounts);

		if (idx < TasksTotalCounts) {
			halIntState_t intState;
			event32_t events;

			HAL_ENTER_CRITICAL_SECTION(
				intState); // Hold off interrupts
			events = TasksEvents[idx];
			TasksEvents[idx] = 0; // Clear the Events for this task.
			HAL_EXIT_CRITICAL_SECTION(
				intState); // Hold off interrupts

			events = (TasksArray[idx])(idx, events);

			HAL_ENTER_CRITICAL_SECTION(
				intState); // Hold off interrupts
			TasksEvents[idx] |=
				events; // Add back unprocessed events to the current task.
			HAL_EXIT_CRITICAL_SECTION(
				intState); // Hold off interrupts

		} else {
			if (OSAL_isExistEvent() != TRUE) {
				int taskIdx;
				for (taskIdx = 0; taskIdx < TasksTotalCounts;
				     taskIdx++) {
					if (TasksEvents[taskIdx] !=
					    0x00000000) {
						break;
					}
				}

				if (taskIdx >= TasksTotalCounts) {
					if (ALARM_status() == 0) {
						ALARM_schedule();
					}
				}
			}
		}
	}
}
