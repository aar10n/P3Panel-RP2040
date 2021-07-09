## P3Panel-RP2040
This library adds support for P3 RGB LED panels to the Arduino Nano RP2040 connect and
other RP2040 based microcontrollers. This library is multi-core safe, which allows the
display updating to be handled by one core, while the other updates the content. This has
been tested with (up to) two 64x32 panels daisychained together, but it should support other
panel dimensions no problem.

This library supports basic pixel updating, filling rows/columns, and rendering text in both
a large and small font.

![fonts](font.jpg)

### Updating the display

This library uses double-buffering to ensure the display does not render before the content
has been updated. All drawing operations take place on the back buffer, after which it is
marked "dirty" and then swapped with the front buffer before the next update. If multiple 
things need to be updated in a single frame, you must first call `beginDraw()` to aquire the 
buffer. Then after the changes have been made, you must call `endDraw()` to release the buffer 
allowing the changes to be rendered.
