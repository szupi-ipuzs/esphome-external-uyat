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
       CONF_TYPE,
       CONF_NUMBER,
       CONF_VALUE,
       ENTITY_CATEGORY_DIAGNOSTIC,
       STATE_CLASS_MEASUREMENT,
)

DEPENDENCIES = ["uart"]

CONF_IGNORE_MCU_UPDATE_ON_DATAPOINTS = "ignore_mcu_update_on_datapoints"

CONF_REPORT_AP_NAME = "report_ap_name"
CONF_ON_DATAPOINT_UPDATE = "on_datapoint_update"
CONF_DATAPOINT = "datapoint"
CONF_DATAPOINT_TYPE = "datapoint_type"
CONF_STATUS_PIN = "status_pin"
CONF_DIAGNOSTICS = "diagnostics"
CONF_NUM_GARBAGE_BYTES = "num_garbage_bytes"
CONF_UNKNOWN_COMMANDS = "unknown_commands"
CONF_UNKNOWN_EXTENDED_COMMANDS = "unknown_extended_commands"
CONF_UNHANDLED_DATAPOINTS = "unhandled_datapoints"
CONF_PAIRING_MODE = "pairing_mode"
CONF_PRODUCT = "product"
CONF_UYAT_ID = "uyat_id"

uyat_ns = cg.esphome_ns.namespace("uyat")
UyatDatapointType = uyat_ns.enum("UyatDatapointType", is_class=True)
BoolDatapointValue = uyat_ns.class_("BoolDatapointValue")
UIntDatapointValue = uyat_ns.class_("UIntDatapointValue")
EnumDatapointValue = uyat_ns.class_("EnumDatapointValue")
UyatDatapoint = uyat_ns.class_("UyatDatapoint")
FactoryResetType = uyat_ns.enum("FactoryResetType")
Uyat = uyat_ns.class_("Uyat", cg.Component, uart.UARTDevice)
MatchingDatapoint = uyat_ns.class_("MatchingDatapoint")
UyatFactoryResetAction = uyat_ns.class_("FactoryResetAction", automation.Action)

FACTORY_RESET_TYPES = {
    "HW": FactoryResetType.BY_HW,
    "APP": FactoryResetType.BY_APP,
    "APP_WIPE": FactoryResetType.BY_APP_WIPE,
}

DPTYPE_ANY = "any"
DPTYPE_DETECT = "detect"
DPTYPE_RAW = "raw"
DPTYPE_BOOL = "bool"
DPTYPE_UINT = "value"
DPTYPE_STRING = "string"
DPTYPE_ENUM = "enum"
DPTYPE_BITMAP = "bitmap"

DP_TYPES_TRANSLATED = {
    DPTYPE_RAW: UyatDatapointType.RAW,
    DPTYPE_BOOL: UyatDatapointType.BOOLEAN,
    DPTYPE_ENUM: UyatDatapointType.ENUM,
    DPTYPE_UINT: UyatDatapointType.INTEGER,
    DPTYPE_STRING: UyatDatapointType.STRING,
    DPTYPE_BITMAP: UyatDatapointType.BITMAP,
}

async def translate_dp_types(dp_types: list):
    cpp_types_enum = []
    for dp_type in dp_types:
        if not dp_type == DPTYPE_DETECT:
            cpp_types_enum.append(DP_TYPES_TRANSLATED[dp_type])

    return cg.ArrayInitializer(*cpp_types_enum)

async def matching_datapoint_from_config(dp_config, allowed_types):
    if not isinstance(dp_config, dict):
        # short form, translate into full
        full_config = {CONF_NUMBER: dp_config, CONF_DATAPOINT_TYPE: allowed_types["default"]}
    else:
        full_config = dp_config

    dp_type = full_config.get(CONF_DATAPOINT_TYPE)
    # raise if selected type is not in the allowed list
    if dp_type not in allowed_types["allowed"]:
        raise ValueError(f"{dp_type} is not allowed")

    if dp_type == DPTYPE_DETECT:
        return cg.StructInitializer(
            MatchingDatapoint,
            ("number", full_config[CONF_NUMBER]),
            ("types", await translate_dp_types(allowed_types["allowed"]))
        )
    else:
        return cg.StructInitializer(
            MatchingDatapoint,
            ("number", full_config[CONF_NUMBER]),
            ("types", await translate_dp_types([dp_type]))
        )

CPP_DATAPOINT_TYPES = {
    DPTYPE_ANY: uyat_ns.struct("UyatDatapoint"),
    DPTYPE_RAW: cg.std_vector.template(cg.uint8),
    DPTYPE_BOOL: cg.bool_,
    DPTYPE_UINT: cg.uint32,
    DPTYPE_STRING: cg.std_string,
    DPTYPE_ENUM: cg.uint8,
    DPTYPE_BITMAP: cg.uint32,
}

DATAPOINT_TRIGGERS = {
    DPTYPE_ANY: uyat_ns.class_(
        "UyatDatapointUpdateTrigger",
        automation.Trigger.template(CPP_DATAPOINT_TYPES[DPTYPE_ANY]),
    ),
    DPTYPE_RAW: uyat_ns.class_(
        "UyatRawDatapointUpdateTrigger",
        automation.Trigger.template(CPP_DATAPOINT_TYPES[DPTYPE_RAW]),
    ),
    DPTYPE_BOOL: uyat_ns.class_(
        "UyatBoolDatapointUpdateTrigger",
        automation.Trigger.template(CPP_DATAPOINT_TYPES[DPTYPE_BOOL]),
    ),
    DPTYPE_UINT: uyat_ns.class_(
        "UyatUIntDatapointUpdateTrigger",
        automation.Trigger.template(CPP_DATAPOINT_TYPES[DPTYPE_UINT]),
    ),
    DPTYPE_STRING: uyat_ns.class_(
        "UyatStringDatapointUpdateTrigger",
        automation.Trigger.template(CPP_DATAPOINT_TYPES[DPTYPE_STRING]),
    ),
    DPTYPE_ENUM: uyat_ns.class_(
        "UyatEnumDatapointUpdateTrigger",
        automation.Trigger.template(CPP_DATAPOINT_TYPES[DPTYPE_ENUM]),
    ),
    DPTYPE_BITMAP: uyat_ns.class_(
        "UyatBitmapDatapointUpdateTrigger",
        automation.Trigger.template(CPP_DATAPOINT_TYPES[DPTYPE_BITMAP]),
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
        cv.Optional(CONF_UNHANDLED_DATAPOINTS): esphome_text_sensor.text_sensor_schema(
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
            cv.Optional(CONF_REPORT_AP_NAME, default="smartlife"): cv.All(
                cv.string, cv.Length(max=32)
            ),
            cv.Optional(CONF_IGNORE_MCU_UPDATE_ON_DATAPOINTS): cv.ensure_list(
                cv.uint8_t
            ),
            cv.Optional(CONF_STATUS_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_ON_DATAPOINT_UPDATE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        DATAPOINT_TRIGGERS[DPTYPE_ANY]
                    ),
                    cv.Required(CONF_DATAPOINT): cv.uint8_t,
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
    cg.add_define("UYAT_REPORT_AP_NAME", cg.safe_exp(f'"{config[CONF_REPORT_AP_NAME]}"'))
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
            conf[CONF_TRIGGER_ID], var, conf[CONF_DATAPOINT]
        )
        await automation.build_automation(
            trigger, [(CPP_DATAPOINT_TYPES[conf[CONF_DATAPOINT_TYPE]], "x")], conf
        )
    if diagnostics_config := config.get(CONF_DIAGNOSTICS):
        cg.add_define("UYAT_DIAGNOSTICS_ENABLED")
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
        if CONF_UNHANDLED_DATAPOINTS in diagnostics_config:
            tsens = await esphome_text_sensor.new_text_sensor(
                diagnostics_config[CONF_UNHANDLED_DATAPOINTS]
            )
            cg.add(var.set_unhandled_datapoints_text_sensor(tsens))
        if CONF_PAIRING_MODE in diagnostics_config:
            tsens = await esphome_text_sensor.new_text_sensor(
                diagnostics_config[CONF_PAIRING_MODE]
            )
            cg.add(var.set_pairing_mode_text_sensor(tsens))



UYAT_ACTION_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(Uyat),
    }
)

UYAT_FACTORY_RESET_SCHEMA = automation.maybe_simple_id(
    UYAT_ACTION_SCHEMA.extend(
        cv.Schema(
            {
                cv.Optional(CONF_TYPE, default="HW"): cv.enum(FACTORY_RESET_TYPES, upper=True),
            }
        )
    )
)


@automation.register_action(
    "uyat.factory_reset", UyatFactoryResetAction, UYAT_FACTORY_RESET_SCHEMA
)
async def uyat_factory_reset_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    cg.add(var.set_reset_type(config[CONF_TYPE]))
    return var
