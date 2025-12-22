from esphome.components import time
from esphome import automation
from esphome import pins
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.components import sensor as esphome_sensor
from esphome.components import text_sensor as esphome_text_sensor
from esphome.const import (
       CONF_ID,
       CONF_TIME_ID,
       CONF_TRIGGER_ID,
       CONF_SENSOR_DATAPOINT,
       ENTITY_CATEGORY_DIAGNOSTIC,
       STATE_CLASS_MEASUREMENT,
)

DEPENDENCIES = ["uart"]

CONF_IGNORE_MCU_UPDATE_ON_DATAPOINTS = "ignore_mcu_update_on_datapoints"

CONF_ON_DATAPOINT_UPDATE = "on_datapoint_update"
CONF_DATAPOINT_TYPE = "datapoint_type"
CONF_STATUS_PIN = "status_pin"
CONF_DIAGNOSTICS = "diagnostics"
CONF_NUM_GARBAGE_BYTES = "num_garbage_bytes"
CONF_UNKNOWN_COMMANDS = "unknown_commands"
CONF_UNKNOWN_EXTENDED_COMMANDS = "unknown_extended_commands"
CONF_PAIRING_MODE = "pairing_mode"
CONF_PRODUCT = "product"
CONF_UYAT_ID = "uyat_id"

uyat_ns = cg.esphome_ns.namespace("uyat")
UyatDatapointType = uyat_ns.enum("UyatDatapointType")
Uyat = uyat_ns.class_("Uyat", cg.Component, uart.UARTDevice)

DPTYPE_ANY = "any"
DPTYPE_RAW = "raw"
DPTYPE_BOOL = "bool"
DPTYPE_INT = "int"
DPTYPE_UINT = "uint"
DPTYPE_STRING = "string"
DPTYPE_ENUM = "enum"
DPTYPE_BITMASK = "bitmask"

DATAPOINT_TYPES = {
    DPTYPE_ANY: uyat_ns.struct("UyatDatapoint"),
    DPTYPE_RAW: cg.std_vector.template(cg.uint8),
    DPTYPE_BOOL: cg.bool_,
    DPTYPE_INT: cg.int_,
    DPTYPE_UINT: cg.uint32,
    DPTYPE_STRING: cg.std_string,
    DPTYPE_ENUM: cg.uint8,
    DPTYPE_BITMASK: cg.uint32,
}

DATAPOINT_TRIGGERS = {
    DPTYPE_ANY: uyat_ns.class_(
        "UyatDatapointUpdateTrigger",
        automation.Trigger.template(DATAPOINT_TYPES[DPTYPE_ANY]),
    ),
    DPTYPE_RAW: uyat_ns.class_(
        "UyatRawDatapointUpdateTrigger",
        automation.Trigger.template(DATAPOINT_TYPES[DPTYPE_RAW]),
    ),
    DPTYPE_BOOL: uyat_ns.class_(
        "UyatBoolDatapointUpdateTrigger",
        automation.Trigger.template(DATAPOINT_TYPES[DPTYPE_BOOL]),
    ),
    DPTYPE_INT: uyat_ns.class_(
        "UyatIntDatapointUpdateTrigger",
        automation.Trigger.template(DATAPOINT_TYPES[DPTYPE_INT]),
    ),
    DPTYPE_UINT: uyat_ns.class_(
        "UyatUIntDatapointUpdateTrigger",
        automation.Trigger.template(DATAPOINT_TYPES[DPTYPE_UINT]),
    ),
    DPTYPE_STRING: uyat_ns.class_(
        "UyatStringDatapointUpdateTrigger",
        automation.Trigger.template(DATAPOINT_TYPES[DPTYPE_STRING]),
    ),
    DPTYPE_ENUM: uyat_ns.class_(
        "UyatEnumDatapointUpdateTrigger",
        automation.Trigger.template(DATAPOINT_TYPES[DPTYPE_ENUM]),
    ),
    DPTYPE_BITMASK: uyat_ns.class_(
        "UyatBitmaskDatapointUpdateTrigger",
        automation.Trigger.template(DATAPOINT_TYPES[DPTYPE_BITMASK]),
    ),
}


def assign_declare_id(value):
    value = value.copy()
    value[CONF_TRIGGER_ID] = cv.declare_id(
        DATAPOINT_TRIGGERS[value[CONF_DATAPOINT_TYPE]]
    )(value[CONF_TRIGGER_ID].id)
    return value


UYAT_DIAGNOSTIC_SENSORS_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_PRODUCT): esphome_text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_NUM_GARBAGE_BYTES): esphome_sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_UNKNOWN_COMMANDS): esphome_text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_UNKNOWN_EXTENDED_COMMANDS): esphome_text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_PAIRING_MODE): esphome_text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Uyat),
            cv.Optional(CONF_DIAGNOSTICS): UYAT_DIAGNOSTIC_SENSORS_SCHEMA,
            cv.Optional(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
            cv.Optional(CONF_IGNORE_MCU_UPDATE_ON_DATAPOINTS): cv.ensure_list(
                cv.uint8_t
            ),
            cv.Optional(CONF_STATUS_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_ON_DATAPOINT_UPDATE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        DATAPOINT_TRIGGERS[DPTYPE_ANY]
                    ),
                    cv.Required(CONF_SENSOR_DATAPOINT): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_ANY): cv.one_of(
                        *DATAPOINT_TRIGGERS, lower=True
                    ),
                },
                extra_validators=assign_declare_id,
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    if CONF_TIME_ID in config:
        time_ = await cg.get_variable(config[CONF_TIME_ID])
        cg.add(var.set_time_id(time_))
    if CONF_STATUS_PIN in config:
        status_pin_ = await cg.gpio_pin_expression(config[CONF_STATUS_PIN])
        cg.add(var.set_status_pin(status_pin_))
    if CONF_IGNORE_MCU_UPDATE_ON_DATAPOINTS in config:
        for dp in config[CONF_IGNORE_MCU_UPDATE_ON_DATAPOINTS]:
            cg.add(var.add_ignore_mcu_update_on_datapoints(dp))
    for conf in config.get(CONF_ON_DATAPOINT_UPDATE, []):
        trigger = cg.new_Pvariable(
            conf[CONF_TRIGGER_ID], var, conf[CONF_SENSOR_DATAPOINT]
        )
        await automation.build_automation(
            trigger, [(DATAPOINT_TYPES[conf[CONF_DATAPOINT_TYPE]], "x")], conf
        )
    if diagnostics_config := config.get(CONF_DIAGNOSTICS):
        if CONF_PRODUCT in diagnostics_config:
            tsens = await esphome_text_sensor.new_text_sensor(
                diagnostics_config[CONF_PRODUCT]
            )
            cg.add(var.set_product_text_sensor(tsens))
        if CONF_NUM_GARBAGE_BYTES in diagnostics_config:
            sens = await esphome_sensor.new_sensor(
                diagnostics_config[CONF_NUM_GARBAGE_BYTES]
            )
            cg.add(var.set_num_garbage_bytes_sensor(sens))
        if CONF_UNKNOWN_COMMANDS in diagnostics_config:
            tsens = await esphome_text_sensor.new_text_sensor(
                diagnostics_config[CONF_UNKNOWN_COMMANDS]
            )
            cg.add(var.set_unknown_commands_text_sensor(tsens))
        if CONF_UNKNOWN_EXTENDED_COMMANDS in diagnostics_config:
            tsens = await esphome_text_sensor.new_text_sensor(
                diagnostics_config[CONF_UNKNOWN_EXTENDED_COMMANDS]
            )
            cg.add(var.set_unknown_extended_commands_text_sensor(tsens))
        if CONF_PAIRING_MODE in diagnostics_config:
            tsens = await esphome_text_sensor.new_text_sensor(
                diagnostics_config[CONF_PAIRING_MODE]
            )
            cg.add(var.set_pairing_mode_text_sensor(tsens))
