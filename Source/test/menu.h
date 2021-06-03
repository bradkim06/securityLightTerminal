#ifndef _MENU_H_

typedef struct {
	int index;
	char *cmd;
	char *usage;
	char *desc;
} ShellCommandSet_t;

#define CMDN_QMARK 0
#define CMDN_RFLASH 1
#define CMDN_EFLASH 2
#define CMDN_PBATT 3
#define CMDN_RSN 4
#define CMDN_WSN 5
#define CMDN_PRESET 6
#define CMDN_CONFIG 7
#define CMDN_MP 8
#define CMDN_MT 9
#define CMDN_SI 10
#define CMDN_DEBUG 11
#define CMDN_RMETER 12
#define CMDN_MODEM 13
#define CMDN_PWSN 14
#define CMDN_PRSN 15
#define CMDN_PVER 16
#define CMDN_PMODEM 17
#define CMDN_PWRTC 18
#define CMDN_PNFC 19
#define CMDN_RNFC 20
#define CMDN_RI_CTRL 21
#define CMDN_PRNBID 22
#define CMDN_PWSERVER 23
#define CMDN_PRSERVER 24
#define CMDN_PWFOTA 25
#define CMDN_PRFOTA 26
#define CMDN_PNBCTX 27

const ShellCommandSet_t CmdSet[] = {
	{
		CMDN_QMARK,
		"?",
		"?",
		"Help command",
	},
	{
		CMDN_RFLASH,
		"rflash",
		"rflash INFO_FLASH_NUMBER(0~3)",
		"Read info flash memory",
	},
	{
		CMDN_EFLASH,
		"eflash",
		"eflash INFO_FLASH_NUMBER(0~3)",
		"Erase info flash memory",
	},
	{
		CMDN_PBATT,
		"pbatt",
		"pbatt",
		"Read battery level",
	},
	{
		CMDN_RSN,
		"rsn",
		"rsn",
		"Read serial number",
	},
	{
		CMDN_WSN,
		"wsn",
		"wsn S/N",
		"Write serial number",
	},
	{
		CMDN_PRESET,
		"preset",
		"preset",
		"Reset system",
	},
	{
		CMDN_CONFIG,
		"config",
		"config",
		"Show configuration",
	},
	{
		CMDN_MP,
		"mp",
		"mp RI(1~24)",
		"Write metering parameter",
	},
	{
		CMDN_MT,
		"mt",
		"mt TYPE",
		"Select meter type",
	},
	{
		CMDN_SI,
		"short",
		"short FLAG(0,1)",
		"On/Off short interval",
	},
	{
		CMDN_DEBUG,
		"debug",
		"debug FLAG(0,1)",
		"On/Off debug print",
	},
	{
		CMDN_RMETER,
		"rmeter",
		"rmeter TYPE TIME(sec)",
		"Read meter continuously",
	},
	{
		CMDN_MODEM,
		"modem",
		"modem",
		"Enter AT command mode(LoRa/NB-IoT)",
	},
	{
		CMDN_PWSN,
		"pwsn",
		"pwsn S/N RI(1~24)",
		"Write device parameter",
	},
	{
		CMDN_PRSN,
		"prsn",
		"prsn",
		"Read device config",
	},
	{
		CMDN_PVER,
		"pver",
		"pver",
		"Read F/W version",
	},
	{
		CMDN_PMODEM,
		"pmodem",
		"pmodem",
		"Check modem ready",
	},
	{
		CMDN_PWRTC,
		"pwrtc",
		"pwrtc YEAR MONTH DAY HOUR MIN SEC",
		"Read/Write RTC time",
	},
	{
		CMDN_PNFC,
		"nfc",
		"nfc",
		"Check NFC",
	},
	{
		CMDN_RNFC,
		"pfrnfc",
		"pfrnfc",
		"Factory reset of NFC Tag",
	},
	{
		CMDN_RI_CTRL,
		"rictrl",
		"rictrl FLAG(0,1)",
		"On/Off report interval",
	},
	{
		CMDN_PRNBID,
		"prnbid",
		"prnbid",
		"Read NB-IoT IMEI IMSI",
	},
	{
		CMDN_PWSERVER,
		"pwserver",
		"pwserver IP(X.X.X.X) PORT SEVICE_CODE",
		"Write server address, service code",
	},
	{
		CMDN_PRSERVER,
		"prserver",
		"prserver",
		"Read server address, service code",
	},
	{
		CMDN_PWFOTA,
		"pwfota",
		"pwfota IP(X.X.X.X) PORT INTERVAL(DAY)",
		"Write FOTA server address, interval",
	},
	{
		CMDN_PRFOTA,
		"prfota",
		"prfota",
		"Read FOTA server address, interval",
	},
	{
		CMDN_PNBCTX,
		"pnbctx",
		"pnbctx",
		"Test NB-IoT Signal quality",
	},
};

#endif // _MENU_H_
