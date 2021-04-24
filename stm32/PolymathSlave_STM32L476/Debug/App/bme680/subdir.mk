################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/bme680/bm680_helper.c \
../App/bme680/bme680.c 

OBJS += \
./App/bme680/bm680_helper.o \
./App/bme680/bme680.o 

C_DEPS += \
./App/bme680/bm680_helper.d \
./App/bme680/bme680.d 


# Each subdirectory must supply rules for building sources it contributes
App/bme680/bm680_helper.o: ../App/bme680/bm680_helper.c App/bme680/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L476xx -c -I"C:/Work/Projects/Quickfeather/Software/repo/slave_controller_software/PolymathSlave_STM32L476/App" -I"C:/Work/Projects/Quickfeather/Software/repo/slave_controller_software/PolymathSlave_STM32L476/App/bme680" -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"App/bme680/bm680_helper.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
App/bme680/bme680.o: ../App/bme680/bme680.c App/bme680/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L476xx -c -I"C:/Work/Projects/Quickfeather/Software/repo/slave_controller_software/PolymathSlave_STM32L476/App" -I"C:/Work/Projects/Quickfeather/Software/repo/slave_controller_software/PolymathSlave_STM32L476/App/bme680" -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"App/bme680/bme680.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

