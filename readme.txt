This codebase is designed to run on a Zilog microcontroller.  Unfortunately this requires windows
and the Zilog development environment as far as I know.  

The program is designed to communicate with remote weather sensors using XBee radios.  The remote
sensors run autonomously without a Zilog microcontroller using only the inate capabilities of the 
XBee radios.  These remote sensors poll temperature, light, pressure, and humidity at regular intervals
and stream them back to the Zilog microcontroller.  The microcontroller can be connected to a computer
over serial, and provides a text based interface supporting the following commands.

See final-report.pdf for more details
