# ☀️ Lumi

<p align="center">
<img src="https://i.imgur.com/xextiVJ.png" height="300">
</p>

[![npm version](https://badge.fury.io/js/lumi-control.svg)](https://badge.fury.io/js/lumi-control)

Lumi is a Node.js module that allows you to adjust the brightness of your internal and external monitors.

## Installation

Install via npm:

```commandline
npm install lumi-control
```

## Usage

```javascript
const lumi = require('lumi-control');

// Get a list of available monitors
const monitors = lumi.monitors();

// Get the current brightness of the primary monitor
const {success, brightness} = await lumi.get();

// Get the current brightness of a specific monitor
const {success, brightness} = await lumi.get(monitorId);

// Set the brightness of the primary monitor to 50%
const {success, message} = await lumi.set(50);

// Set a specific monitor's brightness to 50%
const {success, message} = await lumi.set(monitorId, 50);

// Set the brightness for multiple monitors
const config = {
    [firstMonitorId]: 75,
    [secondMonitorId]: 100,
    [thirdMonitorId]: 25
}
const {success, message} = await lumi.set(config);

// Set the global brightness level to 50%
const {success, message} = await lumi.set(lumi.GLOBAL, 50);

```

## API

### `lumi.get()`

Attempts to get the current brightness level of the primary monitor.

**Returns**: A Promise that resolves to a `GetBrightnessResult` object.

### `lumi.get(monitorId: string)`

Attempts to get the brightness level of a specific monitor identified by monitorId.

- monitorId: The ID of the monitor to retrieve the brightness level for.

**Returns**: A Promise that resolves to a `GetBrightnessResult` object.

### `lumi.set(brightness: number)`

Attempts to set the brightness level of the primary monitor.

- brightness: The brightness level to set.

**Returns**: A Promise that resolves to a `SetBrightnessResult` object.

### `lumi.set(config: BrightnessConfiguration)`

Sets the brightness levels for different monitors.

- config: An object that maps monitor IDs to brightness levels.

**Returns**: A Promise that resolves to a `SetBrightnessResult` object.

### `lumi.set(monitorId: string | ALL_MONITORS, brightness: number)`

Attempts to set the brightness level of a specific monitor by id or, if provided the `GLOBAL` constant, the global brightness level.

- monitorId: The ID of the monitor to set the brightness level for.
- brightness: The brightness level to set.

**Returns**: A Promise that resolves to a `SetBrightnessResult` object.

### `lumi.monitors()`

Returns an array of available monitors.

**Returns**: An array of `Monitor` objects.

## Types

### `BrightnessConfiguration`

An object that maps monitor IDs to brightness levels.

### `Monitor`

Represents a monitor.

- **id**: The unique ID of the monitor.
- **name**: The name of the monitor.
- **manufacturer**: The manufacturer of the monitor.
- **serialNumber**: The serial number of the monitor.
- **productCode**: The product code of the monitor.
- **internal**: Indicates whether the monitor is an internal display.
- **position**: Monitor coordinates in pixels (x, y)
- **size**: Monitor size in pixels (width, height)

### `GetBrightnessResult`

The result of a get brightness operation.

- **success**: Indicates whether the operation was successful.
- **brightness**: The retrieved brightness level. It is null when success is false.

### `SetBrightnessResult`

The result of a set brightness operation.

- **success**: Indicates whether the operation was successful.
- **message**: An error message when success is false, otherwise null.
