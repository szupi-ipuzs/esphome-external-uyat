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
CONF_COLOR_TEMPERATURE_DATAPOINT = "color_temperature_datapoint"
CONF_COLOR_TEMPERATURE_INVERT = "color_temperature_invert"
CONF_COLOR_TEMPERATURE_MAX_VALUE = "color_temperature_max_value"
CONF_SWITCH = "switch"
CONF_DIMMER = "dimmer"
CONF_COLOR = "color"

UyatColorType = uyat_ns.enum("UyatColorType", is_class=True)

COLOR_TYPES = {
    "RGB": UyatColorType.RGB,
    "HSV": UyatColorType.HSV,
    "RGBHSV": UyatColorType.RGBHSV,
}

DIMMER_DP_TYPES = {
    DPTYPE_DETECT,
    DPTYPE_UINT,
    DPTYPE_ENUM
}

MIN_VALUE_DP_TYPES = {
    DPTYPE_DETECT,
    DPTYPE_UINT,
    DPTYPE_ENUM
}

SWITCH_DP_TYPES = {
    DPTYPE_DETECT,
    DPTYPE_BOOL,
    DPTYPE_UINT,
    DPTYPE_ENUM
}

COLOR_DP_TYPES = {
    DPTYPE_STRING
}


UyatLight = uyat_ns.class_("UyatLight", light.LightOutput, cg.Component)

SWITCH_CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
            cv.Schema(
            {
                cv.Required(CONF_NUMBER): cv.uint8_t,
                cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_DETECT): cv.one_of(
                    *SWITCH_DP_TYPES, lower=True
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
                cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_DETECT): cv.one_of(
                    *DIMMER_DP_TYPES, lower=True
                )
            })
        ),
        cv.Optional(CONF_MIN_VALUE, default=0): cv.int_,
        cv.Optional(CONF_MAX_VALUE, default=255): cv.int_,
        cv.Optional(CONF_MIN_VALUE_DATAPOINT): cv.Any(cv.uint8_t,
            cv.Schema(
            {
                cv.Required(CONF_NUMBER): cv.uint8_t,
                cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_DETECT): cv.one_of(
                    *MIN_VALUE_DP_TYPES, lower=True
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
                cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_STRING): cv.one_of(
                    *COLOR_DP_TYPES, lower=True
                )
            })
        ),
        cv.Required(CONF_TYPE): cv.enum(COLOR_TYPES, upper=True),
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
            cv.Inclusive(
                CONF_COLOR_TEMPERATURE_DATAPOINT, "color_temperature"
            ): cv.uint8_t,
            cv.Optional(CONF_COLOR_TEMPERATURE_INVERT, default=False): cv.boolean,
            cv.Optional(CONF_COLOR_TEMPERATURE_MAX_VALUE): cv.int_,
            cv.Inclusive(
                CONF_COLD_WHITE_COLOR_TEMPERATURE, "color_temperature"
            ): cv.color_temperature,
            cv.Inclusive(
                CONF_WARM_WHITE_COLOR_TEMPERATURE, "color_temperature"
            ): cv.color_temperature,
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
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)

    if CONF_DIMMER in config:
        dimmer_config = config[CONF_DIMMER]
        if CONF_MIN_VALUE_DATAPOINT in config:
            cg.add(var.configure_dimmer(
                                        await matching_datapoint_from_config(dimmer_config[CONF_DATAPOINT], DIMMER_DP_TYPES),
                                        dimmer_config[CONF_MIN_VALUE],
                                        dimmer_config[CONF_MAX_VALUE]),
                                        await matching_datapoint_from_config(dimmer_config[CONF_MIN_VALUE_DATAPOINT], MIN_VALUE_DP_TYPES))
        else:
            cg.add(var.configure_dimmer(
                                        await matching_datapoint_from_config(dimmer_config[CONF_DATAPOINT], DIMMER_DP_TYPES),
                                        dimmer_config[CONF_MIN_VALUE],
                                        dimmer_config[CONF_MAX_VALUE]))
    if CONF_SWITCH in config:
        switch_config = config[CONF_SWITCH]
        cg.add(var.configure_switch(await matching_datapoint_from_config(switch_config[CONF_DATAPOINT], SWITCH_DP_TYPES), switch_config[CONF_INVERTED]))
    if CONF_COLOR in config:
        color_config = config[CONF_COLOR]
        cg.add(var.configure_color(await matching_datapoint_from_config(color_config[CONF_DATAPOINT], COLOR_DP_TYPES), color_config[CONF_TYPE]))
    if CONF_COLOR_TEMPERATURE_DATAPOINT in config:
        cg.add(var.set_color_temperature_id(config[CONF_COLOR_TEMPERATURE_DATAPOINT]))
        cg.add(var.set_color_temperature_invert(config[CONF_COLOR_TEMPERATURE_INVERT]))

        cg.add(
            var.set_cold_white_temperature(config[CONF_COLD_WHITE_COLOR_TEMPERATURE])
        )
        cg.add(
            var.set_warm_white_temperature(config[CONF_WARM_WHITE_COLOR_TEMPERATURE])
        )
    if CONF_COLOR_TEMPERATURE_MAX_VALUE in config:
        cg.add(
            var.set_color_temperature_max_value(
                config[CONF_COLOR_TEMPERATURE_MAX_VALUE]
            )
        )

    cg.add(var.set_color_interlock(config[CONF_COLOR_INTERLOCK]))
    paren = await cg.get_variable(config[CONF_UYAT_ID])
    cg.add(var.set_uyat_parent(paren))
