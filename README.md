# Lumi

<p align="center">
<img src="https://cdn.discordapp.com/attachments/1110664108602294352/1116214073881808977/devplex_logo_illustration_design_brightness_monitor_3D_floating_1ab6c071-7b21-42d0-a30f-5bbac0dd1691.png" height="300">
</p>

Lumi is a Node.js module that allows you to adjust the brightness of your internal or external monitors.

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

### `lumi.set(monitorId: string | typeof ALL_MONITORS, brightness: number)`

Attempts to set the brightness level of a specific monitor identified by monitorId or a global brightness level.

- monitorId: The ID of the monitor to set the brightness level for. Use the GLOBAL constant to set a global brightness
  level.
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
- **serial**: The serial number of the monitor.
- **productCode**: The product code of the monitor.
- **internal**: Indicates whether the monitor is an internal display.

### `GetBrightnessResult`

The result of a get brightness operation.

- **success**: Indicates whether the operation was successful.
- **brightness**: The retrieved brightness level. It is null when success is false.

### `SetBrightnessResult`

The result of a set brightness operation.

- **success**: Indicates whether the operation was successful.
- **message**: An error message when success is false, otherwise null.