# Omniglass, on your desktop.
This project (early Work In Progress) intends to provide a touchpad-based adapter over traditional applications on your desktop, without modifying the applications themselves.
 It will do this by asking the compositor: "what is the exact application focused right now?" and then mapping touch gestures to keyboard/mouse actions accordingly.

## Status
Right now this is mostly a copy-paste of the first prototype i could come up with, except this time it's rewritten as a QObject inside a QCoreApplication running from a constant 15ms timer.

There are many tasks ahead until any actual progress can be witnessed. Here are some behind-the scenes janitor items to clear (non-exaustive list):
- make a QDBusAdapter for the glassd core
- export at least one function or signal from glassd to the DBus adaptor
- register the adaptor to the service `com.guarapicci.glassd` on the session bus of DBus
- have at least one window that provides some visualization of what glassd is interpreting from omniglass.
