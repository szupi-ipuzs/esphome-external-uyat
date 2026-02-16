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

UyatBinarySensorConfig = uyat_ns.struct("UyatBinarySensor::Config")

BINARY_SENSOR_DP_TYPES = {
   "allowed": [
        DPTYPE_DETECT,
        DPTYPE_BOOL,
        DPTYPE_UINT,
        DPTYPE_ENUM,
        DPTYPE_BITMAP,
    ],
   "default": DPTYPE_DETECT,
}

CONFIG_SCHEMA = cv.All(
    binary_sensor.binary_sensor_schema(UyatBinarySensor)
    .extend(
        {
            cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
            cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=BINARY_SENSOR_DP_TYPES["default"]): cv.one_of(
                        *BINARY_SENSOR_DP_TYPES["allowed"], lower=True
                    ),
                })
            ),
            cv.Optional(CONF_BIT_NUMBER): cv.int_range(min=1, max=32),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    if CONF_BIT_NUMBER in config:
        bit_number = config[CONF_BIT_NUMBER]-1
    else:
        bit_number = cg.RawExpression("{}")

    config_struct = cg.StructInitializer(UyatBinarySensorConfig,
                                         ("sensor_dp", await matching_datapoint_from_config(config[CONF_DATAPOINT], BINARY_SENSOR_DP_TYPES)),
                                         ("bit_number", bit_number))

    var = await binary_sensor.new_binary_sensor(config, await cg.get_variable(config[CONF_UYAT_ID]), config_struct)
    await cg.register_component(var, config)

