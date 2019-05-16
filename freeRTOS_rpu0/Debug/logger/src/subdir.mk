################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../logger/src/elog.c \
../logger/src/elog_buf.c \
../logger/src/elog_utils.c \
../logger/src/logger.c 

OBJS += \
./logger/src/elog.o \
./logger/src/elog_buf.o \
./logger/src/elog_utils.o \
./logger/src/logger.o 

C_DEPS += \
./logger/src/elog.d \
./logger/src/elog_buf.d \
./logger/src/elog_utils.d \
./logger/src/logger.d 


# Each subdirectory must supply rules for building sources it contributes
logger/src/%.o: ../logger/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM R5 gcc compiler'
	armr5-none-eabi-gcc -DARMR5 -Wall -O0 -g3 -I../../rpu0_freertos_bsp/psu_cortexr5_0/include -c -fmessage-length=0 -MT"$@" -mcpu=cortex-r5 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


