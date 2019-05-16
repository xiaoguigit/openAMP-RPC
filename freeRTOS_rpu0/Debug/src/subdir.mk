################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
LD_SRCS += \
../src/lscript.ld 

C_SRCS += \
../src/helper.c \
../src/main.c \
../src/platform_info.c \
../src/rsc_table.c 

OBJS += \
./src/helper.o \
./src/main.o \
./src/platform_info.o \
./src/rsc_table.o 

C_DEPS += \
./src/helper.d \
./src/main.d \
./src/platform_info.d \
./src/rsc_table.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM R5 gcc compiler'
	armr5-none-eabi-gcc -DARMR5 -Wall -O0 -g3 -I../../rpu0_freertos_bsp/psu_cortexr5_0/include -c -fmessage-length=0 -MT"$@" -mcpu=cortex-r5 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


