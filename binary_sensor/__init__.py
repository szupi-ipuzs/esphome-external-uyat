import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import CONF_SENSOR_DATAPOINT, CONF_NUMBER

from .. import CONF_UYAT_ID, CONF_DATAPOINT_TYPE, uyat_ns, Uyat, DPTYPE_BOOL, DPTYPE_UINT, DPTYPE_ENUM, DPTYPE_BITMASK

DEPENDENCIES = ["uyat"]
CODEOWNERS = ["@szupi_ipuzs"]

CONF_BIT_NUMBER = "bit_number"

UyatBinarySensor = uyat_ns.class_(
    "UyatBinarySensor", binary_sensor.BinarySensor, cg.Component
)

BINARY_SENSOR_DP_TYPES = [
    DPTYPE_BOOL,
    DPTYPE_UINT,
    DPTYPE_ENUM,
    DPTYPE_BITMASK,
]

CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema(UyatBinarySensor)
    .extend(
        {
            cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
            cv.Required(CONF_SENSOR_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_BOOL): cv.one_of(
                        *BINARY_SENSOR_DP_TYPES, lower=True
                    ),
                    cv.Optional(CONF_BIT_NUMBER): cv.uint8_t
                })
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)

    paren = await cg.get_variable(config[CONF_UYAT_ID])
    cg.add(var.set_uyat_parent(paren))

    dp_config = config[CONF_SENSOR_DATAPOINT]
    if not isinstance(dp_config, dict):
        cg.add(var.configure_bool_dp(dp_config))
    else:
        if dp_config[CONF_DATAPOINT_TYPE]==DPTYPE_BITMASK:
            cg.add(var.configure_bitmask_dp(dp_config[CONF_NUMBER], cg.uint8(dp_config[CONF_BIT_NUMBER]-1)))
        elif dp_config[CONF_DATAPOINT_TYPE]==DPTYPE_BOOL:
            cg.add(var.configure_bool_dp(dp_config[CONF_NUMBER]))
        elif dp_config[CONF_DATAPOINT_TYPE]==DPTYPE_UINT:
            cg.add(var.configure_uint_dp(dp_config[CONF_NUMBER]))
        elif dp_config[CONF_DATAPOINT_TYPE]==DPTYPE_ENUM:
            cg.add(var.configure_enum_dp(dp_config[CONF_NUMBER]))
