################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
Source/test/modem_test.obj: C:/Users/User/workspace_v8/terminal/Source/test/modem_test.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/bin/cl430" -vmspx --data_model=small -O0 --opt_for_speed=0 --use_hw_mpy=F5 --include_path="C:/ti/ccsv8/ccs_base/msp430/include" --include_path="C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/include" --include_path="C:/Users/User/workspace_v8/terminal/Source" --include_path="C:/Users/User/workspace_v8/terminal/Source/app" --include_path="C:/Users/User/workspace_v8/terminal/Source/dev" --include_path="C:/Users/User/workspace_v8/terminal/Source/misc" --include_path="C:/Users/User/workspace_v8/terminal/Source/osal" --include_path="C:/Users/User/workspace_v8/terminal/Source/test" --include_path="C:/Users/User/workspace_v8/terminal/Source/modem/nb" --include_path="C:/Users/User/workspace_v8/terminal/Source/modem/nb/uuid" --advice:power="1,3,4,7,9" --define=__MSP430F5419A__ --define=__MODEM_BC95G__ --define=DEBUG --define=REPEATER_DEVICE=0 --define=LORA_DEVICE=0 --define=NBIOT_DEVICE=1 -g --c99 --float_operations_allowed=none --printf_support=nofloat --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="Source/test/$(basename $(<F)).d_raw" --obj_directory="Source/test" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

Source/test/shell.obj: C:/Users/User/workspace_v8/terminal/Source/test/shell.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/bin/cl430" -vmspx --data_model=small -O0 --opt_for_speed=0 --use_hw_mpy=F5 --include_path="C:/ti/ccsv8/ccs_base/msp430/include" --include_path="C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/include" --include_path="C:/Users/User/workspace_v8/terminal/Source" --include_path="C:/Users/User/workspace_v8/terminal/Source/app" --include_path="C:/Users/User/workspace_v8/terminal/Source/dev" --include_path="C:/Users/User/workspace_v8/terminal/Source/misc" --include_path="C:/Users/User/workspace_v8/terminal/Source/osal" --include_path="C:/Users/User/workspace_v8/terminal/Source/test" --include_path="C:/Users/User/workspace_v8/terminal/Source/modem/nb" --include_path="C:/Users/User/workspace_v8/terminal/Source/modem/nb/uuid" --advice:power="1,3,4,7,9" --define=__MSP430F5419A__ --define=__MODEM_BC95G__ --define=DEBUG --define=REPEATER_DEVICE=0 --define=LORA_DEVICE=0 --define=NBIOT_DEVICE=1 -g --c99 --float_operations_allowed=none --printf_support=nofloat --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="Source/test/$(basename $(<F)).d_raw" --obj_directory="Source/test" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

Source/test/test.obj: C:/Users/User/workspace_v8/terminal/Source/test/test.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/bin/cl430" -vmspx --data_model=small -O0 --opt_for_speed=0 --use_hw_mpy=F5 --include_path="C:/ti/ccsv8/ccs_base/msp430/include" --include_path="C:/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS/include" --include_path="C:/Users/User/workspace_v8/terminal/Source" --include_path="C:/Users/User/workspace_v8/terminal/Source/app" --include_path="C:/Users/User/workspace_v8/terminal/Source/dev" --include_path="C:/Users/User/workspace_v8/terminal/Source/misc" --include_path="C:/Users/User/workspace_v8/terminal/Source/osal" --include_path="C:/Users/User/workspace_v8/terminal/Source/test" --include_path="C:/Users/User/workspace_v8/terminal/Source/modem/nb" --include_path="C:/Users/User/workspace_v8/terminal/Source/modem/nb/uuid" --advice:power="1,3,4,7,9" --define=__MSP430F5419A__ --define=__MODEM_BC95G__ --define=DEBUG --define=REPEATER_DEVICE=0 --define=LORA_DEVICE=0 --define=NBIOT_DEVICE=1 -g --c99 --float_operations_allowed=none --printf_support=nofloat --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="Source/test/$(basename $(<F)).d_raw" --obj_directory="Source/test" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


