# What is this?
This repo contains esphome external component with alternative implementation of TuyaMCU protocol(s). While esphome already supports TuyaMCU via the [tuya component](https://esphome.io/components/tuya/), I found it incomplete, outdated and inflexible, and it was very hard to make it work with some of my devices. Also it only supports the standard TuyaMCU protocol, which make it useless when dealing with eg. battery devices.

Although Uyat originated from the esphome tuya component, I decided not to keep its yaml syntax. It might be similar, but sometimes there are significant differences, so please make sure to read the [description for each component](#supported-esphome-components).
Also note that this is still work in progress and at this stage the syntax changes are going to happen often and I don't intend the syntax to be backward compatible between versions.

This readme assumes the user already knows how to deal with TuyaMCU devices, what datapoints are and how to find the list and description of the datapoints for their devices. In the future I might also write a guide on how to do that, but till then, use google and ask on forums like [elektroda](http://elektroda.com).

# Supported TuyaMCU protocols
- [Standard protocol](https://developer.tuya.com/en/docs/iot/tuya-cloud-universal-serial-port-access-protocol?id=K9hhi0xxtn9cb)
- [Low-power protocol](https://developer.tuya.com/en/docs/iot/tuyacloudlowpoweruniversalserialaccessprotocol?id=K95afs9h4tjjh) (TBD)
- [Lock protocol](https://developer.tuya.com/en/docs/iot/door-lock-mcu-protocol?id=Kcmktkdx4hovi) (TBD)

# Supported esphome components
## Basic (single-dpids)
- [Binary Sensor](#binary-sensor)
- [Button](#button)
- [Number](#number)
- [Select](#select)
- [Sensor](#sensor)
- [Switch](#switch)
- [Text Sensor](#text-sensor)

## Complex
- [Climate](#climate)
- [Fan](#fan)
- [Cover](#cover)
- [Light](#light)
- Valve (TBD)
- Water Heater (TBD)

# Component configuration
To use Uyat, first you need to make sure it is added as external component:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/szupi-ipuzs/esphome-external-uyat
      ref: main
    components: ["uyat"]
```

Uyat requires uart component to talk to the MCU, so you need to add one in your yaml. Then just add `uyat:`, eg:

```yaml
uart:
  rx_pin: RX
  tx_pin: TX

uyat:
```

or

```yaml
uart:
  id: uyat_uart
  rx_pin: RX2
  tx_pin: TX2

uyat:
  uart_id: uyat_uart
```

The above should make the Uyat component activate on startup and begin communication with TuyaMCU - this includes sending heartbeats (when needed) and seamlessly answering to various MCU requests required by the protocols (pairing, time, wifi status, etc.).

## Time handling
MCU on some devices require valid time to be sent by us before they start sending datapoints. To be able to do this, Uyat needs time_id to be specified, eg.:

```yaml
uyat:
  time_id: homeassistant_time
```

## Diagnostics
You can make Uyat create entities for some diagnostic information that are not really needed during normal usage, but you may find them useful when creating config for a new device, especially if you don't have access to uart logs. See the example config below.

```yaml
uyat:
  diagnostics:
    product:
      name: "Product"
    num_garbage_bytes:
      name: "Garbage bytes"
    unknown_commands:
      name: "Unknown commands"
    unknown_extended_commands:
      name: "Unknown extended commands"
    unhandled_datapoints:
      name: "Unhandled datapoints"
    pairing_mode:
      name: "Pairing mode"
```

Each of the above entries is optional, each creates a text entity.

- `product` - contains full answer to the ['Query product information' command](https://developer.tuya.com/en/docs/iot/tuyacloudlowpoweruniversalserialaccessprotocol?id=K95afs9h4tjjh#title-6-Query%20product%20information) as sent by the MCU.
- `num_garbage_bytes` - the number of bytes skipped when parsing TuyaMCU commands. This can tell you if there's something wrong with the uart connection.
- `unknown_commands` - the list of protocol commands (in hex) that the MCU sent to us and were unhandled. If this is not 0, then the protocol implementation is incomplete.
- `unknown_extended_commands` - similar to the above, but this list contains the subcommands of the [command 0x34](https://developer.tuya.com/en/docs/iot/tuya-cloud-universal-serial-port-access-protocol?id=K9hhi0xxtn9cb#title-39-Extended%20services)
- `unhandled_datapoints` - the list of datapoint ids (in hex) that were reported by the MCU, which were not handled. If this is not empty then you probably have not setup all the functionality yet.
- `pairing mode` - this shows the current pairing mode as seen by the MCU. The possible values are: `ap`, `smartconfig` and `none`. Some devices will not send their datapoints unless pairing is complete and the device is connected to the cloud.

## Manual parsing of datapoint data
If you find that none of the [components](#supported-esphome-components) support your specific datapoints, there's an option to do the parsing manually in a lambda - in the same way it was done in the original esphome tuya implementation, eg:

```yaml
uyat:
  on_datapoint_update:
    - datapoint: 6
      datapoint_type: raw
      then:
        - lambda: |-
            id(voltage_sensor).publish_state((x[0] << 8 | x[1]) * 0.1);
            id(current_sensor).publish_state((x[3] << 8 | x[4]) * 0.001);
            id(power_sensor).publish_state((x[6] << 8 | x[7]));
```
The `x` passed to lambda contains the payload of the datapoint as sent by the MCU. The type of x depends on datapoint_type.
You can also pass `datapoint_type: any`, in which case x carries the whole UyatDatapoint structure. See the source code on how to use it.
Note that if you specify a different type than `any`, your lambda will NOT be called if the type does not match.

## Reporting AP name
For some unknown reason, [the MCU may ask us](https://developer.tuya.com/en/docs/iot/tuya-cloud-universal-serial-port-access-protocol?id=K9hhi0xxtn9cb#subtitle-81-Get%20information%20about%20Wi-Fi%20module) about the base SSID we use in AP mode. I suspect this might be a way for the MCU to detect "re-branded" devices and modify its behavior depending on the brand.
By default, this implementation returns the string `smartlife`, but you can override this by using `report_ap_name`, eg.:

```yaml
uyat:
  report_ap_name: "SL-Vactidy"
```

# Automations
## Factory reset
The standard protocol allows sending the ["factory reset" command](https://developer.tuya.com/en/docs/iot/tuya-cloud-universal-serial-port-access-protocol?id=K9hhi0xxtn9cb#subtitle-80-(Optional)%20The%20reset%20status) to the MCU.
This command can be triggered from yaml by using the `factory_reset` automation.
The automation accepts one parameter:
- `type` (enum, optional) - the type of reset to send to the MCU. Possible values: `HW`, `APP`, `APP_WIPE`. If ommited, then `HW` is sent.

Example yaml:
```yaml
button:
  - platform: template
    name: "Factory Reset HW"
    on_press:
      then:
        - uyat.factory_reset:
            type: HW
  - platform: template
    name: "Factory Reset APP"
    on_press:
      then:
        - uyat.factory_reset:
            type: APP
  - platform: template
    name: "Factory Reset APP_WIPE"
    on_press:
      then:
        - uyat.factory_reset:
            type: APP_WIPE
```

# Datapoints
To be able to correctly control the device, this implementation needs to know its datapoints, both their numbers and their types.
You need to specify them as part of yaml config for a specific component.
## Types
Using correct type for a datapoint is crucial for the implementation. In case of type mismatch (ie. the MCU sends different type than expected for a datapoint) the component handler will not able to send/receive the data to/from the MCU.

Unlike the original Esphome tuya component, I chose to follow the naming that Tuya uses. Therefore these are the types you can specify:
- `raw` - raw data, in lambdas represented as `std::vector<uint8_t>`
- `string` - an utf8-encoded text, in lambdas represented as `std::string`
- `value` - a 32-bit unsigned number, in lambdas represented as `uint32_t`
- `enum` - an 8-bit unsigned number, in lambdas represented as `uint8_t`
- `bool` - a boolean value, in lambdas represented as `bool`
- `bitmap` - an 8- or 16- or 32-bit value, in lambdas represented as `uint32_t`. The actual number of bits that the MCU sends may vary (eg. it does not send more than 8 if the rest of bits is 0).

There are also special values that you can use as type in some cases:
- `any` - will match any datapoint type. Can only be used with on_datapoint_update (see [here](#manual-parsing-of-datapoint-data))
- `detect` - we will use the type that the MCU reported for the datapoint. See [here](#autodetection-of-datapoint-type).

## Specifing a datapoint
Most of the time when you specify a datapoint you can do it in three forms:
### Short form
Short form only requires to specify the number, while the type is automatically set to a default type, which depends on the context - see the [detailed description of components](#components).
Example of short form
```yaml
switch:
  platform: uyat
  datapoint: 5
```

### Long form
Long form allows specifying also the `datapoint_type`:
- `number` (required, number) - the datapoint number
- `datapoint_type` (optional, enum), one of the [types](#types), if not specified a default type is used, which depends on the context - see the [detailed description of components](#components).

Example of long form:
```yaml
switch:
  platform: uyat
  datapoint:
    number: 10
    datapoint_type: enum
```

## Long form with payload
So far used only for buttons, requires additionally to specify the `trigger_payload`:
- `number` (required, number) - the datapoint number
- `datapoint_type` (optional, enum), one of the [types](#types), if not specified a default type is used, which depends on the context - see the [detailed description of components](#components).
- `trigger_payload` (required) - the datapoint value, must match the selected type.


In general I recommend to specify the type explicitly, as the default type is not always the correct one.

## Autodetection of datapoint type
In some cases it is allowed to specify `datapoint_type: detect`. While this sounds convenient, I would advise *not to use it* with components that send a datapoint value to the MCU (most of them do). Here's why: sending datapoint value to the MCU can only be done if the type is known. So if the MCU does not report a specific datapoint first (some devices do that), then you will never be able to set it.
This means you can safely use it with sensor and binary_sensor, or when you're sure your device always reports the datapoint value first.

# Components
## Binary Sensor
When creating a binary sensor entity (ie. a true/false value) you need to specify:
- `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`, `bitmap`. The default type is `bool`.
- `bit_number` (optional, 1-32) - the bit to check in the value. Note that the number is 1-based.
- all other options from the [Esphome Binary Sensor](https://esphome.io/components/binary_sensor/)

If `bit_number` is specified, the sensor will be evaluated to `True` if the bit is set in the received value.
If `bit_number` is not specified, the sensor will be evaluated to `True` if the value is non-zero.
The actual value of the sensor can be altered by the filters common for all binary sensors (see [Esphome Binary Sensor](https://esphome.io/components/binary_sensor/)).

Example usage:
```yaml
binary_sensor:
  - platform: "uyat"
    name: "Side Brush Problem"
    datapoint:
      number: 11
      datapoint_type: bitmap
    bit_number: 1
  - platform: "uyat"
    name: "Mid Brush Problem"
    datapoint:
      number: 11
      datapoint_type: bitmap
    bit_number: 2
```

```yaml
binary_sensor:
  - platform: "uyat"
    datapoint:
      number: 9
    bit_number: 1
    name: "Short Circuit"
    icon: "mdi:alert-octagon"
    entity_category: diagnostic
    filters:
      - delayed_off: 1s
```

## Button
When creating a button entity, you need to specify:
- `datapoint` (required) - must be in its [long form with payload](#long-form-with-payload). Allowed types: `bool`, `value`, `enum`. The default type is `bool`.
- all other options from the [Esphome Button](https://esphome.io/components/button/)

Examples:
```yaml
  - platform: "uyat"
    datapoint:
      number: 12
      datapoint_type: bool
      trigger_payload: True
    name: "Energy Reset"
    icon: "mdi:recycle"
    entity_category: config
```

## Number
When creating a number entity, you need to specify:
- `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `bool`.
- `min_value` (required) - the minimum value of the entity
- `min_value` (required) - the maximum value of the entity
- `step` (required) - the granularity
- `multiplier` (optional, exclusive with `scale`) - the value received from the MCU will be multiplied by this number and the result will be set as the entity state. The value set to the MCU will be divided by this number before sending.
- `scale` (optional, exclusive with `multiplier`) - same as `multiplier`, but sets the multiplier to 10^scale
- `offset` (optional) - the value received from the MCU will be be increased by `offset`. The value set to the MCU will be decreased by `offset`.
- all other options from the [Esphome Number](https://esphome.io/components/number/)

Example yaml:
```yaml
number:
  - platform: uyat
    name: Volume
    datapoint:
      number: 43
      datapoint_type: value
    min_value: 0
    max_value: 100
    step: 10
    scale: 1 # times 10
    unit_of_measurement: "%"
    entity_category: config
    device_class: volume
    icon: mdi:volume-high
```

## Select
When creating a select entity you need to specify:
- `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `enum`.
- `options` (required) - the mapping between the MCU values and the entity values
- `optimistic` (optional, bool) - if True, then setting the entity value will take effect immediately, without waiting for the MCU to answer. Default is false.
- all other options from the [Esphome Select](https://esphome.io/components/select/)

Example yaml:
```yaml
select:
  - platform: uyat
    name: "Cleaning Mode"
    datapoint:
      number: 25
      datapoint_type: enum
    options:
      0: "Global Sweep"
      1: "Edge Sweep"
      2: "Mopping"
      3: "Auto Recharging"
      4: "Spot Sweep"
      5: "Standby"
```

## Sensor
Two kinds of sensors are currently supported: a `number` sensor and a `vap` sensor. They can be distinguished by specifying additional `type` key. If the `type` key is omitted, the `number` is assumed.
### Number sensor
The state (ie. value) of this sensor is exactly what the MCU sent in the datapoint.
When creating this sensor, you need to specify:
- `type` (optional) - either set to `number` or don't define it.
- `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`, `bitmap`. The default type is `detect`.
- all other options from the [Esphome Sensor](https://esphome.io/components/sensor/)

Example yaml:
```yaml
sensor:
  - platform: "uyat"
    type: number
    name: "Mopping Type"
    datapoint:
      number: 107
      datapoint_type: detect
```

### VAP sensor
`VAP` stands for Voltage, Amperage and Power. Some Tuya breakers and power meters encode these value on specific parts of a raw datapoint. They can be [decoded manually](#manual-parsing-of-datapoint-data), but it is more convenient to get them using this sensor.
You can only decode one of the three types in one sensor, but you can use the same datapoint in many sensors.
When creating this sensor, you need to specify:
- `type` (optional) - must be set to `vap`
- `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `raw`. The default type is `raw`.
- `vap_value_type` (required) - one of `voltage`, `amperage` or `power`
- all other options from the [Esphome Sensor](https://esphome.io/components/sensor/)

Example yaml:
```yaml
sensor:
  - platform: "uyat"
    datapoint: 6
    type: vap
    vap_value_type: voltage
    name: "Voltage"
    unit_of_measurement: "V"
    filters:
      multiply: 0.1
  - platform: "uyat"
    datapoint: 6
    type: vap
    vap_value_type: amperage
    name: "Current"
    unit_of_measurement: "A"
    filters:
      multiply: 0.001
  - platform: "uyat"
    datapoint: 6
    type: vap
    vap_value_type: power
    name: "Power"
    id: power_sensor
    unit_of_measurement: "W"
```

## Switch
When creating a switch entity you need to specify:
- `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `bool`.
- all other options from the [Esphome Switch](https://esphome.io/components/switch/)

Note that handling of the switch datapoint is the same as of the binary sensor's datapoint, ie. the 0 values are treated as `False`, non-0 as `True`.
But setting the switch to `True` always sends value of `1` to the MCU and setting it to `False` always sends value of `0`.

Example yaml:
```yaml
switch:
  - platform: uyat
    datapoint:
      number: 16
      datapoint_type: bool
    name: Relay
```

## Text Sensor
Two kinds of text sensors are currently supported: a `text` type and a `mapped` type. They can be selected by specifying additional `type` key. If the `type` key is omitted, `text` is used by default.
### Normal Text Sensor
This kind of sensor simply uses the payload of a datapoint and interprets it as text. You can select additional encoding of this text.
When creating this sensor you need to specify:
- `type` (optional) - either `text` or don't define it.
- `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `raw`, `string`. The default type is `detect`.
- `encoding` (optional) - additional processing to be done over the datapoint payload. Can be `plain` (no processing), `base64` (base64 decoder is used) or `hex` (each character is shown in hex). If not specified, `plain` is used.
- all other options from the [Esphome Text Sensor](https://esphome.io/components/text_sensor/)

Example yaml:
```yaml
text_sensor:
  - platform: "uyat"
    name: "Serial Number"
    datapoint: 58
    type: text
```

### Mapped Text Sensor
This kind of sensor reads a value from a numeric datapoint and uses a user-defined table to translate it into a text. Think of it as of a read-only [select component](#select).
When creating this sensor you need to specify:
- `type` (required) - must be set to `mapped`
- `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `detect`.
- `options` (required) - the mapping of datapoint numerical values to text values.
- all other options from the [Esphome Text Sensor](https://esphome.io/components/text_sensor/)

Example yaml:
```yaml
text_sensor:
  - platform: "uyat"
    name: "Current Status"
    type: "mapped"
    datapoint:
      number: 38
      datapoint_type: enum
    options:
      0: "Standby"
      1: "Automatic Cleaning"
      2: "Mopping"
      3: "Wall Cleaning"
      4: "Returning to Charging Station"
      5: "Charging"
      6: "Spot Cleaning"
```

## Climate
The original Esphome Tuya Climate component allows specifying many options. I decided to keep all of the for the time being, but made them tidier (I think). Note that some of the combination of options below are mutually exclusive.
There are some generic options that should be specified on the top-level. The rest of the options are separated in their own section.

### Climate: generic options
- `supports_heat` (optional, boolean) - set to `True` if the device has the actual heating functionality. Defaults to `True`.
- `supports_cool` (optional, boolean) - set to `True` if the device has the actual cooling functionality. Defaults to `False`.
- all other options from the [Esphome Climate](https://esphome.io/components/climate/)

Example yaml:
```yaml
climate:
  - platform: "uyat"
    supports_heat: True
```

### Climate: Switch
This section enables handling of a datapoint that turns the device on/off. If your device supports it, specify all the options under the `switch` key.
The possible options:
- `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `bool`.
- `inverted` (optional, boolean) - set to `True` if the behavior should be inverted (ie. if switch value `False` should mean `ON`). Defaults to `False`.

Example yaml:
```yaml
climate:
  - platform: "uyat"
    switch:
      datapoint: 7
      inverted: True
```

### Climate: Active State Datapoint
This section enables handling of a datapoint that controls the device's state, but is not a simple switch. If your device supports it, specify all the options under the `active_state_datapoint` key.
The possible options:
- `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `value`, `enum`. The default type is `enum`.
- `heating_value` (optional, number) - the datapoint value which corresponds to the heating state. If omitted, value of `1` is used.
- `cooling_value` (optional, number) - the datapoint value which corresponds to the cooling state.
- `drying_value` (optional, number) - the datapoint value which corresponds to the drying state.
- `fanonly_value` (optional, number) - the datapoint value which corresponds to the fan-only state.

Example yaml:
```yaml
climate:
  - platform: "uyat"
    active_state_datapoint:
      datapoint:
        number: 7
        datapoint_type: enum
      cooling_value: 100
```


### Climate: Active State Pins
In some older Tuya devices, the heating/cooling state was signaled using GPIOs. If your device supports it, then specify all the options under the `active_state_pins` key.
The possible options (at least one must be specified):
- `heating` (optional, pin) - the GPIO that can used to get the heating state
- `cooling` (optional, pin) - the GPIO that can used to get the cooling state

Example yaml:
```yaml
climate:
  - platform: "uyat"
    active_state_pins:
      cooling: GPIO4
```

### Climate: Temperature
Some devices allow setting target and reading current temperature. If your device supports it, then specify all the options under the `temperature` key.
The possible options:
- `target` (required) - section for specifying options for the target temperature datapoint. The options can be as follows:
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `value`, `enum`. The default type is `value`.
  * `multiplier` (optional, float) - the value received from the MCU will be multiplied by this number and the result will used as the temperature value. The value set to the MCU will be divided by this number before sending.
  * `offset` (optional, float) - the value received from the MCU will be increased by this number and the result will be used as the temperature. The value set to the MCU will be decreased by this number before sending.
- `current` (optional) - section for specifying options for the current temperature datapoint. The options can be as follows:
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `value`, `enum`. The default type is `value`.
  * `multiplier` (optional, float) - the value received from the MCU will be multiplied by this number and the result will used as the temperature value.
  * `offset` (optional, float) - the value received from the MCU will be increased by this number and the result will be used as the temperature.
- `reports_fahrenheit` (optional, boolean) - setting this to `True` will cause recalculation of every temperature value read from the device from ºF to ºC and the other direction when setting temperature to the device. Defaults to `False`.
- `hysteresis` (optional, float) - for the devices that don't have a way of detecting their heating/cooling state from a datapoint, the hysteresis will be used for that. Both current and target temperature datapoints must be defined for the hysteresis to work. If not specified, value of `1.0` is used.

Example yaml:
```yaml
climate:
  - platform: "uyat"
    temperature:
      target:
        datapoint: 10
        multiplier: 0.5
        offset: 30
      current:
        datapoint: 11
      reports_fahrenheit: True
```

### Climate: Preset
Some devices allow users to select special modes of operation. Esphome and HA call them "presets". You can configure them all under the `preset` section.

Three types of presets are currently supported by this implementation: `sleep`, `eco` and `boost`. For some preset types you can also define the temperature that should be set if the preset is selected.

- To create the Eco preset, specify the following under the `eco` key:
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `bool`.
  * `inverted` (optional) - should the behavior of this switch be inverted (ie. switch off if the datapoint evaluates to `True`). The default is `False`.
  * `temperature` (optional) - the temperature that should be set as target if this preset is selected.

- To create the Boost preset, specify the following under the `boost` key:
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `bool`.
  * `inverted` (optional) - should the behavior of this switch be inverted (ie. switch off if the datapoint evaluates to `True`). The default is `False`.
  * `temperature` (optional) - the temperature that should be set as target if this preset is selected.

- To create the Sleep preset, specify the following under the `sleep` key:
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `bool`.
  * `inverted` (optional) - should the behavior of this switch be inverted (ie. switch off if the datapoint evaluates to `True`). The default is `False`.

Example yaml:
```yaml
climate:
  - platform: "uyat"
    preset:
      sleep:
        datapoint: 1
      boost:
        datapoint: 2
        inverted: True
        temperature: 75
```

# Climate: Fan
If your device has a fan, there probably is a datapoint that controls it. The `fan_mode` section deals with options for just that.
The possible options:
- `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `value`, `enum`. The default type is `enum`.
- `auto_value` (optional, number) - the datapoint value which corresponds to the "auto" setting
- `low_value` (optional, number) - the datapoint value which corresponds to the "low" setting
- `medium_value` (optional, number) - the datapoint value which corresponds to the "medium" setting
- `middle_value` (optional, number) - the datapoint value which corresponds to the "middle" setting
- `high_value` (optional, number) - the datapoint value which corresponds to the "high" setting

Example yaml:
```yaml
climate:
  - platform: "uyat"
    fan_mode:
      datapoint: 5
      low_value: 0
      high_value: 1
```

# Climate: Swings
If your device can swing, there probably is a datapoint that controls it. The `swing_mode` section deals with options for just that.
The possible options (at least one must be specified):
- `vertical` (optional) - the section containing settings for vertical swings. In this section you can specify:
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `bool`.
  * `inverted` (optional) - should the behavior of this swings be inverted (ie. swings are off if the datapoint evaluates to `True`). The default is `False`.
- `horizontal` (optional) - the section containing settings for vertical swings
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `bool`.
  * `inverted` (optional) - should the behavior of this swings be inverted (ie. swings are off if the datapoint evaluates to `True`). The default is `False`.

Example yaml:
```yaml
climate:
  - platform: "uyat"
    swing_mode:
      horizontal:
        datapoint: 11
        inverted: True
```

## Cover
The original Esphome Tuya Cover component allows specifying many options. I decided to keep all of the for the time being, but made them tidier (I think).
There are some generic options that should be specified on the top level. The rest of the options are separated in their specific sections.

*Please note that as I don't own a cover device I was not able to test this code with a real device yet.*

### Cover: generic options
- `restore_mode` - one of `NO_RESTORE`, `RESTORE` and `RESTORE_AND_CALL`.
- all other options from the [Esphome Cover](https://esphome.io/components/cover/)

### Cover: Control
The optional `control` section is where you specify the command datatapoint and map its values for Uyat to know which exact values mean which command.
You need to specify the following under the `control` key:
- `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `value`, `enum`. The default type is `enum`.
- `open_value` (optional, number) - the datapoint value for the "open" command.
- `close_value` (optional, number) - the datapoint value for the "close" command.
- `stop_value` (optional, number) - the datapoint value for the "stop" command.

Example yaml:
```yaml
cover:
  - platform: "uyat"
    control:
      datapoint: 5
      open_value: 1
      close_value: 0
```

### Cover: Direction
The optional `direction` section is where you specify the direction datapoint.
You need to specify the following under the `direction` key:
- `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `bool`.
- `inverted` (optional, boolean) - set to True if the logic of this datapoint is inverted (ie. `True` means down). If not specified, defaults to `False`.

Example yaml:
```yaml
cover:
  - platform: "uyat"
    direction:
      datapoint:
        number: 11
        datapoint_type: enum
      inverted: True
```

### Cover: Position
Some devices allow users to specify the exact percentage the cover should close and some also report the exact percentage via a different datapoint. The `position` section is where you can specify these datapoints, as well as their settings. This section is always required.
You need to specify the following under the `position` key:
- `position_datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `value`, `enum`. The default type is `value`. Uyat will write the requested percentage to this datapoint.
- `position_report_datapoint` (optional) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `value`, `enum`. The default type is `value`. If this is specified, Uyat will use this datapoint to read the current percentage. Else the `position_datapoint` will be used for that.
- `inverted` (optional, boolean) - set to True if the value range in the position datapoints is inverted (ie. `min_value` is 100%).
- `min_value` (optional, number) - the exact value that MCU uses for the "fully closed" position (or "fully open" if `inverted`). If not specified, `0` is used.
- `max_value` (optional, number) - the exact value that MCU uses for the "fully open" position (or "fully closed" if `inverted`). If not specified, `100` is used.
- `uncalibrated_value` (optional, number) - the exact value that MCU reports when position requires calibration.

Example yaml:
```yaml
cover:
  - platform: "uyat"
    position:
      position_datapoint:
        number: 11
        datapoint_type: value
      position_report_datapoint: 8
      inverted: True
      min_value: 1
      max_value: 100
      uncalibrated_value: 0
```

## Fan
The mcu fan devices usually allow users to enable oscillation and specify the fan speed either in steps or as a percentage. There is a datapoint for each of these features and you can specify them here.

*Please note that as I don't own a fan device I was not able to test this code with a real device yet.*

### Fan: generic options
There are currently no additional generic options for fans, you can only apply:
- all other options from the [Esphome Fan](https://esphome.io/components/fan/)

### Fan: Switch
Most devices have an on/off switch as a separate datapoint.
You need to specify it under the `switch` key:
- `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `bool`.
- `inverted` (optional, boolean) - set to True if the logic of this datapoint is inverted (ie. `True` or `1` means "off"). If not specified, defaults to `False`.

Example yaml:
```yaml
fan:
  - platform: "uyat"
    switch:
      datapoint: 7
      inverted: False
```

### Fan: Oscillation
The fan devices that can oscillate have a datapoint for it.
You need to specify it under the `oscillation` key:
- `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `bool`.
- `inverted` (optional, boolean) - set to True if the logic of this datapoint is inverted (ie. `True` or `1` means "off"). If not specified, defaults to `False`.

Example yaml:
```yaml
fan:
  - platform: "uyat"
    oscillation:
      datapoint: 10
      inverted: True
```

### Fan: Direction
Some ceiling fans can move in any of two directions which can be controlled by a datapoint.
You need to specify it under the `direction` key:
- `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `bool`.
- `inverted` (optional, boolean) - set to True if the logic of this datapoint is inverted (ie. `True` or `1` means "reverse"). If not specified, defaults to `False`.

Example yaml:
```yaml
fan:
  - platform: "uyat"
    direction:
      datapoint: 11
      inverted: True
```

### Fan: Speed
Some fans allow users to specify the rotation speed, by either selecting on of the presets or by specifying the percentage.
In both cases the datapoint need to be specified under the `speek` key:
- `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `value`, `enum`. The default type is `value`.
- `min_value` (optional, number) - the exact value that MCU uses for the "lowest speed". If not specified, `1` is used.
- `max_value` (optional, number) - the exact value that MCU uses for the "highest speed". If not specified, `100` is used.

Example yaml:
```yaml
fan:
  - platform: "uyat"
    speed:
      datapoint:
        number: 11
        datapoint_type: enum
      min_value: 0
      max_value: 3
```


## Light

Different subsections are required depending on which datapoints are available for your light
- `type` (required) - the type of light to interface. This setting determines what subsections are required and optional. Allowed types: `binary`, `dimmer`, `ct`, `rgb`, `rgbw`, `rgbct`.

### type: binary
- `switch` (optional) - the section containing the settings for the binary control of the light (ON/OFF):
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `bool`.
  * `inverted` (optional) - should the behavior of this switch be inverted (ie. switch off if the datapoint evaluates to `True`). The default is `False`.

```yaml
light:
  - platform: uyat
    type: binary
    name: "Light"
    switch:
      datapoint: 1
      inverted: false
```

### type: dimmer
- `switch` (optional) - the section containing the settings for the binary control of the light (ON/OFF):
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `bool`.
  * `inverted` (optional) - should the behavior of this switch be inverted (ie. switch off if the datapoint evaluates to `True`). The default is `False`.
- `dimmmer` (required) - the section containing settings for the light dimming part of the light entity:
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `value`, `enum`. The default type is `value`.
  * `min_value` (optional) - minimal numerical value the dimmer can be set to. The default is `0`.
  * `max_value` (optional) - maximum numerical value the  dimmer can be set to. The default is `255`.
  * `inverted` (optional) - should the light intensity value be inverted where highest number is the dimmest possible value. The default is `False`.
  * `min_value_datapoint` (optional) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `value`, `enum`. The default type is `value`.
- `gamma_correct` (optional) - manual gamma correction factor. The default is `1.0`.
- `default_transition_length` (optional) - on and off transition length. The default is `0s`.

Example yaml:
```yaml
light:
  - platform: uyat
    type: dimmer
    name: "Dimmer"
    dimmer:
      datapoint: 2
      min_value: 1
      max_value: 1000
      inverted: false
      min_value_datapoint: 2
    switch:
      datapoint: 1
      inverted: false
```

### type: ct
- `switch` (optional) - the section containing the settings for the binary control of the light (ON/OFF):
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `bool`.
  * `inverted` (optional) - should the behavior of this switch be inverted (ie. switch off if the datapoint evaluates to `True`). The default is `False`.
- `dimmmer` (required) - the section containing settings for the light dimming part of the light entity:
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `value`, `enum`. The default type is `value`.
  * `min_value` (optional) - minimal numerical value the dimmer can be set to. The default is `0`.
  * `max_value` (optional) - maximum numerical value the  dimmer can be set to. The default is `255`.
  * `inverted` (optional) - should the light intensity value be inverted where highest number is the dimmest possible value. The default is `False`.
  * `min_value_datapoint` (optional) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `value`, `enum`. The default type is `value`.
- `white_temperature` (required) - the section containing settings for the white color temperature.
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `value`, `enum`. The default type is `value`.
  * `min_value` (optional) - minimal numerical value the dimmer can be set to. The default is `0`.
  * `max_value` (optional) - maximum numerical value the  dimmer can be set to. The default is `255`.
  * `inverted` (optional) - should the light intensity value be inverted where highest number is the dimmest possible value. The default is `False`.
  * `cold_white_color_temperature` (required) - the absolute max temperature value for cold white. Possible units are Kelvin (K) or Mireds(mireds).
  * `warm_white_color_temperature` (required) - the absolute min temperature value for warm white. Possible units are Kelvin (K) or Mireds(mireds).
- `gamma_correct` (optional) - manual gamma correction factor. The default is `1.0`.
- `default_transition_length` (optional) - on and off transition length. The default is `0s`.

Example yaml:
```yaml
light:
  - platform: uyat
    type: ct
    name: "Color temperature"
    dimmer:
      datapoint: 2
      min_value: 1
      max_value: 1000
      inverted: false
      min_value_datapoint: 2
    switch:
      datapoint: 1
      inverted: false
    white_temperature:
      datapoint: 101
      cold_white_color_temperature: 6500K
      warm_white_color_temperature: 3000K

```

### type: rgb
- `switch` (optional) - the section containing the settings for the binary control of the light (ON/OFF):
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `bool`.
  * `inverted` (optional) - should the behavior of this switch be inverted (ie. switch off if the datapoint evaluates to `True`). The default is `False`.
- `color` (required) - the section  containing the settings for the RBB contorl of the light.
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed type: `string`.
  * `type` (required) - color representation format. Allowed types: `RGB`, `HSV`, `RGBHSV`.

```yaml
light:
  - platform: uyat
    type: rgb
    name: "RGB"
    switch:
      datapoint: 1
      inverted: false
    color:
      datapoint: 102
      type: "HSV"
```

### type: rgbw
- `switch` (optional) - the section containing the settings for the binary control of the light (ON/OFF):
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `bool`.
  * `inverted` (optional) - should the behavior of this switch be inverted (ie. switch off if the datapoint evaluates to `True`). The default is `False`.
- `dimmmer` (required) - the section containing settings for the light dimming part of the light entity:
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `value`, `enum`. The default type is `value`.
  * `min_value` (optional) - minimal numerical value the dimmer can be set to. The default is `0`.
  * `max_value` (optional) - maximum numerical value the  dimmer can be set to. The default is `255`.
  * `inverted` (optional) - should the light intensity value be inverted where highest number is the dimmest possible value. The default is `False`.
  * `min_value_datapoint` (optional) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `value`, `enum`. The default type is `value`.
- `color` (required) - the section  containing the settings for the RBB contorl of the light.
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed type: `string`.
  * `type` (required) - color representation format. Allowed types: `RGB`, `HSV`, `RGBHSV`.
- `color_interlock` (optional) - Prevent colors and white channel to light up at the same time. The default is `False`.
- `gamma_correct` (optional) - manual gamma correction factor. The default is `1.0`.
- `default_transition_length` (optional) - on and off transition length. The default is `0s`.

```yaml
light:
  - platform: uyat
    type: rgbw
    name: "RGBW"
    switch:
      datapoint: 1
      inverted: false
    dimmer:
      datapoint: 2
      min_value: 1
      max_value: 1000
      inverted: false
      min_value_datapoint: 2
    color:
      datapoint: 102
      type: "HSV"
    color_interlock: true
```

### type: rgbct
- `switch` (optional) - the section containing the settings for the binary control of the light (ON/OFF):
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `bool`.
  * `inverted` (optional) - should the behavior of this switch be inverted (ie. switch off if the datapoint evaluates to `True`). The default is `False`.
- `dimmmer` (required) - the section containing settings for the light dimming part of the light entity:
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `value`, `enum`. The default type is `value`.
  * `min_value` (optional) - minimal numerical value the dimmer can be set to. The default is `0`.
  * `max_value` (optional) - maximum numerical value the  dimmer can be set to. The default is `255`.
  * `inverted` (optional) - should the light intensity value be inverted where highest number is the dimmest possible value. The default is `False`.
  * `min_value_datapoint` (optional) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `value`, `enum`. The default type is `value`.
- `color` (required) - the section  containing the settings for the RBB contorl of the light.
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed type: `string`.
  * `type` (required) - color representation format. Allowed types: `RGB`, `HSV`, `RGBHSV`.
- `white_temperature` (required) - the section containing settings for the white color temperature.
  * `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `value`, `enum`. The default type is `value`.
  * `min_value` (optional) - minimal numerical value the dimmer can be set to. The default is `0`.
  * `max_value` (optional) - maximum numerical value the  dimmer can be set to. The default is `255`.
  * `inverted` (optional) - should the light intensity value be inverted where highest number is the dimmest possible value. The default is `False`.
  * `cold_white_color_temperature` (required) - the absolute max temperature value for cold white. Possible units are Kelvin (K) or Mireds(mireds).
  * `warm_white_color_temperature` (required) - the absolute min temperature value for warm white. Possible units are Kelvin (K) or Mireds(mireds).
- `color_interlock` (optional) - Prevent colors and white channel to light up at the same time. The default is `False`.
- `gamma_correct` (optional) - manual gamma correction factor. The default is `1.0`.
- `default_transition_length` (optional) - on and off transition length. The default is `0s`.

```yaml
light:
  - platform: uyat
    type: rgbct
    name: "RGBCT"
    switch:
      datapoint: 1
      inverted: false
    dimmer:
      datapoint: 2
      min_value: 1
      max_value: 1000
      inverted: false
      min_value_datapoint: 2
    color:
      datapoint: 102
      type: "HSV"
    white_temperature:
      datapoint: 101
      cold_white_color_temperature: 6500K
      warm_white_color_temperature: 3000K
    color_interlock: true
```

# Shoulders of the giant
Even though I don't like the original tuya component, I still think the Esphome Team did a great job with it. I would never be able to write Uyat without it and I have learnt a great deal just from studying it.
Thank you Esphome Team!

# Still to do
Not necessarily in this order.
- The low-power protocol (merge lpuyat)
- The lock protocol
- The breaker alarm triggers
- Code quality: use FixedVector, StringRef
- Remove code bloat
- Persistently storing & restoring settable values

# Contributing
As you can see above there's still plenty to do.
If you want to help or know how to improve this component, you are more than welcome to create a pull request or drop me a line on [Esphome Discord Server](https://discord.com/invite/n9sdw7pnsn).

# License
Since this component is a rewrite, I think it's only fair to keep the [Esphome license](LICENSE).
