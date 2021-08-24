################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../Core/Startup/startup_stm32f103c8tx.s 

OBJS += \
./Core/Startup/startup_stm32f103c8tx.o 

S_DEPS += \
./Core/Startup/startup_stm32f103c8tx.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Startup/startup_stm32f103c8tx.o: ../Core/Startup/startup_stm32f103c8tx.s
	arm-none-eabi-gcc -mcpu=cortex-m3 -g3 -c -I"D:/BACKUP/Faculdade/16_Embarcados/Controlador_Temperatura/Controle-de-temperatura/Codigos/controle_temperatura_RTOS/Middlewares/Third_Party/SEGGER/Config" -x assembler-with-cpp -MMD -MP -MF"Core/Startup/startup_stm32f103c8tx.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@" "$<"

