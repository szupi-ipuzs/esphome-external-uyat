import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import CONF_SENSOR_DATAPOINT, CONF_NUMBER

from .. import CONF_UYAT_ID, CONF_DATAPOINT_TYPE, uyat_ns, Uyat, UyatDatapointType, MatchingDatapoint, DPTYPE_BOOL, DPTYPE_UINT, DPTYPE_ENUM

DEPENDENCIES = ["uyat"]
CODEOWNERS = ["@jesserockz"]

UyatBinarySensor = uyat_ns.class_(
    "UyatBinarySensor", binary_sensor.BinarySensor, cg.Component
)

BINARY_SENSOR_DP_TYPES = {
    DPTYPE_BOOL: UyatDatapointType.BOOLEAN,
    DPTYPE_UINT: UyatDatapointType.INTEGER,
    DPTYPE_ENUM: UyatDatapointType.ENUM,
}

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
        struct = cg.StructInitializer(
            MatchingDatapoint, ("number", dp_config)
        )
        cg.add(var.set_matching_dp(struct))
    else:
        struct = cg.StructInitializer(
            MatchingDatapoint, ("number", dp_config[CONF_NUMBER]), ("type", BINARY_SENSOR_DP_TYPES[dp_config[CONF_DATAPOINT_TYPE]])
        )
        cg.add(var.set_matching_dp(struct))
