import esphome.codegen as cg
from esphome.components import light
import esphome.config_validation as cv
from esphome.const import (
    CONF_COLD_WHITE_COLOR_TEMPERATURE,
    CONF_COLOR_INTERLOCK,
    CONF_DEFAULT_TRANSITION_LENGTH,
    CONF_GAMMA_CORRECT,
    CONF_MAX_VALUE,
    CONF_MIN_VALUE,
    CONF_OUTPUT_ID,
    CONF_WARM_WHITE_COLOR_TEMPERATURE,
    CONF_NUMBER,
    CONF_INVERTED,
    CONF_TYPE
)

from .. import CONF_UYAT_ID, CONF_DATAPOINT, CONF_DATAPOINT_TYPE, Uyat, uyat_ns, DPTYPE_BOOL, DPTYPE_UINT, DPTYPE_ENUM, DPTYPE_STRING, DPTYPE_DETECT, matching_datapoint_from_config

DEPENDENCIES = ["uyat"]

CONF_MIN_VALUE_DATAPOINT = "min_value_datapoint"
CONF_SWITCH = "switch"
CONF_DIMMER = "dimmer"
CONF_COLOR = "color"
CONF_WHITE_TEMPERATURE = "white_temperature"

UyatColorType = uyat_ns.enum("UyatColorType", is_class=True)
UyatLightConfigDimmer = uyat_ns.struct("UyatLight::ConfigDimmer")
UyatLightConfigSwitch = uyat_ns.struct("UyatLight::ConfigSwitch")
UyatLightConfigColor = uyat_ns.struct("UyatLight::ConfigColor")
UyatLightConfigWhiteTemperature = uyat_ns.struct("UyatLight::ConfigWhiteTemperature")
UyatLightConfig = uyat_ns.struct("UyatLight::Config")

COLOR_TYPES = {
    "RGB": UyatColorType.RGB,
    "HSV": UyatColorType.HSV,
    "RGBHSV": UyatColorType.RGBHSV,
}

DIMMER_DP_TYPES = {
    "allowed": [
        DPTYPE_DETECT,
        DPTYPE_UINT,
        DPTYPE_ENUM,
    ],
    "default": DPTYPE_UINT
}

MIN_VALUE_DP_TYPES = {
    "allowed": [
        DPTYPE_DETECT,
        DPTYPE_UINT,
        DPTYPE_ENUM,
    ],
    "default": DPTYPE_UINT
}

SWITCH_DP_TYPES = {
    "allowed": [
        DPTYPE_DETECT,
        DPTYPE_BOOL,
        DPTYPE_UINT,
        DPTYPE_ENUM
    ],
    "default": DPTYPE_BOOL
}

COLOR_DP_TYPES = {
    "allowed": [
        DPTYPE_STRING
    ],
    "default": DPTYPE_STRING
}

WHITE_TEMPERATURE_DP_TYPES = {
    "allowed": [
        DPTYPE_DETECT,
        DPTYPE_UINT,
        DPTYPE_ENUM,
    ],
    "default": DPTYPE_UINT
}


UyatLight = uyat_ns.class_("UyatLight", light.LightOutput, cg.Component)

SWITCH_CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
            cv.Schema(
            {
                cv.Required(CONF_NUMBER): cv.uint8_t,
                cv.Optional(CONF_DATAPOINT_TYPE, default=SWITCH_DP_TYPES["default"]): cv.one_of(
                    *SWITCH_DP_TYPES["allowed"], lower=True
                )
            })
        ),
        cv.Optional(CONF_INVERTED, default=False): cv.boolean,
    }
)

DIMMER_CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
            cv.Schema(
            {
                cv.Required(CONF_NUMBER): cv.uint8_t,
                cv.Optional(CONF_DATAPOINT_TYPE, default=DIMMER_DP_TYPES["default"]): cv.one_of(
                    *DIMMER_DP_TYPES["allowed"], lower=True
                )
            })
        ),
        cv.Optional(CONF_MIN_VALUE, default=0): cv.int_,
        cv.Optional(CONF_MAX_VALUE, default=255): cv.int_,
        cv.Optional(CONF_INVERTED, default=False): cv.boolean,
        cv.Optional(CONF_MIN_VALUE_DATAPOINT): cv.Any(cv.uint8_t,
            cv.Schema(
            {
                cv.Required(CONF_NUMBER): cv.uint8_t,
                cv.Optional(CONF_DATAPOINT_TYPE, default=MIN_VALUE_DP_TYPES["default"]): cv.one_of(
                    *MIN_VALUE_DP_TYPES["allowed"], lower=True
                )
            })
        ),
    }
)

COLOR_CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
            cv.Schema(
            {
                cv.Required(CONF_NUMBER): cv.uint8_t,
                cv.Optional(CONF_DATAPOINT_TYPE, default=COLOR_DP_TYPES["default"]): cv.one_of(
                    *COLOR_DP_TYPES["allowed"], lower=True
                )
            })
        ),
        cv.Required(CONF_TYPE): cv.enum(COLOR_TYPES, upper=True),
    }
)

WHITE_TEMPERATURE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
            cv.Schema(
            {
                cv.Required(CONF_NUMBER): cv.uint8_t,
                cv.Optional(CONF_DATAPOINT_TYPE, default=WHITE_TEMPERATURE_DP_TYPES["default"]): cv.one_of(
                    *WHITE_TEMPERATURE_DP_TYPES["allowed"], lower=True
                )
            })
        ),
        cv.Optional(CONF_INVERTED, default=False): cv.boolean,
        cv.Optional(CONF_MIN_VALUE, default=0): cv.int_,
        cv.Optional(CONF_MAX_VALUE, default=255): cv.int_,
        cv.Required(CONF_COLD_WHITE_COLOR_TEMPERATURE): cv.color_temperature,
        cv.Required(CONF_WARM_WHITE_COLOR_TEMPERATURE): cv.color_temperature,
    }
)

CONFIG_SCHEMA = cv.All(
    light.BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(UyatLight),
            cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
            cv.Required(CONF_DIMMER): DIMMER_CONFIG_SCHEMA,
            cv.Optional(CONF_SWITCH): SWITCH_CONFIG_SCHEMA,
            cv.Optional(CONF_COLOR): COLOR_CONFIG_SCHEMA,
            cv.Optional(CONF_COLOR_INTERLOCK, default=False): cv.boolean,
            cv.Optional(CONF_WHITE_TEMPERATURE): WHITE_TEMPERATURE_SCHEMA,
            # Change the default gamma_correct and default transition length settings.
            # The Uyat MCU handles transitions and gamma correction on its own.
            cv.Optional(CONF_GAMMA_CORRECT, default=1.0): cv.positive_float,
            cv.Optional(
                CONF_DEFAULT_TRANSITION_LENGTH, default="0s"
            ): cv.positive_time_period_milliseconds,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.has_at_least_one_key(
        CONF_DIMMER,
        CONF_SWITCH,
        CONF_COLOR,
    ),
)


async def to_code(config):
    if CONF_DIMMER in config:
        dimmer_config = config[CONF_DIMMER]
        if CONF_MIN_VALUE_DATAPOINT in config:
            min_value_dp = await matching_datapoint_from_config(dimmer_config[CONF_MIN_VALUE_DATAPOINT], MIN_VALUE_DP_TYPES)
        else:
            min_value_dp = cg.RawExpression("{}")

        dimmer_conf_struct = cg.StructInitializer(UyatLightConfigDimmer,
                                                  ("dimmer_dp", await matching_datapoint_from_config(dimmer_config[CONF_DATAPOINT], DIMMER_DP_TYPES)),
                                                  ("min_value", dimmer_config[CONF_MIN_VALUE]),
                                                  ("max_value", dimmer_config[CONF_MAX_VALUE]),
                                                  ("inverted", dimmer_config[CONF_INVERTED]),
                                                  ("min_value_dp", min_value_dp))
    else:
        dimmer_conf_struct = cg.RawExpression("{}")

    if CONF_SWITCH in config:
        switch_config = config[CONF_SWITCH]
        switch_conf_struct = cg.StructInitializer(UyatLightConfigSwitch,
                                                  ("switch_dp", await matching_datapoint_from_config(switch_config[CONF_DATAPOINT], SWITCH_DP_TYPES)),
                                                  ("inverted", switch_config[CONF_INVERTED]))
    else:
        switch_conf_struct = cg.RawExpression("{}")

    if CONF_COLOR in config:
        color_config = config[CONF_COLOR]
        color_conf_struct = cg.StructInitializer(UyatLightConfigColor,
                                                 ("color_dp", await matching_datapoint_from_config(color_config[CONF_DATAPOINT], COLOR_DP_TYPES)),
                                                 ("color_type", color_config[CONF_TYPE]))
    else:
        color_conf_struct = cg.RawExpression("{}")

    if CONF_WHITE_TEMPERATURE in config:
        white_temperature_config = config[CONF_WHITE_TEMPERATURE]
        white_temperature_conf_struct = cg.StructInitializer(UyatLightConfigWhiteTemperature,
                                                             ("white_temperature_dp", await matching_datapoint_from_config(white_temperature_config[CONF_DATAPOINT], WHITE_TEMPERATURE_DP_TYPES)),
                                                             ("min_value", white_temperature_config[CONF_MIN_VALUE]),
                                                             ("max_value", white_temperature_config[CONF_MAX_VALUE]),
                                                             ("inverted", white_temperature_config[CONF_INVERTED]),
                                                             ("cold_white_temperature", white_temperature_config[CONF_COLD_WHITE_COLOR_TEMPERATURE]),
                                                             ("warm_white_temperature", white_temperature_config[CONF_WARM_WHITE_COLOR_TEMPERATURE]))
    else:
        white_temperature_conf_struct = cg.RawExpression("{}")

    full_config_struct = cg.StructInitializer(UyatLightConfig,
                                              ("dimmer_config", dimmer_conf_struct),
                                              ("switch_config", switch_conf_struct),
                                              ("color_config", color_conf_struct),
                                              ("wt_config", white_temperature_conf_struct),
                                              ("color_interlock", config[CONF_COLOR_INTERLOCK]))

    var = cg.new_Pvariable(config[CONF_OUTPUT_ID], await cg.get_variable(config[CONF_UYAT_ID]), full_config_struct)
    await cg.register_component(var, config)
    await light.register_light(var, config)
