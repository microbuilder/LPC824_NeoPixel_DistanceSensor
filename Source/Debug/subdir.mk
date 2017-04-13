################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../aeabi_romdiv_patch.s 

C_SRCS += \
../cr_startup_lpc82x.c \
../crp.c \
../main.c \
../mtb.c 

OBJS += \
./aeabi_romdiv_patch.o \
./cr_startup_lpc82x.o \
./crp.o \
./main.o \
./mtb.o 

C_DEPS += \
./cr_startup_lpc82x.d \
./crp.d \
./main.d \
./mtb.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.s
	@echo 'Building file: $<'
	@echo 'Invoking: MCU Assembler'
	arm-none-eabi-gcc -c -x assembler-with-cpp -DDEBUG -D__CODE_RED -DCORE_M0PLUS -D__USE_ROMDIVIDE -D__LPC82X__ -D__REDLIB__ -I"/Users/ktown/Documents/LPCXpresso_8.2.2/workspace/lpc824_neopixel/common/inc" -g3 -mcpu=cortex-m0 -mthumb -specs=redlib.specs -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -DDEBUG -D__CODE_RED -DCORE_M0PLUS -D__MTB_DISABLE -D__MTB_BUFFER_SIZE=0 -D__USE_ROMDIVIDE -D__LPC82X__ -D__REDLIB__ -I"/Users/ktown/Documents/LPCXpresso_8.2.2/workspace/lpc824_neopixel/common/inc" -I"/Users/ktown/Documents/LPCXpresso_8.2.2/workspace/common/inc" -I"/Users/ktown/Documents/LPCXpresso_8.2.2/workspace/peripherals_lib/inc" -I"/Users/ktown/Documents/LPCXpresso_8.2.2/workspace/utilities_lib/inc" -O0 -fno-common -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=cortex-m0 -mthumb -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


