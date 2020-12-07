#include "misc.h"
#include "device.h"
#include "lcd.h"

int MISC_getDeviceType()
{
#if REPEATER_DEVICE
    return TERM_MODEL_HAT_435W;
#else
    int type = TERM_MODEL_UNKNOWN;
    if(LCD_Exist()) {
        type = TERM_MODEL_HAT_114W;
    } else {
        P8DIR &= ~BM(HTM115W_DETECT_P8_PIN);
        P8REN |=  BM(HTM115W_DETECT_P8_PIN);
        P8OUT |=  BM(HTM115W_DETECT_P8_PIN);

        MISC_delayUs(1);
        if(P8IN & BM(HTM115W_DETECT_P8_PIN)) {
            type = TERM_MODEL_HAT_124W;
        } else {
            type = TERM_MODEL_HTM_115W;
        }

        P8REN &= ~BM(HTM115W_DETECT_P8_PIN);
        P8OUT &= ~BM(HTM115W_DETECT_P8_PIN);
    }
    return type;
#endif
}

uint8 MISC_asciiToHex(char ch)
{
    uint8   hex = 0;

    if(ch >= 'A' && ch <= 'F') {
        hex = (ch - 'A') + 10;
    } else if(ch >= 'a' & ch <= 'f') {
        hex = (ch - 'a') + 10;
    } else if(ch >= '0' & ch <= '9') {
        hex = ch - '0';
    }
    return hex;
}

uint8 MISC_asciiToBCD(char a, char b)
{
    return (MISC_asciiToHex(a) * 0x10 + MISC_asciiToHex(b));
}

uint8 MISC_hexStringToInt(uint8 value)
{
    if(value >= '0' && value <= '9') {
        return (value - '0');
    }

    if(value >= 'A' && value <= 'F') {
        return (value - 'A' + 10);
    }

    if(value >= 'a' && value <= 'f') {
        return (value - 'a' + 10);
    }

    return 0xff;
}

void MISC_bcdToInt(uint8 *pBCD, uint32 *pInt, uint8 nDigit)
{
    uint32  value = 0;
    uint8   temp = 0;

    for(int i = 0; i < nDigit; i++) {
        value *= 100;
        temp = *(pBCD + i);
        temp = ((temp & 0xf0) >> 4) * 10 + (temp & 0x0f);
        value += temp;
    }

    *pInt = value;
}

void MISC_intToBCD(uint32 *pInt, uint8 *pBCD, uint8 nDigit)
{
    uint32 value = *pInt;
    uint8   temp = 0;

    for(int i = 0; i < nDigit; i++) {
        temp = value % 100;
        value /= 100;
        *(pBCD + (nDigit-1-i)) = (temp / 10) * 0x10 + (temp % 10);
    }
}

void MISC_ipToHexArray(char *ip, uint8 *hexIp)
{
    char *p = ip;

    *(hexIp + 0) = atoi(p);

    int valid = 1;
    for(int i = 1; i < 4; i++) {
        if((p = strstr(p, ".")) == NULL) {
            valid = 0;
            break;
        }
        p += 1;

        *(hexIp + i) = atoi(p);
    }

    if(valid == 0) {
        memset(hexIp, 0, 4);
    }
}

#pragma FUNCTION_OPTIONS(MISC_delayUs, "--opt_level=off")
void MISC_delayUs( uint32 timeout )
{
    // This sequence uses exactly 8 clock cycle for each round (1 micro sec)
    do {
        NOP();
        NOP();
        NOP();
        NOP();
    } while( --timeout );
} // MISC_delayUs

#pragma FUNCTION_OPTIONS(MISC_delayMs, "--opt_level=off")
void MISC_delayMs( uint32 timeout )
{
    volatile unsigned int i;
    // This sequence uses exactly 8000 clock cycle for each round (1ms)
    do {
        i = 1000;
        do {
            NOP();
            NOP();
            NOP();
            NOP();
        } while( --i );
    } while( --timeout );
} // MISC_delayMs

                        //  1   2   3   4   5   6   7   8   9  10  11  12
const uint8 MaxDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

void MISC_decDateHour(Date_t *p, uint8 hour)
{
    if(p->hour >= hour) {
        p->hour -= hour;
        return;
    }

    p->hour += (24 - hour);

    if(--p->day > 0) {
        return;
    }

    if(--p->mon > 0) {
        p->day = MaxDays[p->mon-1];
        if(p->mon == 2 && (p->year%4) == 0) {
            p->day = 29;                    // leap year
        }
    } else {
        p->day = 31;
        p->mon = 12;
        p->year--;
    }
}

void MISC_incDateHour(Date_t *p, uint8 hour)
{
    p->hour += hour;
    if(p->hour < 24) {
        return;
    }
    p->hour -= 24;

    uint8  maxDay = MaxDays[p->mon-1];
    if(p->mon == 2 && (p->year%4) == 0){
        maxDay++;
    }

    p->day++;
    if(p->day <= maxDay) {
        return;
    }
    p->day = 1;

    p->mon++;
    if(p->mon <= 12) {
        return;
    }
    p->mon = 1;

    p->year++;
}

void MISC_incDateTime(Date_t *p, uint16 secOffset)
{
    if(p->year < YEAR_MIN || p->year > YEAR_MAX) {
        return;
    }

    if(secOffset == 0 || (secOffset > ((uint16)3600 * (uint16)18))) {
        return;
    }

    uint8  inc_hour = secOffset/3600;
    secOffset %= 3600;

    uint8  inc_min  = secOffset/60;
    uint8  inc_sec  = secOffset%60;


    p->sec += inc_sec;
    if(p->sec >= 60) {
        p->sec -= 60;
        p->min++;
    }

    p->min += inc_min;
    if(p->min >= 60) {
        p->min -= 60;
        p->hour++;
    }

    p->hour += inc_hour;
    if(p->hour < 24) {
        return;
    }
    p->hour -= 24;

    uint8  maxDay = MaxDays[p->mon-1];
    if(p->mon == 2 && (p->year%4) == 0){
        maxDay++;
    }

    p->day++;
    if(p->day <= maxDay) {
        return;
    }
    p->day = 1;

    p->mon++;
    if(p->mon <= 12) {
        return;
    }
    p->mon = 1;

    p->year++;
}

int MISC_isValidDate(Date_t *date)
{
    int valid = 0;

    do {
        if(date->year < YEAR_MIN || date->year > YEAR_MAX) {
            break;
        }

        if(date->mon < 1 || date->mon > 12) {
            break;
        }

        if(date->day < 1 || date->day > 31) {
            break;
        }

        if(date->hour > 23 || date->min > 59 || date->sec > 59) {
            break;
        }

        valid = 1;

    } while(0);

    return valid;
}

long MISC_calcSecDiff(Date_t *prev, Date_t *next)
{
    struct tm   time;

    time.tm_year = prev->year - 1900;
    time.tm_mon  = prev->mon - 1;
    time.tm_mday = prev->day;
    time.tm_hour = prev->hour;
    time.tm_min  = prev->min;
    time.tm_sec  = prev->sec;

    long prevTime = (uint32)(mktime(&time));

    time.tm_year = next->year - 1900;
    time.tm_mon  = next->mon - 1;
    time.tm_mday = next->day;
    time.tm_hour = next->hour;
    time.tm_min  = next->min;
    time.tm_sec  = next->sec;

    long nextTime = (uint32)(mktime(&time));

    return nextTime - prevTime;
}

int MISC_calcMinDiff(Date_t *prev, Date_t *next)
{
    const long secLimit = (32000 * 60L);

    long secDiff = MISC_calcSecDiff(prev, next);
    if(secDiff < 0) {
        secDiff = 0;
    }

    int minDiff = 32000;
    if(secDiff < secLimit) {
        minDiff = (int)(secDiff/60);
    }

    return minDiff;
}

int MISC_calcHourDiff(Date_t *prev, Date_t *next)
{
    uint8   prevMin = prev->min;
    uint8   nextMin = next->min;
    uint8   prevSec = prev->sec;
    uint8   nextSec = next->sec;

    prev->min = next->min = 0;
    prev->sec = next->sec = 0;
    int hourDiff = MISC_calcMinDiff(prev, next)/60;

    prev->min = prevMin;  next->min = nextMin;
    prev->sec = prevSec;  next->sec = nextSec;

    return hourDiff;
}

BOOL MISC_checkInterval(int h, int basetime, int interval)
{
    int hour = h + 24 - basetime;
    return ((hour % interval) == 0) ? TRUE : FALSE;
}

static uint16 ShiftReg;
static uint16 Mask;

/* Initialize the seed from the ID of the node */
BOOL MISC_initRandom(uint32 myAddr) {
    ShiftReg = 119 * 119 * ((myAddr&0xFFFF) + 1);
//initSeed = ShiftReg;
    Mask = 137 * 29 * ((myAddr&0xFFFF) + 1);

    return SUCCESS;
}

/* Return the next 16 bit random number */
uint16 MISC_getRandomNum() {
    BOOL endbit;
    uint16 tmpShiftReg;

    tmpShiftReg = ((ShiftReg+RTCPS) & 0xFFFF);
    endbit = ((tmpShiftReg & 0x8000) != 0);
    tmpShiftReg <<= 1;
    if (endbit)
    tmpShiftReg ^= 0x100b;
    tmpShiftReg++;
    ShiftReg = tmpShiftReg;
    tmpShiftReg = tmpShiftReg ^ Mask;

    return tmpShiftReg;
}
