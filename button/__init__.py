import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import CONF_NUMBER
from esphome.types import ConfigType

from .. import (
   CONF_UYAT_ID,
   CONF_DATAPOINT_TYPE,
   uyat_ns,
   Uyat,
   UyatDatapoint,
   UyatDatapointType,
   BoolDatapointValue,
   UIntDatapointValue,
   EnumDatapointValue,
   DPTYPE_BOOL,
   DPTYPE_UINT,
   DPTYPE_ENUM
)

DEPENDENCIES = ["uyat"]
CODEOWNERS = ["@szupi_ipuzs"]

CONF_TRIGGER_PAYLOAD = "trigger_payload"
CONF_BUTTON_DATAPOINT = "button_datapoint"

UyatButton = uyat_ns.class_("UyatButton", button.Button, cg.Component)

BUTTON_DP_TYPES = [
    DPTYPE_BOOL,
    DPTYPE_UINT,
    DPTYPE_ENUM,
]

def _validate_extended_datapoint_schema(config):
    if CONF_TRIGGER_PAYLOAD in config:
        payload_config = config[CONF_TRIGGER_PAYLOAD]
        if not CONF_DATAPOINT_TYPE in config:
            raise cv.Invalid(
                "datapoint_type must be set when defining payload"
            )
        dp_type = config[CONF_DATAPOINT_TYPE]
        if dp_type==DPTYPE_BOOL:
            if not cv.boolean(payload_config):
                raise cv.Invalid(
                    "payload must contain a boolean value"
                )
        if dp_type==DPTYPE_UINT:
            if not cv.uint32_t(payload_config):
                raise cv.Invalid(
                    "payload must contain an 32bit unsigned integer value"
                )
        if dp_type==DPTYPE_ENUM:
            if not cv.uint8_t(payload_config):
                raise cv.Invalid(
                    "payload must contain an 8bit unsigned integer value"
                )
    return config


CONFIG_BUTTON_EXTENDED_DATAPOINT_SCHEMA = cv.All(
    cv.Schema(
    {
        cv.Required(CONF_NUMBER): cv.uint8_t,
        cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_BOOL): cv.one_of(
            *BUTTON_DP_TYPES, lower=True
        ),
        cv.Optional(CONF_TRIGGER_PAYLOAD): cv.Any(
            cv.boolean,
            cv.uint32_t,
            cv.uint8_t
        ),
    }),
    _validate_extended_datapoint_schema
)

CONFIG_SCHEMA = cv.All(
    button.button_schema(UyatButton)
    .extend(
        {
            cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
            cv.Required(CONF_BUTTON_DATAPOINT): cv.Any(
                cv.uint8_t,
                CONFIG_BUTTON_EXTENDED_DATAPOINT_SCHEMA
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
)

async def to_code(config):
    var = await button.new_button(config)
    await cg.register_component(var, config)

    paren = await cg.get_variable(config[CONF_UYAT_ID])
    cg.add(var.set_uyat_parent(paren))

    dp_config = config.get(CONF_BUTTON_DATAPOINT)
    if not isinstance(dp_config, dict):
        UyatDatapointValue = cg.StructInitializer(
            BoolDatapointValue, ("value", True)
        )
        UyatDatapointStruct = cg.StructInitializer(
            UyatDatapoint, ("number", dp_config), ("value", UyatDatapointValue)
        )
        cg.add(var.set_trigger_payload(UyatDatapointStruct))
    else:
        dp_type = dp_config.get(CONF_DATAPOINT_TYPE, None)
        payload_config = dp_config.get(CONF_TRIGGER_PAYLOAD, None)
        if dp_type==DPTYPE_BOOL:
            UyatDatapointValue = cg.StructInitializer(
                BoolDatapointValue, ("value", payload_config)
            )
        if dp_type==DPTYPE_UINT:
            UyatDatapointValue = cg.StructInitializer(
                UIntDatapointValue, ("value", payload_config)
            )
        if dp_type==DPTYPE_ENUM:
            UyatDatapointValue = cg.StructInitializer(
                EnumDatapointValue, ("value", payload_config)
            )
        UyatDatapointStruct = cg.StructInitializer(
            UyatDatapoint, ("number", dp_config[CONF_NUMBER]), ("value", UyatDatapointValue)
        )
        cg.add(var.set_trigger_payload(UyatDatapointStruct))
