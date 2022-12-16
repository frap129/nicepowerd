# AC_PATH changes between devices. Replace the value with the correct
# /sys/class/power_supply/<some directory>/online for your system.
AC_PATH="/sys/class/power_supply/AC/online"

# Charge thresholds for low power/performance profiles
BAT_LOW_THRESH=20
BAT_HIGH_THRESH=90
