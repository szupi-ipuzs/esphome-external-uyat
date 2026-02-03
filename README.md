# What is this?
This repo contains esphome external component with alternative implementation of TuyaMCU protocol(s). While esphome already supports TuyaMCU via the [tuya component](https://esphome.io/components/tuya/), I found it incomplete, outdated and inflexible, and it was very hard to make it work with some of my devices. Also it only supports the standard TuyaMCU protocol, which make it useless when dealing with eg. battery devices.

Although Uyat originated from the esphome tuya component, I decided not to keep its yaml syntax. It might be similar, but sometimes there are significant differences, so please make sure to read the [description for each component](#supported-esphome-components).

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
- `multiplier` (optional, exclusive with `multiplier`) - the value received from the MCU will be multiplied by this number and the result will be set as the entity state. The value set to the MCU will be divided by this number before sending.
- `scale` (optional, exclusive with `multiplier`) - same as `multiplier`, but sets the multiplier to 10^scale
- `offset` (optional) - the value received from the MCU will be be increased by `offset`. The value set to the MCU will be decreased by `offset`.
- all other options from the [Esphome Number](https://esphome.io/components/number/)

## Select
When creating a select entity you need to specify:
- `datapoint` (required) - either [the short](#short-form) or [long form](#long-form). Allowed types: `detect`, `bool`, `value`, `enum`. The default type is `enum`.
- `options` (required) - the mapping between the MCU values and the entity values
- `optimistic` (optional, bool) - if True, then setting the entity value will take effect immediately, without waiting for the MCU to answer. Defautl is false.

## Sensor
## Switch
## Text Sensor

## Climate
## Cover
## Fan
## Light


# Shoulders of the giants
Even though I don't like the original tuya component, I still think the Esphome Team did a great job with it. I would never be able to write Uyat without it and I have learnt a great deal just from studying it.
Thank you Esphome Team!

# Still to do
Not necessarily in this order.
- The low-power protocol (merge lpuyat)
- The lock protocol
- The breaker alarm triggers
- Code quality: use FixedVector, StringRef, stop using get_object_id
- Remove code bloat
- Persistently storing & restoring settable values

# Contributing
As you can see above there's still plenty to do.
If you want to help or know how to improve this component, you are more than welcome to create a pull request or drop me a line on [Esphome Discord Server](https://discord.com/invite/n9sdw7pnsn).

# License
Since this component is a rewrite, I think it's only fair to keep the Esphome license.