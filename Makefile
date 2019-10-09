APPLICATION = Anemo


# BOARDS:
#	0: rhomb-zero
#	1: samd21-xpro
#	2: arduino-due

BOARD ?= samd21-xpro
BOARD_NUMBER ?= 1


USEMODULE += xtimer
FEATURES_REQUIRED += periph_timer
FEATURES_REQUIRED += periph_adc
FEATURES_REQUIRED += periph_uart


DIRS += anemo_davis
USEMODULE += anemo_davis

DIRS += wind_data
USEMODULE += wind_data

DIRS += env_data
USEMODULE += env_data
USEMODULE += bme280

DIRS += el_data
USEMODULE += el_data
USEMODULE += ina220

DIRS += serial_data
USEMODULE += serial_data

DIRS += tasks
USEMODULE += tasks


# Share board info with app
CFLAGS += -DBOARD=\"$(BOARD)\"		# Convert to string
CFLAGS += -DBOARD_NUMBER=$(BOARD_NUMBER)


RIOTBASE ?= $(CURDIR)/../../RIOT-custom
include $(RIOTBASE)/Makefile.include

