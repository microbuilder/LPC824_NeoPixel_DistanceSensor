Tool chain versions supported
=============================
- This package was built using LPCXpresso v8.2.0_647



Steps for building this workspace when it's clean
=================================================
1. Click 'Build all projects [flash]' in the Quickstart Panel.



Note about using the debug COM port (a.k.a. the MBED serial port)
=================================================================
Most projects in this package are interactive and communicate with the user
via a terminal emulator running on the host PC, connected either to the MBED
serial port, or to a USB-to-UART cable connected to pins on the Arduino header.

Some OM13071 boards have UART RXD connected to the Arduino site by default,
and need solder jumper SJ9 changed in order to use the MBED serial port for
UART communication with the terminal emulator.

It is recommended to run the project Example_UART0_Terminal first, in order to
establish that communication between the terminal emulator (via the MBED
serial port and/or the USB COM port with USB-to-UART cable) and the LPC8xx are
working.

* To use the MBED Serial Port:
    1. No external connections are necessary.
    2. The Max board must have the necessary solder-bump jumper modifications:
      A. For LPC812 Max, short pins 1 and 2 of both SJ1 and SJ4
      B. For LPC824 Max, short pins 2 and 3 of SJ9
    3. The Windows MBED Serial Port Driver must be installed on the PC.
       https://developer.mbed.org/handbook/Windows-serial-configuration
    4. The terminal emulator can then be connected to the MBED serial port which
       enumerates when the board is connected. It appears something like this: 
       COM13: mbed Serial Port (COM13)
    5. Uncomment the appropriate ConfigSWM() calls in the beginning of main()
       or in setup_debug_uart() in Serial.c as appropriate for the project.

* To use a USB-to-RS232 breakout cable:
    1. There are three external connections necessary: RXD and TXD based on the SWM
       settings, plus a ground connection.
       U0_TXD = breakout cable RXD
       U0_RXD = breakout cable TXD
       Board GND = breakout cable GND
    2. Uncomment the appropriate ConfigSWM() calls in the beginning of main()
       or in setup_debug_uart() in Serial.c as appropriate for the project.




