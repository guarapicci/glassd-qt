# Omniglass, on your desktop.
This project (early Work In Progress) intends to provide a touchpad-based adapter over traditional applications on your desktop, without modifying the applications themselves.
 It will do this by asking the compositor: "what is the exact application focused right now?" and then mapping touch gestures to keyboard/mouse actions accordingly.

## Status
Right now the project builds a simple terminal application that claims the name "com.guarapicci.glassd" on the DBus session bus. It emits signals for when initialization is cleared and for when you drag from the top-left corner towards the center.

There are many tasks ahead until any actual progress can be witnessed. Here are some behind-the scenes janitor items to clear (non-exaustive list):
- ~~make a QDBusAdapter for the glassd core~~ done, Glassd_dbus class does it.
- ~~export at least one function or signal from glassd to the DBus adaptor~~ done, some thin logic in Glassd_dbus emits state change signals and replies to methods to-and-from DBus.
- ~~register the adaptor to the service `com.guarapicci.glassd` on the session bus of DBus~~ upon init, the DBus service goes live at "com.guarapicci.glassd"
- have at least one window that provides some visualization of what glassd is interpreting from omniglass.
