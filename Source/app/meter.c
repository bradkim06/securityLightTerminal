#include "meter.h"
#include "common.h"
#include "misc.h"

#include "alarm.h"
#include "battery.h"
#include "flash.h"
#include "lcd.h"
#include "rtc.h"

typedef struct {
	uint8 caliberDp;
	uint8 dif;
	uint8 vif;
	uint8 meterSerial[4];
	MeterUnitData_t unit;
} MeteringData_t;

typedef struct {
	int nData;
	int storedInterval;
	MeterUnitData_t unit[MAX_NUM_STORED_DATA];
	MeterUnitData_t reportedUnit;
} MeterStoredData_t;

// 마지막 검침 데이터 저장 영역
MeteringData_t LastMetering;

//주기 검침 데이터 저장 영역
MeterStoredData_t StoredMeterData;

static void insertDateToData(Date_t *pDate, MeterUnitData_t *pData,
			     BOOL isIgnoreSec)
{
	// Data의 year는 RTC의 year에서 2000을 빼서 사용
	pData->year = pDate->year - 2000;
	pData->mon = pDate->mon;
	pData->day = pDate->day;
	pData->hour = pDate->hour;
	pData->min = pDate->min;
	pData->sec = (isIgnoreSec == TRUE) ? 0 : pDate->sec;
	pData->isTimeSync = ALARM_isTimeSync();
}

static void copyDateFromData(MeterUnitData_t *pData, Date_t *pDate,
			     BOOL isIgnoreSec)
{
	// RTC의 year는 Data의 year에서 2000을 더해야 함.
	pDate->year = pData->year + 2000;
	pDate->mon = pData->mon;
	pDate->day = pData->day;
	pDate->hour = pData->hour;
	pDate->min = pData->min;
	pDate->sec = (isIgnoreSec == TRUE) ? 0 : pData->sec;
}

char *METER_getMeterName(int meterType)
{
	char *name = "Unknown";
	switch (meterType) {
	case E_METER:
		name = "E_STD";
		break;
	}
	return name;
}

void METER_displayStoredData()
{
	char termSerial[0x10] = "";
	strncpy(termSerial, (char *)conf.serialNum, SERIAL_NUM_LEN);
	termSerial[SERIAL_NUM_LEN] = '\0';

	MeteringData_t *q = &LastMetering;
	MeterStoredData_t *p = &StoredMeterData;

	printf("# %d data, serial no: %02x%02x%02x%02x(%02x) T: %s\n", p->nData,
	       q->meterSerial[0], q->meterSerial[1], q->meterSerial[2],
	       q->meterSerial[3], q->caliberDp, termSerial);

	for (int i = 0; i < p->nData; i++) {
		MeterUnitData_t *pUnit = &p->unit[i];
		printf("   %2d)  %2d-%02d-%02d %02d:%02d:%02d   %02x%02x%02x%02x(%02x)\n",
		       i + 1, pUnit->year, pUnit->mon, pUnit->day, pUnit->hour,
		       pUnit->min, pUnit->sec, pUnit->meterData[0],
		       pUnit->meterData[1], pUnit->meterData[2],
		       pUnit->meterData[3], pUnit->meterStatus);
	}
}

void METER_displayTempData()
{
	char termSerial[0x10] = "";
	strncpy(termSerial, (char *)conf.serialNum, SERIAL_NUM_LEN);
	termSerial[SERIAL_NUM_LEN] = '\0';

	MeteringData_t *p = &LastMetering;

	printf("# serial no: %02x%02x%02x%02x(%02x) T: %s\n", p->meterSerial[0],
	       p->meterSerial[1], p->meterSerial[2], p->meterSerial[3],
	       p->caliberDp, termSerial);

	MeterUnitData_t *pUnit = &LastMetering.unit;
	printf("         %2d-%02d-%02d %02d:%02d:%02d   %02x%02x%02x%02x(%02x)\n",
	       pUnit->year, pUnit->mon, pUnit->day, pUnit->hour, pUnit->min,
	       pUnit->sec, pUnit->meterData[0], pUnit->meterData[1],
	       pUnit->meterData[2], pUnit->meterData[3], pUnit->meterStatus);
}

BOOL METER_isAllFF(uchar *p, int len)
{
	BOOL result = TRUE;

	for (int i = 0; i < len; i++) {
		if (*(p + i) != 0xFF) {
			result = FALSE;
			break;
		}
	}
	return result;
}

void METER_clearTempData()
{
	memset(&LastMetering, 0, sizeof(LastMetering));
}

void METER_clearStoredData(BOOL isClearAll)
{
	memset(&StoredMeterData, 0, sizeof(StoredMeterData));
	StoredMeterData.storedInterval = conf.meterInterval;
	if (isClearAll) {
		memset(&StoredMeterData.reportedUnit, 0xFF,
		       sizeof(MeterUnitData_t));
	}
}

int METER_getNumberOfStoredData()
{
	return StoredMeterData.nData;
}

int METER_getStoredInterval()
{
	return StoredMeterData.storedInterval;
}

uint8 *METER_getSerialNum()
{
	return LastMetering.meterSerial;
}

uint8 METER_getMeterCaliberDp()
{
	return LastMetering.caliberDp;
}

MeterUnitData_t *METER_getTempData()
{
	return &LastMetering.unit;
}

BOOL METER_needNewMetering(int secLimit)
{
	Date_t prev;
	copyDateFromData(&LastMetering.unit, &prev, FALSE);

	Date_t now;
	RTC_read(&now);

	int32 secDiff = MISC_calcSecDiff(&prev, &now);

	return (secDiff < 0 || secDiff > secLimit) ? TRUE : FALSE;
}

MeterUnitData_t *METER_getStoredData(int pos)
{
	int nData = METER_getNumberOfStoredData();
	if (pos >= 0 && pos < nData) {
		return &StoredMeterData.unit[pos];
	}
	return NULL;
}

static int checkStoreInterval(MeterUnitData_t *after, MeterUnitData_t *before,
			      int si)
{
	Date_t prev;
	copyDateFromData(before, &prev, TRUE);

	Date_t next;
	copyDateFromData(after, &next, TRUE);

	int elapsed = 0;
	if (conf.isShortInterval) {
		if (MISC_calcMinDiff(&prev, &next) >= si) {
			elapsed = 1;
		}
	} else {
		if (MISC_calcHourDiff(&prev, &next) >= si) {
			elapsed = 1;
		}
	}

	return elapsed;
}

/**
 * @brief Save meter data in Flash with date
 *
 * @param pDate meter data time
 * @param pUnit meter data
 */
void METER_saveInFlash(Date_t *pDate, MeterUnitData_t *pUnit)
{
	// If RTC is not synchronized, do not save meter data in flash
	if (ALARM_isTimeSync()) {
		FLASH_saveData(pDate, BATT_getVoltage(), pUnit->meterData);
		printf("Meter Data Store in Flash : %04d-%02d-%02d %02dH\n",
		       pDate->year, pDate->mon, pDate->day, pDate->hour);
	} else {
		printf("Meter Data Store in Flash : Not synchronized RTC Time\n");
	}
}

/**
 * @brief Save meter data in RAM memory.
 *
 * @param pDate meter data time
 * @param pUnit meter data
 */
void METER_addStoredData(Date_t *pDate, MeterUnitData_t *pUnit)
{
	if (conf.isUsingPF) {
		// Save meter data in RAM memory. (Keep datas, not tuning)
		insertDateToData(pDate, pUnit, TRUE);
		// Interval 유효성 관계없이 마지막 검침값을 저장
		memcpy(&LastMetering.unit, pUnit, sizeof(MeterUnitData_t));

		MeterStoredData_t *p = &StoredMeterData;
		printf("%d data, storedInterval = %d\n", p->nData,
		       p->storedInterval);
		if (p->nData != 0 &&
		    checkStoreInterval(pUnit, &p->unit[0], p->storedInterval) ==
			    0) {
			printf("within storedInterval - not saved\n");
			return;
		}

		int nMaxData = MAX_NUM_STORED_DATA;
		if (p->nData >= nMaxData) {
			// 저장된 데이터가 "최대 데이터 개수" 이상인 경우 마지막 데이터를
			// 버리기위해 현재 데이터 개수를 "최대 데이터 개수 - 1"로 설정한다.
			p->nData = nMaxData - 1;
		}

		if (p->nData) {
			for (int i = p->nData - 1; i >= 0; i--) {
				memcpy(&p->unit[i + 1], &p->unit[i],
				       sizeof(MeterUnitData_t));
			}
		}
		memcpy(&p->unit[0], pUnit, sizeof(MeterUnitData_t));
		p->nData++;
	} else {
		// Save meter data in RAM memory. (If num of data is max, compress saved
		// datas)
		insertDateToData(pDate, pUnit, TRUE);
		// Interval 유효성 관계없이 마지막 검침값을 저장
		memcpy(&LastMetering.unit, pUnit, sizeof(MeterUnitData_t));

		MeterStoredData_t *p = &StoredMeterData;
		printf("%d data, storedInterval = %d\n", p->nData,
		       p->storedInterval);
		if (p->nData != 0 &&
		    checkStoreInterval(pUnit, &p->unit[0], p->storedInterval) ==
			    0) {
			printf("within storedInterval - not saved\n");
			return;
		}

		int nMaxData = MAX_NUM_STORED_DATA;
		if (p->nData >= nMaxData) {
			switch (p->storedInterval) {
			case 1:
			case 2:
			case 3:
			case 4:
			case 6:
			case 12:
				// 하나 걸러 하나씩 없앰 - 짝수 번째 것은 무조건 지우고,
				// 홀수 번째 것은 1->0, 3->1, 5->2, 7->3, 9->4와 같이 이동함.
				// 결과적으로 데이터의 갯수는 절반이 됨
				for (int i = 0; i < p->nData; i++) {
					if ((i % 2) == 0) {
						memset(&p->unit[i], 0,
						       sizeof(MeterUnitData_t));
					} else {
						memcpy(&p->unit[(i - 1) / 2],
						       &p->unit[i],
						       sizeof(MeterUnitData_t));
					}
				}
				p->nData /= 2;
				p->storedInterval *= 2;
				break;

			case 8:
				// 하나 걸러 두개씩 없앰
				// 0,1, 3,4, 6,7, 9,10 등은 삭제
				// 2-->0, 5-->1, 8-->2, 11-->3, 14-->4, 17-->5방식으로 이동
				// 결과적으로 데이터의 갯수는 1/3이 됨
				for (int i = 0; i < p->nData; i++) {
					if ((i % 3) == 2) {
						memcpy(&p->unit[(i - 2) / 3],
						       &p->unit[i],
						       sizeof(MeterUnitData_t));
					} else {
						memset(&p->unit[i], 0,
						       sizeof(MeterUnitData_t));
					}
				}
				p->nData /= 3;
				p->storedInterval = 24;
				break;

			case 24:
				p->nData = nMaxData - 1;
				break;

			default:
				p->storedInterval = conf.meterInterval;
				METER_clearStoredData(FALSE);
				break;
			}
		}

		if (p->nData) {
			for (int i = p->nData - 1; i >= 0; i--) {
				memcpy(&p->unit[i + 1], &p->unit[i],
				       sizeof(MeterUnitData_t));
			}
		}
		memcpy(&p->unit[0], pUnit, sizeof(MeterUnitData_t));
		p->nData++;
	}
}

void METER_collectStoredData()
{
	uint32 prevMeter;
	uint32 nextMeter;
	uint32 currentMeter;
	MeterUnitData_t *prev;
	MeterUnitData_t *next;
	MeterUnitData_t *current;

	MeterStoredData_t *p = &StoredMeterData;
	if (p->nData <= 0) {
		return;
	}

	MeterUnitData_t reportedUnit;
	// Oldest data에 대한 collect를 위해 Last diff data를 복사.
	memcpy(&reportedUnit, &p->reportedUnit, sizeof(MeterUnitData_t));
	// Next collect를 위해 Last data를 복사.
	memcpy(&p->reportedUnit, &p->unit[0], sizeof(MeterUnitData_t));

	// MT_DOWN meter data collection
	int count = 0;
	const int end = p->nData - 1;
	for (int index = end; index >= 0; index--) {
		current = &p->unit[index];
		next = (index == 0) ? NULL : &p->unit[index - 1];
		prev = (index == end) ? &reportedUnit : &p->unit[index + 1];

		if (METER_isAllFF(current->meterData, 4)) {
			if (next == NULL) {
				// Last 값이 MT DOWN 인 경우 이전 검침 값을 copy
				if (METER_isAllFF(prev->meterData, 4) ==
				    FALSE) {
					MISC_bcdToInt(prev->meterData,
						      &prevMeter, 4);
					currentMeter = prevMeter;

					// Meter data, Meter status 복사
					MISC_intToBCD(&currentMeter,
						      current->meterData, 4);
					current->meterStatus =
						prev->meterStatus;
					count++;
				}
			} else {
				// 현재 값이 MT DOWN 이지만, 이전 값과 다음 값이 유효할 경우.
				if ((METER_isAllFF(next->meterData, 4) ==
				     FALSE) &&
				    (METER_isAllFF(prev->meterData, 4) ==
				     FALSE)) {
					MISC_bcdToInt(prev->meterData,
						      &prevMeter, 4);
					MISC_bcdToInt(next->meterData,
						      &nextMeter, 4);
					if (prevMeter > nextMeter) {
						// PREV-NEXT 값이 BACKFLOW인 경우 NEXT 값을 할당.
						currentMeter = nextMeter;
					} else {
						currentMeter = (prevMeter +
								nextMeter) /
							       2;
					}

					// Meter data, Meter status 복사
					MISC_intToBCD(&currentMeter,
						      current->meterData, 4);
					current->meterStatus =
						prev->meterStatus;
					count++;
				}
			}
		}
	}

	if (count > 0) {
		printf_ts("METER : %d datas are collected\n", count);
	}

#if 0 // TODO : will be implemented bacflow meter data collection
    // BACKFLOW meter data collection (oldest 값은 보정하지 않음)
    for (int index = end - 1; index >= 0; index--) {
        current = &p->unit[index];
        prev = &p->unit[index+1];

        if ((METER_isAllFF(current->meterData, 4) == FALSE) &&
            (METER_isAllFF(prev->meterData, 4)    == FALSE) ) {
            MISC_bcdToInt(prev->meterData, &prevMeter, 4);
            MISC_bcdToInt(current->meterData, &currentMeter, 4);
            if(prevMeter > currentMeter) { // 검침 값이 역류인 경우
                currentMeter = prevMeter;

                // Meter data 만 복사.
                MISC_intToBCD(&currentMeter, current->meterData, 4);
            }
        }
    }
#endif
}

void METER_addTempData(Date_t *pDate, MeterUnitData_t *pUnit)
{
	insertDateToData(pDate, pUnit, FALSE);
	memcpy(&LastMetering.unit, pUnit, sizeof(MeterUnitData_t));
}

int METER_getFirstValidPos()
{
	int pos = 0xFF;

	for (int i = 0; i < StoredMeterData.nData; i++) {
		if (METER_isAllFF(StoredMeterData.unit[i].meterData, 4) ==
		    FALSE) {
			pos = i;
			break;
		}
	}

	return pos;
}

void METER_saveMeterInfo(uint8 *serial, uint8 caliberDp, uint8 dif, uint8 vif)
{
	if (memcmp(serial, LastMetering.meterSerial, 4) ||
	    caliberDp != LastMetering.caliberDp) {
		// data를 처음 읽은 것이거나 중간에 계량기가 교체된 것임
		METER_clearStoredData(TRUE);
		memcpy(LastMetering.meterSerial, serial, 4);
		LastMetering.caliberDp = caliberDp;
		if (dif & vif) {
			LastMetering.dif = dif;
			LastMetering.vif = vif;
		}
	}
}
