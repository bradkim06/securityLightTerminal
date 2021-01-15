################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

CG_TOOL_ROOT := C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS

GEN_OPTS__FLAG := 
GEN_CMDS__FLAG := 

ORDERED_OBJS += \
"./Source/app/alarm.obj" \
"./Source/app/app.obj" \
"./Source/app/flash.obj" \
"./Source/app/meter.obj" \
"./Source/app/metering.obj" \
"./Source/app/nfc_app.obj" \
"./Source/dev/battery.obj" \
"./Source/dev/flash_msp430.obj" \
"./Source/dev/lcd.obj" \
"./Source/dev/nfc.obj" \
"./Source/dev/rtc.obj" \
"./Source/dev/timer.obj" \
"./Source/dev/uart.obj" \
"./Source/main.obj" \
"./Source/misc/crc.obj" \
"./Source/misc/misc.obj" \
"./Source/misc/queue.obj" \
"./Source/modem/nb/at_cmd.obj" \
"./Source/modem/nb/message.obj" \
"./Source/modem/nb/modem.obj" \
"./Source/modem/nb/nbiot_bc95g.obj" \
"./Source/modem/nb/uuid/md5.obj" \
"./Source/modem/nb/uuid/uuid.obj" \
"./Source/osal/osal.obj" \
"./Source/osal/task_mgr.obj" \
"./Source/test/modem_test.obj" \
"./Source/test/shell.obj" \
"./Source/test/test.obj" \
"../lnk_msp430f5419a.cmd" \
$(GEN_CMDS__FLAG) \
-llibmath.a \
-llibc.a \

-include ../makefile.init

RM := DEL /F
RMDIR := RMDIR /S/Q

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include Source/app/subdir_vars.mk
-include Source/dev/subdir_vars.mk
-include Source/subdir_vars.mk
-include Source/misc/subdir_vars.mk
-include Source/modem/nb/subdir_vars.mk
-include Source/modem/nb/uuid/subdir_vars.mk
-include Source/osal/subdir_vars.mk
-include Source/test/subdir_vars.mk
-include subdir_rules.mk
-include Source/app/subdir_rules.mk
-include Source/dev/subdir_rules.mk
-include Source/subdir_rules.mk
-include Source/misc/subdir_rules.mk
-include Source/modem/nb/subdir_rules.mk
-include Source/modem/nb/uuid/subdir_rules.mk
-include Source/osal/subdir_rules.mk
-include Source/test/subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(SV7A_DEPS)),)
-include $(SV7A_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
TERMINAL_NBIOT_BC95G.out \

EXE_OUTPUTS__QUOTED += \
"TERMINAL_NBIOT_BC95G.out" \

BIN_OUTPUTS += \
TERMINAL_NBIOT_BC95G.hex \

BIN_OUTPUTS__QUOTED += \
"TERMINAL_NBIOT_BC95G.hex" \


# All Target
all: $(OBJS) $(CMD_SRCS) $(GEN_CMDS)
	@$(MAKE) --no-print-directory -Onone "TERMINAL_NBIOT_BC95G.out" secondary-outputs

# Tool invocations
TERMINAL_NBIOT_BC95G.out: $(OBJS) $(CMD_SRCS) $(GEN_CMDS)
	@echo 'Building target: "$@"'
	@echo 'Invoking: MSP430 Linker'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/bin/cl430" -vmspx --data_model=small -O0 --opt_for_speed=0 --use_hw_mpy=F5 --advice:power="1,3,4,7,9" --define=__MSP430F5419A__ --define=__MODEM_BC95G__ --define=DEBUG --define=REPEATER_DEVICE=0 --define=LORA_DEVICE=0 --define=NBIOT_DEVICE=1 -g --c99 --float_operations_allowed=none --printf_support=nofloat --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 -z -m"TERMINAL_NBIOT_BC95G.map" --heap_size=8192 --stack_size=4096 --cinit_hold_wdt=on -i"C:/ti/ccsv8/ccs_base/msp430/include" -i"C:/ti/ccsv8/ccs_base/msp430/lib/5xx_6xx_FRxx" -i"C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/lib" -i"C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/include" --reread_libs --diag_wrap=off --display_error_number --warn_sections --xml_link_info="TERMINAL_NBIOT_BC95G_linkInfo.xml" --use_hw_mpy=F5 --rom_model -o "TERMINAL_NBIOT_BC95G.out" $(ORDERED_OBJS)
	@echo 'Finished building target: "$@"'
	@echo ' '

TERMINAL_NBIOT_BC95G.hex: $(EXE_OUTPUTS)
	@echo 'Building files: $(strip $(EXE_OUTPUTS__QUOTED))'
	@echo 'Invoking: MSP430 Hex Utility'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/bin/hex430" --memwidth=8 --romwidth=8 -o "TERMINAL_NBIOT_BC95G.hex" $(EXE_OUTPUTS__QUOTED)
	@echo 'Finished building: $(strip $(EXE_OUTPUTS__QUOTED))'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(BIN_OUTPUTS__QUOTED)$(EXE_OUTPUTS__QUOTED)
	-$(RM) "Source\app\alarm.obj" "Source\app\app.obj" "Source\app\flash.obj" "Source\app\meter.obj" "Source\app\metering.obj" "Source\app\nfc_app.obj" "Source\dev\battery.obj" "Source\dev\flash_msp430.obj" "Source\dev\lcd.obj" "Source\dev\nfc.obj" "Source\dev\rtc.obj" "Source\dev\timer.obj" "Source\dev\uart.obj" "Source\main.obj" "Source\misc\crc.obj" "Source\misc\misc.obj" "Source\misc\queue.obj" "Source\modem\nb\at_cmd.obj" "Source\modem\nb\message.obj" "Source\modem\nb\modem.obj" "Source\modem\nb\nbiot_bc95g.obj" "Source\modem\nb\uuid\md5.obj" "Source\modem\nb\uuid\uuid.obj" "Source\osal\osal.obj" "Source\osal\task_mgr.obj" "Source\test\modem_test.obj" "Source\test\shell.obj" "Source\test\test.obj" 
	-$(RM) "Source\app\alarm.d" "Source\app\app.d" "Source\app\flash.d" "Source\app\meter.d" "Source\app\metering.d" "Source\app\nfc_app.d" "Source\dev\battery.d" "Source\dev\flash_msp430.d" "Source\dev\lcd.d" "Source\dev\nfc.d" "Source\dev\rtc.d" "Source\dev\timer.d" "Source\dev\uart.d" "Source\main.d" "Source\misc\crc.d" "Source\misc\misc.d" "Source\misc\queue.d" "Source\modem\nb\at_cmd.d" "Source\modem\nb\message.d" "Source\modem\nb\modem.d" "Source\modem\nb\nbiot_bc95g.d" "Source\modem\nb\uuid\md5.d" "Source\modem\nb\uuid\uuid.d" "Source\osal\osal.d" "Source\osal\task_mgr.d" "Source\test\modem_test.d" "Source\test\shell.d" "Source\test\test.d" 
	-@echo 'Finished clean'
	-@echo ' '

secondary-outputs: $(BIN_OUTPUTS)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

