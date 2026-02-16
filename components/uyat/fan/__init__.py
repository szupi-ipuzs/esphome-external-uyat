import esphome.codegen as cg
from esphome.components import fan
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_NUMBER, CONF_INVERTED, CONF_MIN_VALUE, CONF_MAX_VALUE

from .. import CONF_UYAT_ID, CONF_DATAPOINT, CONF_DATAPOINT_TYPE, Uyat, uyat_ns, DPTYPE_BOOL, DPTYPE_ENUM, DPTYPE_UINT, DPTYPE_DETECT, matching_datapoint_from_config

DEPENDENCIES = ["uyat"]

CONF_SPEED = "speed"
CONF_SWITCH = "switch"
CONF_OSCILLATION = "oscillation"
CONF_SPEED = "speed"
CONF_DIRECTION = "direction"

UyatFan = uyat_ns.class_("UyatFan", cg.Component, fan.Fan)
UyatFanSpeedConfig = uyat_ns.struct("UyatFan::SpeedConfig")
UyatFanSwitchConfig = uyat_ns.struct("UyatFan::SwitchConfig")
UyatFanOscillationConfig = uyat_ns.struct("UyatFan::OscillationConfig")
UyatFanDirectionConfig = uyat_ns.struct("UyatFan::DirectionConfig")
UyatFanConfig = uyat_ns.struct("UyatFan::Config")

SPEED_DP_TYPES = {
    "allowed": [
        DPTYPE_DETECT,
        DPTYPE_ENUM,
        DPTYPE_UINT,
    ],
    "default": DPTYPE_UINT
}

OSCILLATION_DP_TYPES = {
    "allowed": [
        DPTYPE_DETECT,
        DPTYPE_BOOL,
        DPTYPE_ENUM,
    ],
    "default": DPTYPE_BOOL
}

SWITCH_DP_TYPES = {
    "allowed": [
        DPTYPE_DETECT,
        DPTYPE_BOOL,
        DPTYPE_UINT,
        DPTYPE_ENUM,
    ],
    "default": DPTYPE_BOOL
}

DIRECTION_DP_TYPES = {
    "allowed": [
        DPTYPE_DETECT,
        DPTYPE_BOOL,
        DPTYPE_UINT,
        DPTYPE_ENUM,
    ],
    "default": DPTYPE_BOOL
}

SPEED_CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
            cv.Schema(
            {
                cv.Required(CONF_NUMBER): cv.uint8_t,
                cv.Optional(CONF_DATAPOINT_TYPE, default=SPEED_DP_TYPES["default"]): cv.one_of(
                    *SPEED_DP_TYPES["allowed"], lower=True
                )
            })
        ),
        cv.Optional(CONF_MIN_VALUE, default=1): cv.uint32_t,
        cv.Optional(CONF_MAX_VALUE, default=100): cv.uint32_t,
    }
)

OSCILLATION_CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
            cv.Schema(
            {
                cv.Required(CONF_NUMBER): cv.uint8_t,
                cv.Optional(CONF_DATAPOINT_TYPE, default=OSCILLATION_DP_TYPES["default"]): cv.one_of(
                    *OSCILLATION_DP_TYPES["allowed"], lower=True
                )
            })
        ),
        cv.Optional(CONF_INVERTED, default=False): cv.boolean,
    }
)

SWITCH_CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
            cv.Schema(
            {
                cv.Required(CONF_NUMBER): cv.uint8_t,
                cv.Optional(CONF_DATAPOINT_TYPE, default=SWITCH_DP_TYPES["default"]): cv.one_of(
                    SWITCH_DP_TYPES["allowed"], lower=True
                )
            })
        ),
        cv.Optional(CONF_INVERTED, default=False): cv.boolean,
    }
)

DIRECTION_CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
            cv.Schema(
            {
                cv.Required(CONF_NUMBER): cv.uint8_t,
                cv.Optional(CONF_DATAPOINT_TYPE, default=DIRECTION_DP_TYPES["default"]): cv.one_of(
                    *DIRECTION_DP_TYPES["allowed"], lower=True
                )
            })
        ),
        cv.Optional(CONF_INVERTED, default=False): cv.boolean,
    }
)

CONFIG_SCHEMA = cv.All(
    fan.fan_schema(UyatFan)
    .extend(
        {
            cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
            cv.Optional(CONF_OSCILLATION): OSCILLATION_CONFIG_SCHEMA,
            cv.Optional(CONF_SPEED): SPEED_CONFIG_SCHEMA,
            cv.Optional(CONF_SWITCH): SWITCH_CONFIG_SCHEMA,
            cv.Optional(CONF_DIRECTION): DIRECTION_CONFIG_SCHEMA,
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
    cv.has_at_least_one_key(CONF_SPEED, CONF_SWITCH),
)


async def to_code(config):
    if CONF_SPEED in config:
        speed_config = config[CONF_SPEED]
        speed_conf_struct = cg.StructInitializer(UyatFanSpeedConfig,
                                                 ("matching_dp", await matching_datapoint_from_config(speed_config[CONF_DATAPOINT], SPEED_DP_TYPES)),
                                                 ("min_value", speed_config[CONF_MIN_VALUE]),
                                                 ("max_value", speed_config[CONF_MAX_VALUE]))
    else:
        speed_conf_struct = cg.RawExpression("{}")

    if CONF_SWITCH in config:
        switch_config = config[CONF_SWITCH]
        switch_conf_struct = cg.StructInitializer(UyatFanSwitchConfig,
                                                  ("matching_dp", await matching_datapoint_from_config(switch_config[CONF_DATAPOINT], SWITCH_DP_TYPES)),
                                                  ("inverted", switch_config[CONF_INVERTED]))
    else:
        switch_conf_struct = cg.RawExpression("{}")
    if CONF_OSCILLATION in config:
        oscillation_config = config[CONF_OSCILLATION]
        oscillation_conf_struct = cg.StructInitializer(UyatFanOscillationConfig,
                                                       ("matching_dp", await matching_datapoint_from_config(oscillation_config[CONF_DATAPOINT], OSCILLATION_DP_TYPES)),
                                                       ("inverted", oscillation_config[CONF_INVERTED]))
    else:
        oscillation_conf_struct = cg.RawExpression("{}")

    if CONF_DIRECTION in config:
        direction_config = config[CONF_DIRECTION]
        direction_conf_struct = cg.StructInitializer(UyatFanDirectionConfig,
                                                     ("matching_dp", await matching_datapoint_from_config(direction_config[CONF_DATAPOINT], DIRECTION_DP_TYPES)),
                                                     ("inverted", direction_config[CONF_INVERTED]))
    else:
        direction_conf_struct = cg.RawExpression("{}")

    final_conf_struct = cg.StructInitializer(UyatFanConfig,
                                        ("speed_config", speed_conf_struct),
                                        ("switch_config", switch_conf_struct),
                                        ("oscillation_config", oscillation_conf_struct),
                                        ("direction_config", direction_conf_struct))

    var = cg.new_Pvariable(config[CONF_ID], await cg.get_variable(config[CONF_UYAT_ID]), final_conf_struct)
    await cg.register_component(var, config)
    await fan.register_fan(var, config)
