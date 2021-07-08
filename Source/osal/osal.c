#include "osal.h"
#include "task_mgr.h"
#include "timer.h"

#include "common.h"
#include "device.h"
#include "misc.h"

typedef struct {
	void *prev;
	void *next;
	uint8 taskId;
	uint16 timeout;
	event32_t event;
} OsalEventTimer_t;
static OsalEventTimer_t *OsalTimerHead = NULL;

static inline OsalEventTimer_t *findEventTimer(byte taskId, event32_t event)
{
	// Head of the timer list
	OsalEventTimer_t *findTimer = OsalTimerHead;
	// Stop when found or at the end
	while (findTimer) {
		if (findTimer->event == event && findTimer->taskId == taskId) {
			break;
		}
		// Not this one, check another
		findTimer = findTimer->next;
	}
	return (findTimer);
}

static inline OsalEventTimer_t *addEventTimer(byte taskId, event32_t event, uint16 timeout)
{
	OsalEventTimer_t *findTimer = findEventTimer(taskId, event);
	if (findTimer) { // aleady added, update timer timeout.
		findTimer->timeout = timeout;
		return (findTimer);
	} else {
		// Look for an existing timer first
		OsalEventTimer_t *newTimer = (OsalEventTimer_t *)malloc(sizeof(OsalEventTimer_t));
		if (newTimer) {
			// Fill in new timer
			newTimer->taskId = taskId;
			newTimer->event = event;
			newTimer->timeout = timeout;
			newTimer->prev = (void *)NULL;
			newTimer->next = (void *)NULL;

			if (OsalTimerHead == NULL) {
				OsalTimerHead = newTimer;
			} else {
				// Add it to the end of the timer list
				OsalEventTimer_t *lastTimer = OsalTimerHead;
				while (lastTimer->next) {
					lastTimer = lastTimer->next;
				}
				lastTimer->next = newTimer;
				newTimer->prev = lastTimer;
			}
		} else {
			printf("malloc error - adding OSAL event timer\n");
		}
		return (newTimer);
	}
}

static inline void removeEventTimer(OsalEventTimer_t *rmTimer)
{
	// Does the timer list really exist
	if ((OsalTimerHead != NULL) && rmTimer) {
		OsalEventTimer_t *prevTimer = (rmTimer->prev);
		OsalEventTimer_t *nextTimer = (rmTimer->next);
		if (prevTimer == NULL) { // prevTimer == OsalTimerHead
			nextTimer->prev = (void *)NULL;
			OsalTimerHead = nextTimer;
		} else if (nextTimer == NULL) { // nextTimer == Last
			prevTimer->next = (void *)NULL;
		} else {
			prevTimer->next = nextTimer;
			nextTimer->prev = prevTimer;
		}
		rmTimer->prev = 0;
		rmTimer->next = 0;
		rmTimer->taskId = 0;
		rmTimer->timeout = 0;
		rmTimer->event = 0;

		free(rmTimer);
	}
}

/*********************************************************************
 * @fn      OSAL_startEventTimer
 *
 * @brief
 *
 *   This function is called to start a timer to expire in n mSecs.
 *   When the timer expires, the calling task will get the specified event.
 *
 * @param   byte taskId - task id to set timer for
 * @param   uint16 event - event to be notified with
 * @param   UNINT16 timeout - in milliseconds.
 *
 * @return  ZSUCCESS, or NO_TIMER_AVAIL.
 */
BOOL OSAL_startEventTimer(byte taskId, event32_t event, uint32 timeout)
{
	uint16 timeout16 = (uint16)(timeout / TIMER_INTERVAL);
	if (timeout16 == 0) {
		timeout16 = 1;
	}

	halIntState_t intState;
	HAL_ENTER_CRITICAL_SECTION(intState); // Hold off interrupts.
	OsalEventTimer_t *newTimer = addEventTimer(taskId, event, timeout16);
	HAL_EXIT_CRITICAL_SECTION(intState); // Re-enable interrupts.

	return ((newTimer != NULL) ? TRUE : FALSE);
}

/*********************************************************************
 * @fn      OSAL_stopEventTimer
 *
 * @brief
 *
 *   This function is called to stop a timer that has already been started.
 *   If ZSUCCESS, the function will cancel the timer and prevent the event
 *   associated with the timer from being set for the calling task.
 *
 * @param   byte taskId - task id of timer to stop
 * @param   uint16 event - identifier of the timer that is to be stopped
 *
 * @return  ZSUCCESS or INVALID_EVENT_ID
 */
BOOL OSAL_stopEventTimer(byte taskId, event32_t event)
{
	halIntState_t intState;
	HAL_ENTER_CRITICAL_SECTION(intState); // Hold off interrupts
	OsalEventTimer_t *foundTimer = findEventTimer(taskId, event);
	if (foundTimer) {
		removeEventTimer(foundTimer);
	}
	HAL_EXIT_CRITICAL_SECTION(intState); // Release interrupts

	return ((foundTimer != NULL) ? TRUE : FALSE);
}

/*********************************************************************
 * @fn      OSAL_setEvent
 *
 * @brief
 *
 *    This function is called to set the event flags for a task.  The
 *    event passed in is OR'd into the task's event variable.
 *
 * @param   byte taskId - receiving tasks ID
 * @param   byte event - what event to set
 *
 * @return  ZSUCCESS, INVALID_TASK
 */
BOOL OSAL_setEvent(uint8 taskId, event32_t event)
{
	if (taskId < TasksTotalCounts) {
		halIntState_t intState;
		HAL_ENTER_CRITICAL_SECTION(intState); // Hold off interrupts
		TasksEvents[taskId] |= event; // Stuff the event bit(s)
		HAL_EXIT_CRITICAL_SECTION(intState); // Release interrupts
		return (TRUE);
	} else {
		return (FALSE);
	}
}

BOOL OSAL_isExistEvent()
{
	return (OsalTimerHead == NULL) ? FALSE : TRUE;
}

void OSAL_init()
{
	if (OsalTimerHead != NULL) {
		free(OsalTimerHead);
		OsalTimerHead = NULL;
	}
}

void OSAL_eventHandler()
{
#define EVENT_UPDATE_TIME 1
	// Look for open timer slot
	if (OsalTimerHead != NULL) {
		// Add it to the end of the timer list
		OsalEventTimer_t *srchTimer = OsalTimerHead;
		// Look for open timer slot
		while (srchTimer) {
			// Decrease the correct amount of time
			if (srchTimer->timeout > EVENT_UPDATE_TIME) {
				srchTimer->timeout -= EVENT_UPDATE_TIME;
			} else {
				srchTimer->timeout = 0;
			}

			// When timeout, execute the task
			if (srchTimer->timeout == 0) {
				OSAL_setEvent(srchTimer->taskId, srchTimer->event);

				OsalEventTimer_t *rmTimer = srchTimer;
				srchTimer = srchTimer->next;
				removeEventTimer(rmTimer);
			} else {
				// Get next
				srchTimer = srchTimer->next;
			}
		}
	}
}
