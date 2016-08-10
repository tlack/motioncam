motioncam
=========

A simple, adaptable web-connected camera, using the ESP8266, Arducam, and
optionally some server-side bits for archiving.

Perhaps a good base for other, more elaborate projects.

Overview
--------

This collection of software lets you set up ad-hoc camera thingies.

It presents a web-based view of your Arducam-based camera and optionally
uploads frames when it detects motion.

Hardware
--------

ESP8266 board. I used a Wemos D1 Mini but this should work on any board with some adaptations.
Here's [one on Amazon](https://www.amazon.com/Winson-eseller-D1-mini-V2-development/dp/B01GFAO6VW/).
They're cheaper directly from Aliexpress.

ArduCAM 2mp (5mp would work with minor changes). [Available on
Amazon](https://www.amazon.com/Arducam-Module-Megapixels-Arduino-Mega2560/dp/B012UXNDOY/).

Optional: HC-SR501 motion sensor

Wiring
------

For Arducam, this wiring seemed to work for me. You may have a different
experience.

Arducam Pin | ESP8266 pin | Notes
------------|-------------|------
CS | D3 | Didn't seem to work on D8
Mosi | D7 | 
Miso | D6 | 
SCK | D5 | 
GND | GND | 
VCC | 3V3 | 
SDA | D2 | 
SCL | D1 |

If you want to use an HC-SR501 as well, attach these. The pin I selected is
arbitrary so feel free to use others. Carefully press on the edge of the white
dome on the motion detector to release it so you can see which pin is which.

HC-SR501 Pin | ESP8266 pin 
-------------|------------
VCC | 5V | Apparently some are 3V instead of 5V 
GND | GND | 
OUT | D0 |


Software
--------

`esp8266/` is the software to install on the ESP8266/camera device. The device
can be configured to join an existing wifi network or to start its own. Either
way it presents a streaming view of the camera frames on its own IP. See the
configuration section at top of the `motioncam.ino` file to configure its
behavior.

`server/` contains the optional server-side part. It will record a burst of frames
when motion is detected by the device. See CONFIG.sh to customize its behavior.

`server/post/` receives frames from camera device(s) and stores them. It's a
Bash one-liner using `netcat` which must be installed as `nc`, basically.  This
should be updated to HTTP at some point, but I had issues with the client-side
Arduino HTTP libraries when doing post with image data. Eventually it should
accept data samples other than images, allow senders to indicate their node id,
and restrict upload access to configured users/keys, but right now it doesn't
do any of that.

`server/viewer-node/` allows you to review stored frames. It's a crude Node.js
web server using Express and the static file stuff. It uses HTTP AUTH and the
list of users can be configured in users.txt.  The password is always ignored.

Status
------

Prototype

Credits
-------

@tlack

Built at [Building.co](http://building.co) in Miami
