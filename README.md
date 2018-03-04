# pi-buttons

A low level gpio service that generates button events on a Unix socket for a
specified list of GPIO inputs connected to buttons.

## build

The src directory includes the source code and a Makefile. Enter this directory
and type *make* to build the service.


## install

In the src directory type *sudo make install* to install the executable.


## install service

In the src directory type *sudo make install_service* to install the systemd service.


## configure

After installing the service edit the /etc/pi-buttons.conf file with your button
gpio values, a unix socket path, and button timing values.


## run service

To start the service type *sudo systemctl enable pi-buttons.service* followed with
*sudo systemctl start pi-buttons.service*.
