# power-bank-sensor-node
Tutorial how to build a USB-chargeable temperature / humidity / pressure sensor node in a power bank case which transmits the data via 433MHz to a receiver whose serial protocol is MySensor compatible.


## Prequesites

Here is a list of things to buy:

1. Arduino Pro Mini 8MHz, 3.3V
2. 433 MHz sender module
3. BMP180 pressure sensor
4. DHT22 humidity sensor
5. USB cell phone charger
6. 16340 rechargeable battery

## Howto

#### Step 1: Remove unecessary and power hungry parts of the Arduino

Here is the Arduino Pro Mini I used (left picture). First unsolder the reset button (right image, green circle on the left), the power indicator LED (circle in the middle) and the step-down converter on the right.

<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0001.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0002.jpg" width="200">

#### Step 2: Reduce the size of the Arduino

We have to shorten the Arduino so it fits together with the battery into the power bank case. So cut it with a saw between pin 11 and 12 so that pin 12 still remains usable as seen on the left picture. The result can be seen on the right.

<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0003.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0004.jpg" width="200">

#### Step 3: Remove the pins of the 433MHz sender

The sender comes with pins that are not needed.

<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0005.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0006.jpg" width="200">

#### Step 4: Add programming header

Add a female pin header to the Arduino to program it later. The free pin on the right is the second ground pin on the row.

<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0007.jpg" width="200">

#### Step 5: Create sender adapter

To connect the sender to the Arduino create an adapter as seen on the left picture. Solder it to GND, VCC and pin A3.

<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0008.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0009.jpg" width="200">

#### Step 6: Add some cables

Solder two (half blue) cables to the free GND pin right of the programming headers (green circle at the top). Also add cables to pin A4 (half green wire) and A5 (green wire) (green circle in the middle). Remove the plastic connectors of the sender adapter and cut away one section. Then add it on the to straight pins and push it down to the Arduino board as seen on the right picture (green circle)

<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0010.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0011.jpg" width="200">

#### Step 7: Add pull up resistor and connect sender

Solder a 4.7kOhm resistor to pin 4 and 7 as seen on the left picture (green circle). Then add the sender module on the top connected to the adapter as seen on the picture.

<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0012.jpg" width="200">

#### Step 8: Add pin for the BMP180 sensor

The VCC of the BMP180 sensor will be connected to pin 5 on the Arduino. Therefore a long pin header is soldered to pin 5 as seen on the picture below (green circle).

<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0013.jpg" width="200">

#### Step 9: Prepare BMP180 sensor

Put a piece of tape on the bottom side of the BMP180 sensor as seen on the right image.

<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0014.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0015.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0016.jpg" width="200">

#### Step 10: Add BMP180 sensor

Cut the green wires and one of the blue ones as seen on the picture. Then solder the BMP180 as seen to the pin header (pin 5), GND (half blue wire), A5 (green wire) and A4 (half green wire).

<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0017.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0018.jpg" width="200">

#### Step 11: Add DS18B20 connector

Solder a long (10cm) wire to Arduino pin 6 as seen on the left picture. On the back side of the Arduino connect pin 6 and pin 7 as seen on the right picture (I forgot it when taking the pictures, that's why it shows a later step).

<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0019.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0035.jpg" width="200">

...soon to continue...

<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0020.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0021.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0022.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0023.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0024.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0025.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0026.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0027.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0028.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0029.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0030.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0031.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0032.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0033.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0034.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0036.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0037.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0038.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0039.jpg" width="200">
<img src="https://raw.githubusercontent.com/sebastianha/power-bank-sensor-node/master/images/0040.jpg" width="200">