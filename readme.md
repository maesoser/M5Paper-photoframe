# M5Paper Photo Frame

This is a low-power dithered photo viewer for the M5Stack M5Paper device. It’s capable of displaying 960x540 resolution images in gorgeous 16 shades of gray.

Special thanks to @Sarah-C, who originally developed the dithering code that makes this possible. [Her version](https://github.com/Sarah-C/M5Stack_M5Paper_PhotoFrame/tree/main) was already excellent, but I decided to fork her repository and make some changes to better fit my needs.

## How It Works

Just by formatting an SD card to FAT32, placing your images there and inserting the SDCard in the M5Paper should be enough.

## Additional Info

#### Image Handling

Small images are centered on the screen but not enlarged. Large images (those exceeding 960x540) are automatically resized to fit the screen by scaling them down to half, a quarter, or an eighth of their original size. For best results, prepare a collection of images with a resolution of 960x540 pixels to fully utilize the display.

This repository includes a script named `encode.sh` which uses [ImageMagick](https://imagemagick.org/index.php) to resize and prepare the images.

#### Index File Creation

The first time you insert the SD card into the M5Paper, it will generate an index file to speed up the image selection process. If you add new images later, you must delete the index file to force the device to recreate the database and add the new images to it.

#### Power and Sleep Behavior

By default, the M5Paper displays a new image every 10 minutes and then enters deep sleep mode to conserve power. If the device is connected to power, it will refresh the image every minute instead.

#### Temperature, Humidity and Battery Level

The M5Paper also displays two additional pieces of information on the screen:

- Temperature and Humidity: Shown in the top-right corner, detected by the DHT11 sensor embedded in the device.
- Battery Percentage: Shown in the bottom-right corner of the screen.