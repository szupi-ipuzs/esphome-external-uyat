from esphome import pins
import esphome.codegen as cg
from esphome.components import climate
import esphome.config_validation as cv
from esphome.const import (
    CONF_FAN_MODE,
    CONF_PRESET,
    CONF_SUPPORTS_COOL,
    CONF_SUPPORTS_HEAT,
    CONF_SWING_MODE,
    CONF_TEMPERATURE,
    CONF_NUMBER,
    CONF_INVERTED,
    CONF_OFFSET,
    CONF_SWITCH,
    CONF_HYSTERESIS
)

from .. import (
   CONF_UYAT_ID,
   CONF_DATAPOINT,
   CONF_DATAPOINT_TYPE,
   Uyat,
   uyat_ns,
   DPTYPE_BOOL,
   DPTYPE_UINT,
   DPTYPE_ENUM,
   DPTYPE_DETECT,
   matching_datapoint_from_config
)

DEPENDENCIES = ["uyat"]
CODEOWNERS = ["@szupi_ipuzs"]

UyatClimateSwitchConfig = uyat_ns.struct("UyatClimate::SwitchConfig")
UyatClimateActiveStatePinsConfig = uyat_ns.struct("UyatClimate::ActiveStatePinsConfig")
ActiveStateDpValueMapping = uyat_ns.struct("UyatClimate::ActiveStateDpValueMapping")
ActiveStateDpConfig = uyat_ns.struct("UyatClimate::ActiveStateDpConfig")
TemperatureDpConfig = uyat_ns.struct("UyatClimate::TemperatureDpConfig")
TemperatureConfig = uyat_ns.struct("UyatClimate::TemperatureConfig")
SinglePresetConfig = uyat_ns.struct("UyatClimate::SinglePresetConfig")
PresetsConfig = uyat_ns.struct("UyatClimate::PresetsConfig")
AnySwingConfig = uyat_ns.struct("UyatClimate::AnySwingConfig")
SwingsConfig = uyat_ns.struct("UyatClimate::SwingsConfig")
FanSpeedDpValueMapping = uyat_ns.struct("UyatClimate::FanSpeedDpValueMapping")
FanConfig = uyat_ns.struct("UyatClimate::FanConfig")
UyatClimateConfig = uyat_ns.struct("UyatClimate::Config")

CONF_ACTIVE_STATE_DATAPOINT = "active_state_datapoint"
CONF_HEATING_VALUE = "heating_value"
CONF_COOLING_VALUE = "cooling_value"
CONF_DRYING_VALUE = "drying_value"
CONF_FANONLY_VALUE = "fanonly_value"
CONF_HEATING_STATE_PIN = "heating"
CONF_COOLING_STATE_PIN = "cooling"
CONF_TARGET_TEMPERATURE = "target"
CONF_CURRENT_TEMPERATURE = "current"
CONF_ACTIVE_STATE_PINS = "active_state_pins"
CONF_BOOST = "boost"
CONF_ECO = "eco"
CONF_SLEEP = "sleep"
CONF_REPORTS_FAHRENHEIT = "reports_fahrenheit"
CONF_VERTICAL = "vertical"
CONF_HORIZONTAL = "horizontal"
CONF_LOW_VALUE = "low_value"
CONF_MEDIUM_VALUE = "medium_value"
CONF_MIDDLE_VALUE = "middle_value"
CONF_HIGH_VALUE = "high_value"
CONF_AUTO_VALUE = "auto_value"
CONF_MULTIPLIER = "multiplier"

SWITCH_DP_TYPES = {
   "allowed": [
        DPTYPE_DETECT,
        DPTYPE_BOOL,
        DPTYPE_UINT,
        DPTYPE_ENUM,
    ],
    "default": DPTYPE_BOOL
}

ACTIVE_STATE_DP_TYPES = {
   "allowed": [
        DPTYPE_DETECT,
        DPTYPE_UINT,
        DPTYPE_ENUM,
    ],
    "default": DPTYPE_ENUM
}

TEMPERATURE_DP_TYPES = {
   "allowed": [
        DPTYPE_DETECT,
        DPTYPE_UINT,
        DPTYPE_ENUM,
    ],
    "default": DPTYPE_UINT
}

ECO_DP_TYPES = {
   "allowed": [
        DPTYPE_DETECT,
        DPTYPE_BOOL,
        DPTYPE_UINT,
        DPTYPE_ENUM,
    ],
    "default": DPTYPE_BOOL
}

BOOST_DP_TYPES = {
   "allowed": [
        DPTYPE_DETECT,
        DPTYPE_BOOL,
        DPTYPE_UINT,
        DPTYPE_ENUM,
    ],
    "default": DPTYPE_BOOL
}

SLEEP_DP_TYPES = {
   "allowed": [
        DPTYPE_BOOL,
   ],
   "default": DPTYPE_BOOL
}

SWING_DP_TYPES = {
   "allowed": [
        DPTYPE_DETECT,
        DPTYPE_BOOL,
        DPTYPE_UINT,
        DPTYPE_ENUM,
   ],
   "default": DPTYPE_BOOL
}

FAN_SPEED_DP_TYPES = {
   "allowed": [
        DPTYPE_DETECT,
        DPTYPE_ENUM,
        DPTYPE_UINT,
   ],
   "default": DPTYPE_ENUM
}

UyatClimate = uyat_ns.class_("UyatClimate", climate.Climate, cg.Component)


def validate_cool_heat_values(value):
    has_cooling_pin_defined = CONF_ACTIVE_STATE_PINS in value and CONF_COOLING_STATE_PIN in value[CONF_ACTIVE_STATE_PINS]
    has_heating_pin_defined = CONF_ACTIVE_STATE_PINS in value and CONF_HEATING_STATE_PIN in value[CONF_ACTIVE_STATE_PINS]
    has_cooling_mode_defined = CONF_ACTIVE_STATE_DATAPOINT in value and CONF_COOLING_VALUE in value[CONF_ACTIVE_STATE_DATAPOINT]
    has_heating_mode_defined = CONF_ACTIVE_STATE_DATAPOINT in value and CONF_HEATING_VALUE in value[CONF_ACTIVE_STATE_DATAPOINT]
    if CONF_SUPPORTS_COOL in value:
        cooling_supported = value[CONF_SUPPORTS_COOL]
        if not cooling_supported:
            if has_cooling_pin_defined or has_cooling_mode_defined:
                raise cv.Invalid(
                    f"Device does not support cooling, but {CONF_COOLING_VALUE} or {CONF_COOLING_STATE_PIN} specified."
                    f" Please add '{CONF_SUPPORTS_COOL}: true' to your configuration."
                )
        else:
            if not has_cooling_pin_defined and not has_cooling_mode_defined:
                raise cv.Invalid(
                    f"Either {CONF_COOLING_VALUE} or {CONF_COOLING_STATE_PIN} is required if"
                    f" {CONF_SUPPORTS_COOL}: true' is in your configuration."
                )
    if CONF_SUPPORTS_HEAT in value:
        heating_supported = value[CONF_SUPPORTS_HEAT]
        if not heating_supported:
            if has_heating_pin_defined or has_heating_mode_defined:
                raise cv.Invalid(
                    f"Device does not support heating, but {CONF_HEATING_VALUE} or {CONF_HEATING_STATE_PIN} specified."
                    f" Please add '{CONF_SUPPORTS_HEAT}: true' to your configuration."
                )

    return value

SWITCH_CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=SWITCH_DP_TYPES["default"]): cv.one_of(
                        *SWITCH_DP_TYPES["allowed"], lower=True
                    ),
                })
        ),
        cv.Optional(CONF_INVERTED, default=False): cv.boolean,
    },
)

ACTIVE_STATE_PINS_CONFIG_SCHEMA = cv.All(
    cv.Schema(
    {
        cv.Optional(CONF_HEATING_STATE_PIN): pins.gpio_input_pin_schema,
        cv.Optional(CONF_COOLING_STATE_PIN): pins.gpio_input_pin_schema,
    }),
    cv.has_at_least_one_key(CONF_HEATING_STATE_PIN, CONF_COOLING_STATE_PIN),
)

ACTIVE_STATE_DATAPOINT_CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=ACTIVE_STATE_DP_TYPES["default"]): cv.one_of(
                        *ACTIVE_STATE_DP_TYPES["allowed"], lower=True
                    ),
                })
        ),
        cv.Optional(CONF_HEATING_VALUE): cv.uint8_t,
        cv.Optional(CONF_COOLING_VALUE): cv.uint8_t,
        cv.Optional(CONF_DRYING_VALUE): cv.uint8_t,
        cv.Optional(CONF_FANONLY_VALUE): cv.uint8_t
    }
)

PRESETS_CONFIG_SCHEMA = cv.All(cv.Schema(
    {
        cv.Optional(CONF_BOOST): {
            cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=BOOST_DP_TYPES["default"]): cv.one_of(
                        *BOOST_DP_TYPES["allowed"], lower=True
                    ),
                }),
            ),
            cv.Optional(CONF_INVERTED, default=False): cv.boolean,
            cv.Optional(CONF_TEMPERATURE): cv.temperature,
        },
        cv.Optional(CONF_ECO): {
            cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=ECO_DP_TYPES["default"]): cv.one_of(
                        *ECO_DP_TYPES["allowed"], lower=True
                    ),
                }),
            ),
            cv.Optional(CONF_INVERTED, default=False): cv.boolean,
            cv.Optional(CONF_TEMPERATURE): cv.temperature,
        },
        cv.Optional(CONF_SLEEP): cv.Schema({
            cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=SLEEP_DP_TYPES["default"]): cv.one_of(
                        *SLEEP_DP_TYPES["allowed"], lower=True
                    ),
                }),
            ),
            cv.Optional(CONF_INVERTED, default=False): cv.boolean,
        }),
    },
    cv.has_at_least_one_key(CONF_ECO, CONF_SLEEP),
    )
)

FAN_MODE_CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=FAN_SPEED_DP_TYPES["default"]): cv.one_of(
                        *FAN_SPEED_DP_TYPES["allowed"], lower=True
                    ),
                })
        ),
        cv.Optional(CONF_AUTO_VALUE): cv.uint8_t,
        cv.Optional(CONF_LOW_VALUE): cv.uint8_t,
        cv.Optional(CONF_MEDIUM_VALUE): cv.uint8_t,
        cv.Optional(CONF_MIDDLE_VALUE): cv.uint8_t,
        cv.Optional(CONF_HIGH_VALUE): cv.uint8_t,
    }
)

ANY_SWING_MODE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=SWING_DP_TYPES["default"]): cv.one_of(
                        *SWING_DP_TYPES["allowed"], lower=True
                    ),
                })
        ),
        cv.Optional(CONF_INVERTED, default=False): cv.boolean,
    },
)

SWING_MODE_CONFIG_SCHEMA = cv.All(
    cv.Schema(
    {
        cv.Optional(CONF_VERTICAL): ANY_SWING_MODE_SCHEMA,
        cv.Optional(CONF_HORIZONTAL): ANY_SWING_MODE_SCHEMA,
    },
    cv.has_at_least_one_key(CONF_VERTICAL, CONF_HORIZONTAL),
    )
)

ANY_TEMPERATURE_CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
            cv.Schema(
            {
                cv.Required(CONF_NUMBER): cv.uint8_t,
                cv.Optional(CONF_DATAPOINT_TYPE, default=TEMPERATURE_DP_TYPES["default"]): cv.one_of(
                    *TEMPERATURE_DP_TYPES["allowed"], lower=True
                ),
            })
        ),
        cv.Optional(CONF_MULTIPLIER, default=1.0): cv.positive_float,
        cv.Optional(CONF_OFFSET, default=0.0): cv.float_,
    }
)

TEMPERATURE_CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_TARGET_TEMPERATURE): ANY_TEMPERATURE_CONFIG_SCHEMA,
        cv.Optional(CONF_CURRENT_TEMPERATURE): ANY_TEMPERATURE_CONFIG_SCHEMA,
        cv.Optional(CONF_REPORTS_FAHRENHEIT, default=False): cv.boolean,
        cv.Optional(CONF_HYSTERESIS, default=1.0): cv.positive_float,
    }
)

CONFIG_SCHEMA = cv.All(
    climate.climate_schema(UyatClimate)
    .extend(
        {
            cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
            cv.Optional(CONF_SUPPORTS_HEAT, default=True): cv.boolean,
            cv.Optional(CONF_SUPPORTS_COOL, default=False): cv.boolean,
            cv.Optional(CONF_SWITCH): SWITCH_CONFIG_SCHEMA,
            cv.Optional(CONF_ACTIVE_STATE_DATAPOINT): ACTIVE_STATE_DATAPOINT_CONFIG_SCHEMA,
            cv.Optional(CONF_ACTIVE_STATE_PINS): ACTIVE_STATE_PINS_CONFIG_SCHEMA,
            cv.Optional(CONF_TEMPERATURE): TEMPERATURE_CONFIG_SCHEMA,
            cv.Optional(CONF_PRESET): PRESETS_CONFIG_SCHEMA,
            cv.Optional(CONF_FAN_MODE): FAN_MODE_CONFIG_SCHEMA,
            cv.Optional(CONF_SWING_MODE): SWING_MODE_CONFIG_SCHEMA,
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
    cv.has_at_least_one_key(CONF_TEMPERATURE, CONF_SWITCH),
    validate_cool_heat_values,
)

async def to_code(config):
    if switch_config := config.get(CONF_SWITCH):
        switch_conf_struct = cg.StructInitializer(UyatClimateSwitchConfig,
            ("matching_dp", await matching_datapoint_from_config(switch_config[CONF_DATAPOINT], SWITCH_DP_TYPES)),
            ("inverted", switch_config[CONF_INVERTED]))
    else:
        switch_conf_struct = cg.RawExpression("{}")

    if active_state_pins_config := config.get(CONF_ACTIVE_STATE_PINS):
        if heating_pin_config := active_state_pins_config.get(CONF_HEATING_STATE_PIN):
            heating_pin = await cg.gpio_pin_expression(heating_pin_config)
        else:
            heating_pin = cg.nullptr

        if cooling_pin_config := active_state_pins_config.get(CONF_COOLING_STATE_PIN):
            cooling_pin = await cg.gpio_pin_expression(cooling_pin_config)
        else:
            cooling_pin = cg.nullptr

        active_state_pins_conf_struct = cg.StructInitializer(UyatClimateActiveStatePinsConfig,
                                                                ("heating", heating_pin),
                                                                ("cooling", cooling_pin))
    else:
        active_state_pins_conf_struct = cg.RawExpression("{}")

    if active_state_config := config.get(CONF_ACTIVE_STATE_DATAPOINT):
        active_state_dp_config = active_state_config.get(CONF_DATAPOINT)
        if (heating_value_mapping := active_state_config.get(CONF_HEATING_VALUE)) is None:
            # never set to None, default is 1
            heating_value_mapping = cg.uint32(1)

        if (cooling_value_mapping := active_state_config.get(CONF_COOLING_VALUE)) is None:
            cooling_value_mapping = cg.RawExpression("{}")
        if (drying_value_mapping := active_state_config.get(CONF_DRYING_VALUE)) is None:
            drying_value_mapping = cg.RawExpression("{}")
        if (fanonly_value_mapping := active_state_config.get(CONF_FANONLY_VALUE)) is None:
            fanonly_value_mapping = cg.RawExpression("{}")

        matching_dp = await matching_datapoint_from_config(active_state_dp_config, ACTIVE_STATE_DP_TYPES);
        mapping = cg.StructInitializer(
            ActiveStateDpValueMapping,
            ("heating_value", heating_value_mapping),
            ("cooling_value", cooling_value_mapping),
            ("drying_value", drying_value_mapping),
            ("fanonly_value", fanonly_value_mapping),
        )
        active_state_dp_conf_struct = cg.StructInitializer(ActiveStateDpConfig,
                                                            ("matching_dp", matching_dp),
                                                            ("mapping", mapping))
    else:
        active_state_dp_conf_struct = cg.RawExpression("{}")

    if temperature_config := config.get(CONF_TEMPERATURE):
        target_temperature_config = temperature_config.get(CONF_TARGET_TEMPERATURE)
        tt_conf_struct = cg.StructInitializer(TemperatureDpConfig,
                                              ("matching_dp", await matching_datapoint_from_config(target_temperature_config.get(CONF_DATAPOINT), TEMPERATURE_DP_TYPES)),
                                              ("offset", target_temperature_config.get(CONF_OFFSET, 0.0)),
                                              ("multiplier", target_temperature_config.get(CONF_MULTIPLIER, 1.0))
                                             )

        if current_temperature_config := temperature_config.get(CONF_CURRENT_TEMPERATURE):
            ct_conf_struct = cg.StructInitializer(TemperatureDpConfig,
                                                  ("matching_dp", await matching_datapoint_from_config(current_temperature_config.get(CONF_DATAPOINT), TEMPERATURE_DP_TYPES)),
                                                  ("offset", current_temperature_config.get(CONF_OFFSET, 0.0)),
                                                  ("multiplier", current_temperature_config.get(CONF_MULTIPLIER, 1.0)),
                                                 )
        else:
            ct_conf_struct = cg.RawExpression("{}")

        temperature_conf_struct = cg.StructInitializer(TemperatureConfig,
                                                       ("target_temperature", tt_conf_struct),
                                                       ("current_temperature", ct_conf_struct),
                                                       ("hysteresis", temperature_config.get(CONF_HYSTERESIS)),
                                                       ("reports_fahrenheit", temperature_config.get(CONF_REPORTS_FAHRENHEIT)))

    else:
        temperature_conf_struct = cg.RawExpression("{}")

    if preset_config := config.get(CONF_PRESET, {}):
        if boost_config := preset_config.get(CONF_BOOST, {}):
            if (boost_temperature := boost_config.get(CONF_TEMPERATURE)) is None:
                boost_temperature = cg.RawExpression("{}")
            boost_conf_struct = cg.StructInitializer(SinglePresetConfig,
                                                     ("matching_dp", await matching_datapoint_from_config(boost_config.get(CONF_DATAPOINT), BOOST_DP_TYPES)),
                                                     ("inverted", boost_config[CONF_INVERTED]),
                                                     ("temperature", boost_temperature))
        else:
            boost_conf_struct = cg.RawExpression("{}")

        if eco_config := preset_config.get(CONF_ECO, {}):
            if (eco_temperature := eco_config.get(CONF_TEMPERATURE)) is None:
                eco_temperature = cg.RawExpression("{}")
            eco_conf_struct = cg.StructInitializer(SinglePresetConfig,
                                                     ("matching_dp", await matching_datapoint_from_config(eco_config.get(CONF_DATAPOINT), ECO_DP_TYPES)),
                                                     ("inverted", eco_config[CONF_INVERTED]),
                                                     ("temperature", eco_temperature))
        else:
            eco_conf_struct = cg.RawExpression("{}")

        if sleep_config := preset_config.get(CONF_SLEEP, {}):
            if (sleep_temperature := sleep_config.get(CONF_TEMPERATURE)) is None:
                sleep_temperature = cg.RawExpression("{}")
            sleep_conf_struct = cg.StructInitializer(SinglePresetConfig,
                                                     ("matching_dp", await matching_datapoint_from_config(sleep_config.get(CONF_DATAPOINT), SLEEP_DP_TYPES)),
                                                     ("inverted", sleep_config[CONF_INVERTED]),
                                                     ("temperature", sleep_temperature))
        else:
            sleep_conf_struct = cg.RawExpression("{}")

        presets_conf_struct = cg.StructInitializer(PresetsConfig,
                                                   ("boost_config", boost_conf_struct),
                                                   ("eco_config", eco_conf_struct),
                                                   ("sleep_config", sleep_conf_struct))
    else:
        presets_conf_struct = cg.RawExpression("{}")

    if swing_mode_config := config.get(CONF_SWING_MODE):
        if vertical_config := swing_mode_config.get(CONF_VERTICAL):
            vertical_swing_conf_struct = cg.StructInitializer(AnySwingConfig,
                                                              ("matching_dp", await matching_datapoint_from_config(vertical_config.get(CONF_DATAPOINT), SWING_DP_TYPES)),
                                                              ("inverted", vertical_config.get(CONF_INVERTED)))
        else:
            vertical_swing_conf_struct = cg.RawExpression("{}")

        if horizontal_config := swing_mode_config.get(CONF_HORIZONTAL):
            horizontal_swing_conf_struct = cg.StructInitializer(AnySwingConfig,
                                                              ("matching_dp", await matching_datapoint_from_config(horizontal_config.get(CONF_DATAPOINT), SWING_DP_TYPES)),
                                                              ("inverted", horizontal_config.get(CONF_INVERTED)))
        else:
            horizontal_swing_conf_struct = cg.RawExpression("{}")

        swings_conf_struct = cg.StructInitializer(SwingsConfig,
                                                  ("vertical", vertical_swing_conf_struct),
                                                  ("horizontal", horizontal_swing_conf_struct))
    else:
        swings_conf_struct = cg.RawExpression("{}")

    if fan_mode_config := config.get(CONF_FAN_MODE):
        if (auto_value_mapping := fan_mode_config.get(CONF_AUTO_VALUE)) is None:
            auto_value_mapping = cg.RawExpression("{}")
        if (low_value_mapping := fan_mode_config.get(CONF_LOW_VALUE)) is None:
            low_value_mapping = cg.RawExpression("{}")
        if (medium_value_mapping := fan_mode_config.get(CONF_MEDIUM_VALUE)) is None:
            medium_value_mapping = cg.RawExpression("{}")
        if (middle_value_mapping := fan_mode_config.get(CONF_MIDDLE_VALUE)) is None:
            middle_value_mapping = cg.RawExpression("{}")
        if (high_value_mapping := fan_mode_config.get(CONF_HIGH_VALUE)) is None:
            high_value_mapping = cg.RawExpression("{}")

        mapping = cg.StructInitializer(FanSpeedDpValueMapping,
                                        ("auto_value", auto_value_mapping),
                                        ("low_value", low_value_mapping),
                                        ("medium_value", medium_value_mapping),
                                        ("middle_value", middle_value_mapping),
                                        ("high_value", high_value_mapping))

        fan_config_struct = cg.StructInitializer(FanConfig,
                                                 ("matching_dp", await matching_datapoint_from_config(fan_mode_config.get(CONF_DATAPOINT), FAN_SPEED_DP_TYPES)),
                                                 ("mapping", mapping))
    else:
        fan_config_struct = cg.RawExpression("{}")

    final_config = cg.StructInitializer(UyatClimateConfig,
                                        ("supports_heat", config[CONF_SUPPORTS_HEAT]),
                                        ("supports_cool", config[CONF_SUPPORTS_COOL]),
                                        ("switch_config", switch_conf_struct),
                                        ("active_state_pins_config", active_state_pins_conf_struct),
                                        ("active_state_dp_config", active_state_dp_conf_struct),
                                        ("temperature_config", temperature_conf_struct),
                                        ("presets_config", presets_conf_struct),
                                        ("swings_config", swings_conf_struct),
                                        ("fan_config", fan_config_struct))

    var = await climate.new_climate(config, await cg.get_variable(config[CONF_UYAT_ID]), final_config)
    await cg.register_component(var, config)