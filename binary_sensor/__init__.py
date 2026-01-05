import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import CONF_NUMBER

from .. import (
   CONF_UYAT_ID,
   CONF_DATAPOINT,
   CONF_DATAPOINT_TYPE,
   uyat_ns,
   Uyat,
   DPTYPE_BOOL,
   DPTYPE_UINT,
   DPTYPE_ENUM,
   DPTYPE_BITMAP,
   DPTYPE_DETECT,
   matching_datapoint_from_config
)

DEPENDENCIES = ["uyat"]
CODEOWNERS = ["@szupi_ipuzs"]

CONF_BIT_NUMBER = "bit_number"

UyatBinarySensor = uyat_ns.class_(
    "UyatBinarySensor", binary_sensor.BinarySensor, cg.Component
)

BINARY_SENSOR_DP_TYPES = [
    DPTYPE_DETECT,
    DPTYPE_BOOL,
    DPTYPE_UINT,
    DPTYPE_ENUM,
    DPTYPE_BITMAP,
]

def _validate(config):
    dp_config = config[CONF_DATAPOINT]
    if not isinstance(dp_config, dict):
        if CONF_BIT_NUMBER in dp_config:
            raise cv.Invalid(f"{CONF_BIT_NUMBER} requires setting datapoint type to {DPTYPE_BITMAP}")
        return config

    dp_type = dp_config.get(CONF_DATAPOINT_TYPE)
    if dp_type == DPTYPE_BITMAP:
        if CONF_BIT_NUMBER not in dp_config:
            raise cv.Invalid(f"{CONF_BIT_NUMBER} is required for datapoint type {DPTYPE_BITMAP}")
    else:
        if CONF_BIT_NUMBER in dp_config:
            raise cv.Invalid(f"{CONF_BIT_NUMBER} requires setting datapoint type to {DPTYPE_BITMAP}")

    return config


CONFIG_SCHEMA = cv.All(
    binary_sensor.binary_sensor_schema(UyatBinarySensor)
    .extend(
        {
            cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
            cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_DETECT): cv.one_of(
                        *BINARY_SENSOR_DP_TYPES, lower=True
                    ),
                    cv.Optional(CONF_BIT_NUMBER): cv.int_range(min=1, max=32),
                })
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
    _validate
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)

    paren = await cg.get_variable(config[CONF_UYAT_ID])
    cg.add(var.set_uyat_parent(paren))

    dp_config = config[CONF_DATAPOINT]
    bit_number = 0
    if isinstance(dp_config, dict):
        if CONF_BIT_NUMBER in dp_config:
            bit_number = dp_config[CONF_BIT_NUMBER]-1
    cg.add(var.configure(await matching_datapoint_from_config(dp_config, BINARY_SENSOR_DP_TYPES), bit_number))
