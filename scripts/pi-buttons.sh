#!/bin/bash

source /etc/pi-buttons.conf

# setup sys gpio
setup() {
  for G in $(echo $GPIOS | sed "s/,/ /g")
  do
    # make sure GPIO is exported
    if [ ! -d /sys/class/gpio/gpio$G ]; then
      echo $G > /sys/class/gpio/export
    fi

    sleep 1

    echo "in" > /sys/class/gpio/gpio$G/direction
    echo "both" > /sys/class/gpio/gpio$G/edge
  done
}

setup

CMD="pi-buttons -g $GPIOS -s $SOCK -t $TS"
$CMD
