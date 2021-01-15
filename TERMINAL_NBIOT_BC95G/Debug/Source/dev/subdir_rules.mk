################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
Source/dev/battery.obj: D:/terminal/Source/dev/battery.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/bin/cl430" -vmspx --data_model=small -O0 --opt_for_speed=0 --use_hw_mpy=F5 --include_path="C:/ti/ccsv8/ccs_base/msp430/include" --include_path="C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/include" --include_path="D:/terminal/Source" --include_path="D:/terminal/Source/app" --include_path="D:/terminal/Source/dev" --include_path="D:/terminal/Source/misc" --include_path="D:/terminal/Source/osal" --include_path="D:/terminal/Source/test" --include_path="D:/terminal/Source/modem/nb" --include_path="D:/terminal/Source/modem/nb/uuid" --advice:power="1,3,4,7,9" --define=__MSP430F5419A__ --define=__MODEM_BC95G__ --define=DEBUG --define=REPEATER_DEVICE=0 --define=LORA_DEVICE=0 --define=NBIOT_DEVICE=1 -g --c99 --float_operations_allowed=none --printf_support=nofloat --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="Source/dev/$(basename $(<F)).d_raw" --obj_directory="Source/dev" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

Source/dev/flash_msp430.obj: D:/terminal/Source/dev/flash_msp430.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/bin/cl430" -vmspx --data_model=small -O0 --opt_for_speed=0 --use_hw_mpy=F5 --include_path="C:/ti/ccsv8/ccs_base/msp430/include" --include_path="C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/include" --include_path="D:/terminal/Source" --include_path="D:/terminal/Source/app" --include_path="D:/terminal/Source/dev" --include_path="D:/terminal/Source/misc" --include_path="D:/terminal/Source/osal" --include_path="D:/terminal/Source/test" --include_path="D:/terminal/Source/modem/nb" --include_path="D:/terminal/Source/modem/nb/uuid" --advice:power="1,3,4,7,9" --define=__MSP430F5419A__ --define=__MODEM_BC95G__ --define=DEBUG --define=REPEATER_DEVICE=0 --define=LORA_DEVICE=0 --define=NBIOT_DEVICE=1 -g --c99 --float_operations_allowed=none --printf_support=nofloat --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="Source/dev/$(basename $(<F)).d_raw" --obj_directory="Source/dev" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

Source/dev/lcd.obj: D:/terminal/Source/dev/lcd.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/bin/cl430" -vmspx --data_model=small -O0 --opt_for_speed=0 --use_hw_mpy=F5 --include_path="C:/ti/ccsv8/ccs_base/msp430/include" --include_path="C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/include" --include_path="D:/terminal/Source" --include_path="D:/terminal/Source/app" --include_path="D:/terminal/Source/dev" --include_path="D:/terminal/Source/misc" --include_path="D:/terminal/Source/osal" --include_path="D:/terminal/Source/test" --include_path="D:/terminal/Source/modem/nb" --include_path="D:/terminal/Source/modem/nb/uuid" --advice:power="1,3,4,7,9" --define=__MSP430F5419A__ --define=__MODEM_BC95G__ --define=DEBUG --define=REPEATER_DEVICE=0 --define=LORA_DEVICE=0 --define=NBIOT_DEVICE=1 -g --c99 --float_operations_allowed=none --printf_support=nofloat --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="Source/dev/$(basename $(<F)).d_raw" --obj_directory="Source/dev" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

Source/dev/nfc.obj: D:/terminal/Source/dev/nfc.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/bin/cl430" -vmspx --data_model=small -O0 --opt_for_speed=0 --use_hw_mpy=F5 --include_path="C:/ti/ccsv8/ccs_base/msp430/include" --include_path="C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/include" --include_path="D:/terminal/Source" --include_path="D:/terminal/Source/app" --include_path="D:/terminal/Source/dev" --include_path="D:/terminal/Source/misc" --include_path="D:/terminal/Source/osal" --include_path="D:/terminal/Source/test" --include_path="D:/terminal/Source/modem/nb" --include_path="D:/terminal/Source/modem/nb/uuid" --advice:power="1,3,4,7,9" --define=__MSP430F5419A__ --define=__MODEM_BC95G__ --define=DEBUG --define=REPEATER_DEVICE=0 --define=LORA_DEVICE=0 --define=NBIOT_DEVICE=1 -g --c99 --float_operations_allowed=none --printf_support=nofloat --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="Source/dev/$(basename $(<F)).d_raw" --obj_directory="Source/dev" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

Source/dev/rtc.obj: D:/terminal/Source/dev/rtc.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/bin/cl430" -vmspx --data_model=small -O0 --opt_for_speed=0 --use_hw_mpy=F5 --include_path="C:/ti/ccsv8/ccs_base/msp430/include" --include_path="C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/include" --include_path="D:/terminal/Source" --include_path="D:/terminal/Source/app" --include_path="D:/terminal/Source/dev" --include_path="D:/terminal/Source/misc" --include_path="D:/terminal/Source/osal" --include_path="D:/terminal/Source/test" --include_path="D:/terminal/Source/modem/nb" --include_path="D:/terminal/Source/modem/nb/uuid" --advice:power="1,3,4,7,9" --define=__MSP430F5419A__ --define=__MODEM_BC95G__ --define=DEBUG --define=REPEATER_DEVICE=0 --define=LORA_DEVICE=0 --define=NBIOT_DEVICE=1 -g --c99 --float_operations_allowed=none --printf_support=nofloat --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="Source/dev/$(basename $(<F)).d_raw" --obj_directory="Source/dev" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

Source/dev/timer.obj: D:/terminal/Source/dev/timer.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/bin/cl430" -vmspx --data_model=small -O0 --opt_for_speed=0 --use_hw_mpy=F5 --include_path="C:/ti/ccsv8/ccs_base/msp430/include" --include_path="C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/include" --include_path="D:/terminal/Source" --include_path="D:/terminal/Source/app" --include_path="D:/terminal/Source/dev" --include_path="D:/terminal/Source/misc" --include_path="D:/terminal/Source/osal" --include_path="D:/terminal/Source/test" --include_path="D:/terminal/Source/modem/nb" --include_path="D:/terminal/Source/modem/nb/uuid" --advice:power="1,3,4,7,9" --define=__MSP430F5419A__ --define=__MODEM_BC95G__ --define=DEBUG --define=REPEATER_DEVICE=0 --define=LORA_DEVICE=0 --define=NBIOT_DEVICE=1 -g --c99 --float_operations_allowed=none --printf_support=nofloat --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="Source/dev/$(basename $(<F)).d_raw" --obj_directory="Source/dev" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

Source/dev/uart.obj: D:/terminal/Source/dev/uart.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/bin/cl430" -vmspx --data_model=small -O0 --opt_for_speed=0 --use_hw_mpy=F5 --include_path="C:/ti/ccsv8/ccs_base/msp430/include" --include_path="C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/include" --include_path="D:/terminal/Source" --include_path="D:/terminal/Source/app" --include_path="D:/terminal/Source/dev" --include_path="D:/terminal/Source/misc" --include_path="D:/terminal/Source/osal" --include_path="D:/terminal/Source/test" --include_path="D:/terminal/Source/modem/nb" --include_path="D:/terminal/Source/modem/nb/uuid" --advice:power="1,3,4,7,9" --define=__MSP430F5419A__ --define=__MODEM_BC95G__ --define=DEBUG --define=REPEATER_DEVICE=0 --define=LORA_DEVICE=0 --define=NBIOT_DEVICE=1 -g --c99 --float_operations_allowed=none --printf_support=nofloat --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="Source/dev/$(basename $(<F)).d_raw" --obj_directory="Source/dev" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


