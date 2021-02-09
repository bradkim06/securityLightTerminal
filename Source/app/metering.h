#ifndef __METERING_H__
#define __METERING_H__

#include "meter.h"

#define LEN_MAX_METER_TX 0x40
#define LEN_MAX_METER_RX 0x60

//**************************************
// Electric Meter Message
//**************************************

/* Terminal -> Meter */
#define TERMINAL_STX 0x24
#define TERMINAL_ETX 0x16

#define REQ_READ_METER 0xA0
#define SET_METER_CONFIG 0xA1
#define SET_LIGHT_TIME 0xA2
#define SET_RTC_TIME 0xA3
#define SET_CALIB_FACTOR 0xA4

/* Meter -> Terminal */
#define METER_STX 0x5B
#define METER_ETX 0x8D

#define RESP_READ_METER 0xB0
#define RESP_METER_CONFIG 0xB1
#define RESP_LIGHT_TIME 0xB2
#define RESP_RTC_TIME 0xB3
#define RESP_CALIB_FACTOR 0xB4

typedef struct {
	uint8 stx;
	uint8 msgType;
	uint8 checkSum;
	uint8 etx;
} evMeterReq_t;

//! \brief Type for Power variables
//!
typedef int64_t EM_Power_t;

//! \brief Type for Energy variables
//!
typedef uint64_t EM_Energy_t;

//! \brief Selects appropiate phase to provide results for
//!
typedef enum _EM_Phase_ {
	EM_Phase_A = 0x01,
	//!< - Results requested for phase A
	EM_Phase_B = 0x02,
	//!< - Results requested for phase B
	EM_Phase_C = 0x04,
	//!< - Results requested for phase C
	EM_Phase_D = 0x08,
	//!< - Results requested for phase D
	EM_Phase_E = 0x10,
	//!< - Results requested for phase E
	EM_Phase_F = 0x20,
	//!< - Results requested for phase F
	EM_Phase_Neutral = 0x40
	//!< - Results requested for phase Neutral
} EM_Phase;

//! \brief Contains EM Phase Result
//!
typedef struct _EM_Phase_Results_ {
	uint32_t VRMS;
	//!< VRMS for previous frame (mV)
	uint32_t IRMS;
	//!< IRMS for previous frame (uA)
	uint32_t vPeak;
	//!< VPeak since system initialization (mV)
	uint32_t iPeak;
	//!< IPeak since system initialization (uA)
	int32_t powerFactor;
	//!< Power Factor calculated when the EM_powerFactor
	uint16_t frequency;
	//!< The Frequency is stored in 0.01Hz steps. A 16 bit unsigned number
	//!< supports the range 0.01Hz to 655.35Hz, which is clearly more than we
	//!< will ever need for the fundamental.
	EM_Power_t activePower;
	//!< Active Power for previous frame (uW)
	EM_Power_t reactivePower;
	//!< Reactive Power for previous frame (uVAr)
	EM_Power_t apparentPower;
	//!< Apparent Power for previous frame (uVA)
	EM_Energy_t activeEnergy;
	//!< Active Energy for previous frame (uWh)
	EM_Energy_t reactiveEnergy;
	//!< Reactive Energy for previous frame (uVArh)
	EM_Energy_t apparentEnergy;
	//!< Apparent Energy for previous frame (uVAh)
	EM_Phase phaseID;
	//!< Present Phase ID Name
} EM_Phase_Results;

typedef struct {
	uint8_t stx;
	uint8_t msgType;
	uint8_t status;
	uint32_t volt;
	uint32_t curr;
	uint32_t shortCurr;
	int32_t power;
	int32_t cumPower;
	uint8_t checkSum;
	uint8_t etx;
} elecMeterResp_t;

BOOL METER_sendRequest(uint8 meterType);
BOOL METER_recvResponse(uint8 meterType, MeterUnitData_t *pUnit);

void METER_enable(uint8 meterType);

void METER_bypassReq(uint8 *body, int bodyLen);
BOOL METER_bypassResp();

#endif //__METERING_H__
