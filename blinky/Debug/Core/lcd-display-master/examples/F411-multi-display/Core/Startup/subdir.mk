################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../Core/lcd-display-master/examples/F411-multi-display/Core/Startup/startup_stm32f411ceux.s 

OBJS += \
./Core/lcd-display-master/examples/F411-multi-display/Core/Startup/startup_stm32f411ceux.o 

S_DEPS += \
./Core/lcd-display-master/examples/F411-multi-display/Core/Startup/startup_stm32f411ceux.d 


# Each subdirectory must supply rules for building sources it contributes
Core/lcd-display-master/examples/F411-multi-display/Core/Startup/%.o: ../Core/lcd-display-master/examples/F411-multi-display/Core/Startup/%.s Core/lcd-display-master/examples/F411-multi-display/Core/Startup/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m4 -g3 -DDEBUG -c -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"

clean: clean-Core-2f-lcd-2d-display-2d-master-2f-examples-2f-F411-2d-multi-2d-display-2f-Core-2f-Startup

clean-Core-2f-lcd-2d-display-2d-master-2f-examples-2f-F411-2d-multi-2d-display-2f-Core-2f-Startup:
	-$(RM) ./Core/lcd-display-master/examples/F411-multi-display/Core/Startup/startup_stm32f411ceux.d ./Core/lcd-display-master/examples/F411-multi-display/Core/Startup/startup_stm32f411ceux.o

.PHONY: clean-Core-2f-lcd-2d-display-2d-master-2f-examples-2f-F411-2d-multi-2d-display-2f-Core-2f-Startup

