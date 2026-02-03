# What is this?
This repo contains esphome external component with alternative implementation of TuyaMCU protocol(s). While esphome already supports TuyaMCU via the [tuya component](https://esphome.io/components/tuya/), I found it incomplete, outdated and inflexible, and it was very hard to make it work with some of my devices. Also it only supports the standard TuyaMCU protocol, which make it useless when dealing with eg. battery devices.

Although Uyat originated from the esphome tuya component, I decided not to keep its yaml syntax. It might be similar, but sometimes there are significant differences, so please make sure to read the [description for each platform](#supported-esphome-platforms).

This readme assumes the user already knows how to deal with TuyaMCU devices, what datapoints are and how to find the list and description of the datapoints for their devices. In the future I might also write a guide on how to do that, but till then, use google and ask on forums like [elektroda](http://elektroda.com).

# Supported TuyaMCU protocols
- [Standard protocol](https://developer.tuya.com/en/docs/iot/tuya-cloud-universal-serial-port-access-protocol?id=K9hhi0xxtn9cb)
- [Low-power protocol](https://developer.tuya.com/en/docs/iot/tuyacloudlowpoweruniversalserialaccessprotocol?id=K95afs9h4tjjh) (TBD)
- [Lock protocol](https://developer.tuya.com/en/docs/iot/door-lock-mcu-protocol?id=Kcmktkdx4hovi) (TBD)

# Supported esphome platforms
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
If you find that none of the [platforms](#supported-esphome-platforms) support your specific datapoints, there's an option to do the parsing manually in a lambda - in the same way it was done in the original esphome tuya implementation, eg:

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

# Platforms
## Binary Sensor
## Button
## Number
## Select
## Sensor
## Switch
## Text Sensor

## Climate
## Cover
## Fan
## Light

# Shoulders of the giants
Even though I don't like the original tuya component, I still think the Esphome Team did a great job with it. Without it I would never be able to write Uyat and I have learnt a great deal just from studying it.
Thank you Esphome Team!

# License
Since this component is a rewrite, I think it's only fair to keep the Esphome license.