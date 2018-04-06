# pi-buttons

A low level service that generates button events on a Unix socket for a
specified list of GPIO inputs connected to buttons.


## build

The src directory includes the source code and a Makefile. Enter this directory
and type *make* to build the service.


## install

In the src directory type *sudo make install* to install the pi-buttons executable.

To remove the pi-buttons executable type *sudo make uninstall*.


## install service

The project includes scripts and files needed to enable pi-buttons as a systemd
service. To install the service files type *sudo make install_service* and the
files for systemd operation will be installed.

To uninstall the service files type *sudo make uninstall_service*.


## configure service

After installing the service files edit the /etc/pi-buttons.conf file with your button
GPIO values, a Unix socket path, and button timing values.


## run service

After installing and configuring the service type *sudo systemctl enable pi-buttons.service*
followed with *sudo systemctl start pi-buttons.service* to begin running the
service.
