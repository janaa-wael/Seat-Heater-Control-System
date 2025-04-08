################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
MCAL/GPTM/%.obj: ../MCAL/GPTM/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"D:/Program Files/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="D:/Courses/Advanced Diploma/RTOS/Workspace/Lab4" --include_path="D:/Courses/Advanced Diploma/RTOS/Workspace/Lab4/FreeRTOS/Source/include" --include_path="D:/Courses/Advanced Diploma/RTOS/Workspace/Lab4/FreeRTOS/Source/portable/CCS/ARM_CM4F" --include_path="D:/Courses/Advanced Diploma/RTOS/Workspace/Lab4/Common" --include_path="D:/Courses/Advanced Diploma/RTOS/Workspace/Lab4/MCAL" --include_path="D:/Courses/Advanced Diploma/RTOS/Workspace/Lab4/MCAL/GPIO" --include_path="D:/Courses/Advanced Diploma/RTOS/Workspace/Lab4/MCAL/UART" --include_path="D:/Courses/Advanced Diploma/RTOS/Workspace/Lab4/MCAL/GPTM" --include_path="D:/Courses/Advanced Diploma/RTOS/Workspace/Lab4/MCAL/ADC" --include_path="D:/Program Files/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/include" --define=ccs="ccs" --define=PART_TM4C123GH6PM -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="MCAL/GPTM/$(basename $(<F)).d_raw" --obj_directory="MCAL/GPTM" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


