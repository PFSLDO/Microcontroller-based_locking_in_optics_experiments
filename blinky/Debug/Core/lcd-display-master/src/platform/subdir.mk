################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/lcd-display-master/src/platform/platform_blank.c \
../Core/lcd-display-master/src/platform/platform_stm32f1.c \
../Core/lcd-display-master/src/platform/platform_stm32f4.c 

OBJS += \
./Core/lcd-display-master/src/platform/platform_blank.o \
./Core/lcd-display-master/src/platform/platform_stm32f1.o \
./Core/lcd-display-master/src/platform/platform_stm32f4.o 

C_DEPS += \
./Core/lcd-display-master/src/platform/platform_blank.d \
./Core/lcd-display-master/src/platform/platform_stm32f1.d \
./Core/lcd-display-master/src/platform/platform_stm32f4.d 


# Each subdirectory must supply rules for building sources it contributes
Core/lcd-display-master/src/platform/%.o Core/lcd-display-master/src/platform/%.su Core/lcd-display-master/src/platform/%.cyclo: ../Core/lcd-display-master/src/platform/%.c Core/lcd-display-master/src/platform/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L412xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I"/Users/pfsldo/Programacao/Microcontroller-based_locking_in_optics_experiments/blinky/Core/lcd-display-master" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-lcd-2d-display-2d-master-2f-src-2f-platform

clean-Core-2f-lcd-2d-display-2d-master-2f-src-2f-platform:
	-$(RM) ./Core/lcd-display-master/src/platform/platform_blank.cyclo ./Core/lcd-display-master/src/platform/platform_blank.d ./Core/lcd-display-master/src/platform/platform_blank.o ./Core/lcd-display-master/src/platform/platform_blank.su ./Core/lcd-display-master/src/platform/platform_stm32f1.cyclo ./Core/lcd-display-master/src/platform/platform_stm32f1.d ./Core/lcd-display-master/src/platform/platform_stm32f1.o ./Core/lcd-display-master/src/platform/platform_stm32f1.su ./Core/lcd-display-master/src/platform/platform_stm32f4.cyclo ./Core/lcd-display-master/src/platform/platform_stm32f4.d ./Core/lcd-display-master/src/platform/platform_stm32f4.o ./Core/lcd-display-master/src/platform/platform_stm32f4.su

.PHONY: clean-Core-2f-lcd-2d-display-2d-master-2f-src-2f-platform

