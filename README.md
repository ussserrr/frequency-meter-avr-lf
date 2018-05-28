## Overview
Frequency meter firmware for ATmega328P MCU. It uses 16-bit Timer/Counter1 (TC1) as a main measuring part of the MCU. Can be used as a standalone solution (no need in PC - LCD is used for indication). See [frequency-meter-avr-hf](https://github.com/ussserrr/frequency-meter-avr-hf) for measuring an HF band.

```
     __________________________                                     ____
    /▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉\ ▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉/
   / ▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉ \▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉  ▉▉
__/__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉__▉▉_____
      1   2   3   4   5   6   7   8   9  10  11  12  13  14  15 /16
start on                           \                           /
rising edge                         \                         /
                                     -------------------------
```
Configured in "Input capture" mode, TC1 stores a number of internally generated pulses (with known timings) that had been counted during the one period of input signal (filling frequency), so we can find out its frequency as
```
             number_of_counted_ticks
frequency = -------------------------
                filling_frequency
```
This method is especially good for LF band as the accuracy there is a parameter of
```
accuracy ~ N_pulses * period
```
value, so to improve it we should whether increase the internal frequency of TC1 (which we can't do in general) or increase the period of the measured signal (i.e. measure LF bands). We also use an averaging to get more accurate results.

The algorithm also detects when input frequency is rapidly changes (e.g. you tune it) and increases refresh rate of LCD indicating.


## Pinout
Pin | Function
--- | --------
PB0 (Arduino's pin 8) | Input signal
Arduino's 0, 1, 2, 3, 4, 6, 7 pins (respective PDx pins) | RS, RW, E, D4-7


## Build and run
Build and run via [PlatformIO](https://platformio.org/). In `platformio.ini` specify parameters (e.g. F_CPU or programmer). Then run:
```bash
$ pio run  # build

$ pio run -t program  # flash using external USBASP programmer
or
$ pio run -t upload  # flash using on-board programmer
```


## Limits and accuracy
Check this [spreadsheet](https://docs.google.com/spreadsheets/d/1x5buIiSePPuyIJX-X4MWf-NA7JHJYz23IA_RD0JLFfs/edit?usp=sharing) to see some test measurements (second page). Generally, this method gives the most accurate results for input frequencies below 1 kHz. HF algorithm is more suited for all frequencies above that value.

Also, you can replace crystal oscillator with another one (and even overclock a little bit) to increase maximal measurable frequency. In this case, remember to adjust F_CPU, Timer 0 and 1 clock sources for correct calculations and `board_build.f_cpu` in `platformio.ini` file.

While TC1 is simply triggered by voltage level (rising or falling edge), MCU is able to measure not only strict square signals - sin wave, saw, triangle forms are also can be correctly recognized within certain limits of distortions.
