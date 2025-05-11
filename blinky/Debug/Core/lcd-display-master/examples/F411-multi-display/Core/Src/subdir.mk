################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/lcd-display-master/examples/F411-multi-display/Core/Src/main.c \
../Core/lcd-display-master/examples/F411-multi-display/Core/Src/stm32f4xx_hal_msp.c \
../Core/lcd-display-master/examples/F411-multi-display/Core/Src/stm32f4xx_it.c \
../Core/lcd-display-master/examples/F411-multi-display/Core/Src/syscalls.c \
../Core/lcd-display-master/examples/F411-multi-display/Core/Src/sysmem.c \
../Core/lcd-display-master/examples/F411-multi-display/Core/Src/system_stm32f4xx.c 

OBJS += \
./Core/lcd-display-master/examples/F411-multi-display/Core/Src/main.o \
./Core/lcd-display-master/examples/F411-multi-display/Core/Src/stm32f4xx_hal_msp.o \
./Core/lcd-display-master/examples/F411-multi-display/Core/Src/stm32f4xx_it.o \
./Core/lcd-display-master/examples/F411-multi-display/Core/Src/syscalls.o \
./Core/lcd-display-master/examples/F411-multi-display/Core/Src/sysmem.o \
./Core/lcd-display-master/examples/F411-multi-display/Core/Src/system_stm32f4xx.o 

C_DEPS += \
./Core/lcd-display-master/examples/F411-multi-display/Core/Src/main.d \
./Core/lcd-display-master/examples/F411-multi-display/Core/Src/stm32f4xx_hal_msp.d \
./Core/lcd-display-master/examples/F411-multi-display/Core/Src/stm32f4xx_it.d \
./Core/lcd-display-master/examples/F411-multi-display/Core/Src/syscalls.d \
./Core/lcd-display-master/examples/F411-multi-display/Core/Src/sysmem.d \
./Core/lcd-display-master/examples/F411-multi-display/Core/Src/system_stm32f4xx.d 


# Each subdirectory must supply rules for building sources it contributes
Core/lcd-display-master/examples/F411-multi-display/Core/Src/%.o Core/lcd-display-master/examples/F411-multi-display/Core/Src/%.su Core/lcd-display-master/examples/F411-multi-display/Core/Src/%.cyclo: ../Core/lcd-display-master/examples/F411-multi-display/Core/Src/%.c Core/lcd-display-master/examples/F411-multi-display/Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L412xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I"/Users/pfsldo/Programacao/Microcontroller-based_locking_in_optics_experiments/blinky/Core/lcd-display-master" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-lcd-2d-display-2d-master-2f-examples-2f-F411-2d-multi-2d-display-2f-Core-2f-Src

clean-Core-2f-lcd-2d-display-2d-master-2f-examples-2f-F411-2d-multi-2d-display-2f-Core-2f-Src:
	-$(RM) ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/main.cyclo ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/main.d ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/main.o ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/main.su ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/stm32f4xx_hal_msp.cyclo ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/stm32f4xx_hal_msp.d ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/stm32f4xx_hal_msp.o ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/stm32f4xx_hal_msp.su ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/stm32f4xx_it.cyclo ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/stm32f4xx_it.d ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/stm32f4xx_it.o ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/stm32f4xx_it.su ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/syscalls.cyclo ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/syscalls.d ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/syscalls.o ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/syscalls.su ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/sysmem.cyclo ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/sysmem.d ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/sysmem.o ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/sysmem.su ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/system_stm32f4xx.cyclo ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/system_stm32f4xx.d ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/system_stm32f4xx.o ./Core/lcd-display-master/examples/F411-multi-display/Core/Src/system_stm32f4xx.su

.PHONY: clean-Core-2f-lcd-2d-display-2d-master-2f-examples-2f-F411-2d-multi-2d-display-2f-Core-2f-Src

