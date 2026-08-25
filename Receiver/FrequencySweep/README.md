# CC1101 frequency sweep

This standalone STM32 firmware scans 433.400 through 434.400 MHz in 10 kHz
steps. At every frequency it recalibrates the CC1101, waits for RX to settle,
averages 50 RSSI samples, and prints the average, minimum, and peak RSSI over
USART2 at 115200 baud. At the end it reports the frequency with the strongest
average RSSI.

The original receiver firmware is not replaced. This project reuses its
CubeMX-generated board support, HAL drivers, CC1101 SPI driver, startup file,
and linker script.

# What it does

Write 433.92 MHz registers
→ SIDLE
→ SCAL
→ wait 3 ms
→ SRX
→ wait 5 ms

Sweeps 433.400 - 434.400 MHz
Uses 10 kHz steps - 101 frequencies
Recalibrates the CC1101 at every step 
Averages 50 RSSI readings at every step
Reports average, minimum and peak RSSI
Prints a final BEST: Frequency


# Problems faced and Solutions

Previously, the code relied on the CC1101's automatic calibration. This sweep ensures that it is explicitly tuned before entering the RX and measuring the RSSI.

# Results:

BEST: 433.920 MHz with average RSSI -15.1 dBm
