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
    CONF_SWITCH_DATAPOINT,
    CONF_TEMPERATURE,
    CONF_NUMBER,
    CONF_INVERTED,
    CONF_OFFSET,
)

from .. import CONF_UYAT_ID, CONF_DATAPOINT_TYPE, Uyat, uyat_ns, UyatDatapointType, MatchingDatapoint, DPTYPE_BOOL, DPTYPE_UINT, DPTYPE_ENUM, DPTYPE_DETECT

DEPENDENCIES = ["uyat"]
CODEOWNERS = ["@szupi_ipuzs"]

CONF_ACTIVE_STATE = "active_state"
CONF_DATAPOINT = "datapoint"
CONF_HEATING_VALUE = "heating_value"
CONF_COOLING_VALUE = "cooling_value"
CONF_DRYING_VALUE = "drying_value"
CONF_FANONLY_VALUE = "fanonly_value"
CONF_HEATING_STATE_PIN = "heating_state_pin"
CONF_COOLING_STATE_PIN = "cooling_state_pin"
CONF_TARGET_TEMPERATURE_DATAPOINT = "target_temperature_datapoint"
CONF_CURRENT_TEMPERATURE_DATAPOINT = "current_temperature_datapoint"
CONF_ECO = "eco"
CONF_SLEEP = "sleep"
CONF_SLEEP_DATAPOINT = "sleep_datapoint"
CONF_REPORTS_FAHRENHEIT = "reports_fahrenheit"
CONF_VERTICAL_DATAPOINT = "vertical_datapoint"
CONF_HORIZONTAL_DATAPOINT = "horizontal_datapoint"
CONF_LOW_VALUE = "low_value"
CONF_MEDIUM_VALUE = "medium_value"
CONF_MIDDLE_VALUE = "middle_value"
CONF_HIGH_VALUE = "high_value"
CONF_AUTO_VALUE = "auto_value"
CONF_MULTIPLIER = "multiplier"

SWITCH_DP_TYPES = [
    DPTYPE_DETECT,
    DPTYPE_BOOL,
    DPTYPE_UINT,
    DPTYPE_ENUM,
]

ACTIVE_STATE_DP_TYPES = [
    DPTYPE_DETECT,
    DPTYPE_UINT,
    DPTYPE_ENUM,
]

TARGET_TEMPERATURE_DP_TYPES = [
    DPTYPE_DETECT,
    DPTYPE_UINT,
    DPTYPE_ENUM,
]

CURRENT_TEMPERATURE_DP_TYPES = [
    DPTYPE_DETECT,
    DPTYPE_UINT,
    DPTYPE_ENUM,
]

ECO_DP_TYPES = [
    DPTYPE_DETECT,
    DPTYPE_BOOL,
    DPTYPE_UINT,
    DPTYPE_ENUM,
]

SLEEP_DP_TYPES = [
    DPTYPE_BOOL,
]

VSWING_DP_TYPES = [
    DPTYPE_BOOL,
]

HSWING_DP_TYPES = [
    DPTYPE_BOOL,
]

FAN_SPEED_DP_TYPES = [
    DPTYPE_ENUM,
    DPTYPE_UINT,
]

DP_TYPES_TRANSLATED = {
    DPTYPE_BOOL: UyatDatapointType.BOOLEAN,
    DPTYPE_UINT: UyatDatapointType.INTEGER,
    DPTYPE_ENUM: UyatDatapointType.ENUM
}

UyatClimate = uyat_ns.class_("UyatClimate", climate.Climate, cg.Component)


def validate_cooling_values(value):
    if CONF_SUPPORTS_COOL in value:
        cooling_supported = value[CONF_SUPPORTS_COOL]
        if not cooling_supported and CONF_ACTIVE_STATE in value:
            active_state_config = value[CONF_ACTIVE_STATE]
            if (
                CONF_COOLING_VALUE in active_state_config
                or CONF_COOLING_STATE_PIN in value
            ):
                raise cv.Invalid(
                    f"Device does not support cooling, but {CONF_COOLING_VALUE} or {CONF_COOLING_STATE_PIN} specified."
                    f" Please add '{CONF_SUPPORTS_COOL}: true' to your configuration."
                )
        elif cooling_supported and CONF_ACTIVE_STATE in value:
            active_state_config = value[CONF_ACTIVE_STATE]
            if (
                CONF_COOLING_VALUE not in active_state_config
                and CONF_COOLING_STATE_PIN not in value
            ):
                raise cv.Invalid(
                    f"Either {CONF_ACTIVE_STATE} {CONF_COOLING_VALUE} or {CONF_COOLING_STATE_PIN} is required if"
                    f" {CONF_SUPPORTS_COOL}: true' is in your configuration."
                )
    return value


ACTIVE_STATES = cv.Schema(
    {
        cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_DETECT): cv.one_of(
                        *ACTIVE_STATE_DP_TYPES, lower=True
                    ),
                })
        ),
        cv.Optional(CONF_HEATING_VALUE, default=1): cv.uint8_t,
        cv.Optional(CONF_COOLING_VALUE): cv.uint8_t,
        cv.Optional(CONF_DRYING_VALUE): cv.uint8_t,
        cv.Optional(CONF_FANONLY_VALUE): cv.uint8_t,
    },
)

PRESETS = cv.Schema(
    {
        cv.Optional(CONF_ECO): {
            cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_DETECT): cv.one_of(
                        *ECO_DP_TYPES, lower=True
                    ),
                    cv.Optional(CONF_INVERTED, default=False): cv.boolean,
                }),
            ),
            cv.Optional(CONF_TEMPERATURE): cv.temperature,
        },
        cv.Optional(CONF_SLEEP): cv.Schema({
            cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_BOOL): cv.one_of(
                        *SLEEP_DP_TYPES, lower=True
                    ),
                    cv.Optional(CONF_INVERTED, default=False): cv.boolean,
                }),
            ),
        }),
    },
)

FAN_MODES = cv.Schema(
    {
        cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_DETECT): cv.one_of(
                        *FAN_SPEED_DP_TYPES, lower=True
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

SWING_MODES = cv.Schema(
    {
        cv.Optional(CONF_VERTICAL_DATAPOINT): cv.uint8_t,
        cv.Optional(CONF_HORIZONTAL_DATAPOINT): cv.uint8_t,
    },
)

CONFIG_SCHEMA = cv.All(
    climate.climate_schema(UyatClimate)
    .extend(
        {
            cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
            cv.Optional(CONF_SUPPORTS_HEAT, default=True): cv.boolean,
            cv.Optional(CONF_SUPPORTS_COOL, default=False): cv.boolean,
            cv.Optional(CONF_SWITCH_DATAPOINT): cv.Any(cv.uint8_t,
                    cv.Schema(
                    {
                        cv.Required(CONF_NUMBER): cv.uint8_t,
                        cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_DETECT): cv.one_of(
                            *SWITCH_DP_TYPES, lower=True
                        ),
                        cv.Optional(CONF_INVERTED, default=False): cv.boolean,
                    })
                ),
            cv.Optional(CONF_ACTIVE_STATE): ACTIVE_STATES,
            cv.Optional(CONF_HEATING_STATE_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_COOLING_STATE_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_TARGET_TEMPERATURE_DATAPOINT): cv.Any(cv.uint8_t,
                    cv.Schema(
                    {
                        cv.Required(CONF_NUMBER): cv.uint8_t,
                        cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_DETECT): cv.one_of(
                            *TARGET_TEMPERATURE_DP_TYPES, lower=True
                        ),
                        cv.Optional(CONF_MULTIPLIER, default=1.0): cv.positive_float,
                        cv.Optional(CONF_OFFSET, default=0.0): cv.float_,
                    })
                ),
            cv.Optional(CONF_CURRENT_TEMPERATURE_DATAPOINT): cv.Any(cv.uint8_t,
                    cv.Schema(
                    {
                        cv.Required(CONF_NUMBER): cv.uint8_t,
                        cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_DETECT): cv.one_of(
                            *CURRENT_TEMPERATURE_DP_TYPES, lower=True
                        ),
                        cv.Optional(CONF_MULTIPLIER, default=1.0): cv.positive_float,
                        cv.Optional(CONF_OFFSET, default=0.0): cv.float_,
                    })
                ),
            cv.Optional(CONF_REPORTS_FAHRENHEIT, default=False): cv.boolean,
            cv.Optional(CONF_PRESET): PRESETS,
            cv.Optional(CONF_FAN_MODE): FAN_MODES,
            cv.Optional(CONF_SWING_MODE): SWING_MODES,
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
    cv.has_at_least_one_key(CONF_TARGET_TEMPERATURE_DATAPOINT, CONF_SWITCH_DATAPOINT),
    validate_cooling_values,
)

async def translate_dp_types(dp_types: list):
    cpp_types_enum = []
    for dp_type in dp_types:
        if not dp_type == DPTYPE_DETECT:
            cpp_types_enum.append(DP_TYPES_TRANSLATED[dp_type])

    return cg.ArrayInitializer(*cpp_types_enum)

async def matching_datapoint_from_config(dp_config, allowed_types: list):
    allow_autodetect = DPTYPE_DETECT in allowed_types
    if isinstance(dp_config, dict):
        dp_type = dp_config.get(CONF_DATAPOINT_TYPE)
        if dp_type == DPTYPE_DETECT and allow_autodetect:
            return cg.StructInitializer(
                MatchingDatapoint,
                ("number", dp_config[CONF_NUMBER]),
                ("types", await translate_dp_types(allowed_types))
            )
        else:
            return cg.StructInitializer(
                MatchingDatapoint,
                ("number", dp_config[CONF_NUMBER]),
                ("types", await translate_dp_types([dp_type]))
            )
    else:
        if allow_autodetect:
            return cg.StructInitializer(
                MatchingDatapoint,
                ("number", dp_config),
                ("types", await translate_dp_types(allowed_types))
            )

async def to_code(config):
    var = await climate.new_climate(config)
    await cg.register_component(var, config)

    paren = await cg.get_variable(config[CONF_UYAT_ID])
    cg.add(var.set_uyat_parent(paren))

    cg.add(var.set_supports_heat(config[CONF_SUPPORTS_HEAT]))
    cg.add(var.set_supports_cool(config[CONF_SUPPORTS_COOL]))
    if switch_datapoint := config.get(CONF_SWITCH_DATAPOINT):
        if isinstance(switch_datapoint, dict):
            is_inverted = switch_datapoint[CONF_INVERTED]
        else:
            is_inverted = False

        cg.add(var.set_switch_id(await matching_datapoint_from_config(switch_datapoint, SWITCH_DP_TYPES), is_inverted))

    if heating_state_pin_config := config.get(CONF_HEATING_STATE_PIN):
        heating_state_pin = await cg.gpio_pin_expression(heating_state_pin_config)
        cg.add(var.set_heating_state_pin(heating_state_pin))
    if cooling_state_pin_config := config.get(CONF_COOLING_STATE_PIN):
        cooling_state_pin = await cg.gpio_pin_expression(cooling_state_pin_config)
        cg.add(var.set_cooling_state_pin(cooling_state_pin))
    if active_state_config := config.get(CONF_ACTIVE_STATE):
        cg.add(var.set_active_state_id(await matching_datapoint_from_config(active_state_config, ACTIVE_STATE_DP_TYPES)))
        if (heating_value := active_state_config.get(CONF_HEATING_VALUE)) is not None:
            cg.add(var.set_active_state_heating_value(heating_value))
        if (cooling_value := active_state_config.get(CONF_COOLING_VALUE)) is not None:
            cg.add(var.set_active_state_cooling_value(cooling_value))
        if (drying_value := active_state_config.get(CONF_DRYING_VALUE)) is not None:
            cg.add(var.set_active_state_drying_value(drying_value))
        if (fanonly_value := active_state_config.get(CONF_FANONLY_VALUE)) is not None:
            cg.add(var.set_active_state_fanonly_value(fanonly_value))

    if target_temperature_datapoint := config.get(CONF_TARGET_TEMPERATURE_DATAPOINT):
        multiplier = 1.0
        offset = 0.0
        if isinstance(target_temperature_datapoint, dict):
            multiplier = target_temperature_datapoint.get(CONF_MULTIPLIER, 1.0)
            offset = target_temperature_datapoint.get(CONF_OFFSET, 0.0)
        cg.add(
            var.set_target_temperature_id(await matching_datapoint_from_config(target_temperature_datapoint, TARGET_TEMPERATURE_DP_TYPES),
                                          offset,
                                          multiplier)
            )
    if current_temperature_datapoint := config.get(CONF_CURRENT_TEMPERATURE_DATAPOINT):
        multiplier = 1.0
        offset = 0.0
        if isinstance(current_temperature_datapoint, dict):
            multiplier = current_temperature_datapoint.get(CONF_MULTIPLIER, 1.0)
            offset = current_temperature_datapoint.get(CONF_OFFSET, 0.0)
        cg.add(
            var.set_current_temperature_id(await matching_datapoint_from_config(current_temperature_datapoint, CURRENT_TEMPERATURE_DP_TYPES),
                                          offset,
                                          multiplier)
            )

    if config[CONF_REPORTS_FAHRENHEIT]:
        cg.add(var.set_reports_fahrenheit())

    if preset_config := config.get(CONF_PRESET, {}):
        if eco_config := preset_config.get(CONF_ECO, {}):
            eco_dp_config = eco_config.get(CONF_DATAPOINT)
            if isinstance(eco_dp_config, dict):
                is_inverted = eco_dp_config[CONF_INVERTED]
            else:
                is_inverted = False
            cg.add(var.set_eco_id(await matching_datapoint_from_config(eco_dp_config, ECO_DP_TYPES), is_inverted))
            if eco_temperature := eco_config.get(CONF_TEMPERATURE):
                cg.add(var.set_eco_temperature(eco_temperature))
        if sleep_config := preset_config.get(CONF_SLEEP, {}):
            sleep_dp_config = sleep_config.get(CONF_DATAPOINT)
            if isinstance(eco_dp_config, dict):
                is_inverted = eco_dp_config[CONF_INVERTED]
            else:
                is_inverted = False
            cg.add(var.set_sleep_id(await matching_datapoint_from_config(sleep_dp_config, SLEEP_DP_TYPES), is_inverted))

    if swing_mode_config := config.get(CONF_SWING_MODE):
        if swing_vertical_datapoint := swing_mode_config.get(CONF_VERTICAL_DATAPOINT):
            cg.add(var.set_swing_vertical_id(await matching_datapoint_from_config(swing_vertical_datapoint, VSWING_DP_TYPES)))
        if swing_horizontal_datapoint := swing_mode_config.get(CONF_HORIZONTAL_DATAPOINT):
            cg.add(var.set_swing_vertical_id(await matching_datapoint_from_config(swing_horizontal_datapoint, HSWING_DP_TYPES)))
    if fan_mode_config := config.get(CONF_FAN_MODE):
        fan_modes_dp_config = fan_mode_config.get(CONF_DATAPOINT)
        cg.add(var.set_fan_speed_id(await matching_datapoint_from_config(fan_modes_dp_config, FAN_SPEED_DP_TYPES)))
        if (fan_auto_value := fan_mode_config.get(CONF_AUTO_VALUE)) is not None:
            cg.add(var.set_fan_speed_auto_value(fan_auto_value))
        if (fan_low_value := fan_mode_config.get(CONF_LOW_VALUE)) is not None:
            cg.add(var.set_fan_speed_low_value(fan_low_value))
        if (fan_medium_value := fan_mode_config.get(CONF_MEDIUM_VALUE)) is not None:
            cg.add(var.set_fan_speed_medium_value(fan_medium_value))
        if (fan_middle_value := fan_mode_config.get(CONF_MIDDLE_VALUE)) is not None:
            cg.add(var.set_fan_speed_middle_value(fan_middle_value))
        if (fan_high_value := fan_mode_config.get(CONF_HIGH_VALUE)) is not None:
            cg.add(var.set_fan_speed_high_value(fan_high_value))
