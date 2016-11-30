# MCU=mk66fx1m0		#	Teensy 3.6
# MCU=mk64fx512		#	Teensy 3.5
 MCU=mk20dx256		#	Teensy 3.2 & 3.1
# MCU=mk20dx128		#	Teensy 3.0
# MCU=mkl26z64 		#   Teensy LC
# MCU=at90usb1286 	#	Teensy++ 2.0
# MCU=atmega32u4 		#	Teensy 2.0
# MCU=at90usb646 		#	Teensy++ 1.0
# MCU=at90usb162 		#	Teensy 1.0

blink_test:
	teensy_loader_cli -mmcu=$(MCU) -w blink_both/blink_fast_Teensy32.hex
