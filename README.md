# Anemo embedded
Application intended for wind energy, and other environmental data monitoring. It cyclically shedules tasks, where each task is connected to a module representing a peripheral measuring device. Once the measurement time period (MTP) has elapsed, the application processes the data and relays it via UART. The process ten repeats itself.


## Configuration

#### sys_config.h

The use of different measuring equipment, and their corresponding modules, can be configured by changing the value of `SYS_CONFING`. Likewise, the `DATA_FORMATER` must be changed to match the number of modules in use, with the exception of the serial module. When using an anemometer, the user needs to set the `NORTH_OFFSET_10E1` to the amount of degrees the anemometer is offset from magnetic north, multiplied by 10. Lastly, the MTP can be changed to a desired amount of minutes by modifying `DATA_SEND_PERIOD_MIN`.

*The `ATIMER_CALIBRATION` lets the user compensate for a potential timer offset, but is still in the experimental stages.*


#### Makefile
Similarly to the `sys_config.h` file, the use of different modules needs to be set in the `Makefile`. The `BOARD` and its corresponding `BOARD_NUMBER` need to be set with regard to the equipment in use. If the support for the board hasn't been added yet, the pin configuration of another board can be used, or a new set of pin configurations can be defined for the specific board in `pin_settings.h`.

#### hash.h
 Lastly, a new file, bearing the device's hash string, needs to be generated. Its contents should resemble the following:
```
#define DEVICE_HASH_LEN		16+1
#define DEVICE_HASH		"xxxxxxxxxxxxxxxx"
```


## RIOT-OS Modifications
 Since the Anemo-embedded application relies on an internal cyclic timer interrupt, minor modifications were made to operating system. The procedure may depend from device to device, and as an example, the procedure take for the SAMD21 MCU is describe bellow:

- Open timer definition
```
vim $RIOT/cpu/samd21/periph/timer.c
```

- In "timer_init()", prescale the clock to 15625 kHz
```
//TIMER_0_DEV.CTRLA.bit.PRESCALER = TC_CTRLA_PRESCALER_DIV1_Val;
TIMER_0_DEV.CTRLA.bit.PRESCALER = TC_CTRLA_PRESCALER_DIV64_Val;
```

- In "timer_init()", choose match frequency operation
```
//TIMER_0_DEV.CTRLA.bit.WAVEGEN = TC_CTRLA_WAVEGEN_NFRQ_Val
TIMER_0_DEV.CTRLA.bit.WAVEGEN = TC_CTRLA_WAVEGEN_MFRQ_Val;
```

- In "TIMER_0_ISR()", do not disable match/capture interrupt
```
//TIMER_0_DEV.INTENCLR.reg = TC_INTENCLR_MC0;
```


## Run
Having successfully configured everything and connected the desired sensor modules, make, flash and monitor the application:
```
cd $ANEMO_EMBEDDED
make all PORT=/dev/ttyACM0 flash term
```


## Further reading
To get an idea of how to install a fully functional device, refer to [Alexander's thesis](https://researchgate.net/profile/Alexander_Marinsek), and check out the [Anemo cloud platform](https://anemo.si) where you can view data from other devices.


## Notice
For usage and distribution limitations, please refer to the licensing agreemen, contained in the file `LICENSE`.

