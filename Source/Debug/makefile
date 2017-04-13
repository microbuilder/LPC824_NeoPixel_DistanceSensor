################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: lpc824_neopixel.axf

# Tool invocations
lpc824_neopixel.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"/Users/ktown/Documents/LPCXpresso_8.2.2/workspace/utilities_lib/flash" -L"/Users/ktown/Documents/LPCXpresso_8.2.2/workspace/peripherals_lib/flash" -Xlinker -Map="lpc824_neopixel.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -mcpu=cortex-m0 -mthumb -T "lpc824_neopixel_Debug.ld" -o "lpc824_neopixel.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) lpc824_neopixel.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "lpc824_neopixel.axf"; # arm-none-eabi-objcopy -v -O binary "lpc824_neopixel.axf" "lpc824_neopixel.bin" ; # checksum -p LPC824 -d "lpc824_neopixel.bin";
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
