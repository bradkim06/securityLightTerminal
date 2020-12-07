
#ifndef OSAL_HEADER
#define OSAL_HEADER

#include "var_type.h"

BOOL OSAL_startEventTimer( byte taskId, event32_t event, uint32 timeout );
BOOL OSAL_stopEventTimer( uint8 taskId, event32_t event );
BOOL OSAL_setEvent( uint8 taskId, event32_t event );
BOOL OSAL_isExistEvent();
void OSAL_init();
void OSAL_eventHandler();

#endif
