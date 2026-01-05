import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import CONF_NUMBER

from .. import (
   CONF_UYAT_ID,
   CONF_DATAPOINT,
   CONF_DATAPOINT_TYPE,
   uyat_ns,
   Uyat,
   UyatDatapoint,
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

UyatButton = uyat_ns.class_("UyatButton", button.Button, cg.Component)

BUTTON_DP_TYPES = [
    DPTYPE_BOOL,
    DPTYPE_UINT,
    DPTYPE_ENUM,
]

CONFIG_BUTTON_EXTENDED_DATAPOINT_SCHEMA = cv.typed_schema(
    {
        DPTYPE_BOOL: cv.Schema(
        {
            cv.Required(CONF_NUMBER): cv.uint8_t,
            cv.Required(CONF_TRIGGER_PAYLOAD): cv.boolean,
        }),
        DPTYPE_UINT: cv.Schema(
        {
            cv.Required(CONF_NUMBER): cv.uint8_t,
            cv.Required(CONF_TRIGGER_PAYLOAD): cv.uint32_t,
        }),
        DPTYPE_ENUM: cv.Schema(
        {
            cv.Required(CONF_NUMBER): cv.uint8_t,
            cv.Required(CONF_TRIGGER_PAYLOAD): cv.uint8_t,
        }),
    },
    default_type = DPTYPE_BOOL,
    key = CONF_DATAPOINT_TYPE,
    lower=True,
)

CONFIG_SCHEMA = cv.All(
    button.button_schema(UyatButton)
    .extend(
        {
            cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
            cv.Required(CONF_DATAPOINT): CONFIG_BUTTON_EXTENDED_DATAPOINT_SCHEMA,
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
)

async def to_code(config):
    var = await button.new_button(config)
    await cg.register_component(var, config)

    paren = await cg.get_variable(config[CONF_UYAT_ID])
    cg.add(var.set_uyat_parent(paren))

    dp_config = config.get(CONF_DATAPOINT)
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
