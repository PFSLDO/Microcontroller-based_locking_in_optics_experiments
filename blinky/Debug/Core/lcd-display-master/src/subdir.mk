################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/lcd-display-master/src/lcdDisplay.c 

OBJS += \
./Core/lcd-display-master/src/lcdDisplay.o 

C_DEPS += \
./Core/lcd-display-master/src/lcdDisplay.d 


# Each subdirectory must supply rules for building sources it contributes
Core/lcd-display-master/src/%.o Core/lcd-display-master/src/%.su Core/lcd-display-master/src/%.cyclo: ../Core/lcd-display-master/src/%.c Core/lcd-display-master/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L412xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I"/Users/pfsldo/Programacao/Microcontroller-based_locking_in_optics_experiments/blinky/Core/lcd-display-master/src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-lcd-2d-display-2d-master-2f-src

clean-Core-2f-lcd-2d-display-2d-master-2f-src:
	-$(RM) ./Core/lcd-display-master/src/lcdDisplay.cyclo ./Core/lcd-display-master/src/lcdDisplay.d ./Core/lcd-display-master/src/lcdDisplay.o ./Core/lcd-display-master/src/lcdDisplay.su

.PHONY: clean-Core-2f-lcd-2d-display-2d-master-2f-src

