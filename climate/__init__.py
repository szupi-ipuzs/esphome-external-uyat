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
    CONF_SWITCH
)

from .. import (
   CONF_UYAT_ID,
   CONF_DATAPOINT,
   CONF_DATAPOINT_TYPE,
   Uyat,
   uyat_ns,
   UyatDatapointType,
   MatchingDatapoint,
   DPTYPE_BOOL,
   DPTYPE_UINT,
   DPTYPE_ENUM,
   DPTYPE_DETECT,
   matching_datapoint_from_config
)

DEPENDENCIES = ["uyat"]
CODEOWNERS = ["@szupi_ipuzs"]

CONF_ACTIVE_STATE = "active_state"
CONF_HEATING_VALUE = "heating_value"
CONF_COOLING_VALUE = "cooling_value"
CONF_DRYING_VALUE = "drying_value"
CONF_FANONLY_VALUE = "fanonly_value"
CONF_HEATING_STATE_PIN = "heating_state_pin"
CONF_COOLING_STATE_PIN = "cooling_state_pin"
CONF_TARGET_TEMPERATURE = "target_temperature"
CONF_CURRENT_TEMPERATURE = "current_temperature"
CONF_ECO = "eco"
CONF_SLEEP = "sleep"
CONF_SLEEP_DATAPOINT = "sleep_datapoint"
CONF_REPORTS_FAHRENHEIT = "reports_fahrenheit"
CONF_VERTICAL = "vertical"
CONF_HORIZONTAL = "horizontal"
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

TEMPERATURE_DP_TYPES = [
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

SWING_DP_TYPES = [
    DPTYPE_DETECT,
    DPTYPE_BOOL,
    DPTYPE_UINT,
    DPTYPE_ENUM,
]

FAN_SPEED_DP_TYPES = [
    DPTYPE_ENUM,
    DPTYPE_UINT,
]

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

SWITCH_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_DETECT): cv.one_of(
                        *SWITCH_DP_TYPES, lower=True
                    ),
                })
        ),
        cv.Optional(CONF_INVERTED, default=False): cv.boolean,
    },
)

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
                    cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_BOOL): cv.one_of(
                        *SLEEP_DP_TYPES, lower=True
                    ),
                }),
            ),
            cv.Optional(CONF_INVERTED, default=False): cv.boolean,
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

ANY_SWING_MODE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_DETECT): cv.one_of(
                        *SWING_DP_TYPES, lower=True
                    ),
                })
        ),
        cv.Optional(CONF_INVERTED, default=False): cv.boolean,
    },
)

SWING_MODES = cv.All(
    cv.Schema(
    {
        cv.Optional(CONF_VERTICAL): ANY_SWING_MODE_SCHEMA,
        cv.Optional(CONF_HORIZONTAL): ANY_SWING_MODE_SCHEMA
    },
    cv.has_at_least_one_key(CONF_VERTICAL, CONF_HORIZONTAL),
    )
)

TEMPERATURE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
            cv.Schema(
            {
                cv.Required(CONF_NUMBER): cv.uint8_t,
                cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_DETECT): cv.one_of(
                    *TEMPERATURE_DP_TYPES, lower=True
                ),
            })
        ),
        cv.Optional(CONF_MULTIPLIER, default=1.0): cv.positive_float,
        cv.Optional(CONF_OFFSET, default=0.0): cv.float_,
    }
)

CONFIG_SCHEMA = cv.All(
    climate.climate_schema(UyatClimate)
    .extend(
        {
            cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
            cv.Optional(CONF_SUPPORTS_HEAT, default=True): cv.boolean,
            cv.Optional(CONF_SUPPORTS_COOL, default=False): cv.boolean,
            cv.Optional(CONF_SWITCH): SWITCH_SCHEMA,
            cv.Optional(CONF_ACTIVE_STATE): ACTIVE_STATES,
            cv.Optional(CONF_HEATING_STATE_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_COOLING_STATE_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_TARGET_TEMPERATURE): TEMPERATURE_SCHEMA,
            cv.Optional(CONF_CURRENT_TEMPERATURE): TEMPERATURE_SCHEMA,
            cv.Optional(CONF_REPORTS_FAHRENHEIT, default=False): cv.boolean,
            cv.Optional(CONF_PRESET): PRESETS,
            cv.Optional(CONF_FAN_MODE): FAN_MODES,
            cv.Optional(CONF_SWING_MODE): SWING_MODES,
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
    cv.has_at_least_one_key(CONF_TARGET_TEMPERATURE, CONF_SWITCH),
    validate_cooling_values,
)

async def to_code(config):
    var = await climate.new_climate(config)
    await cg.register_component(var, config)

    paren = await cg.get_variable(config[CONF_UYAT_ID])
    cg.add(var.set_uyat_parent(paren))

    cg.add(var.set_supports_heat(config[CONF_SUPPORTS_HEAT]))
    cg.add(var.set_supports_cool(config[CONF_SUPPORTS_COOL]))
    if (switch_config := config.get(CONF_SWITCH)):
            cg.add(var.set_switch_id(await matching_datapoint_from_config(switch_config[CONF_DATAPOINT], SWITCH_DP_TYPES), switch_config[CONF_INVERTED]))

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

    if target_temperature_config := config.get(CONF_TARGET_TEMPERATURE):
        multiplier = target_temperature_config.get(CONF_MULTIPLIER, 1.0)
        offset = target_temperature_config.get(CONF_OFFSET, 0.0)
        target_temperature_datapoint = target_temperature_config.get(CONF_DATAPOINT)
        cg.add(
            var.set_target_temperature_id(await matching_datapoint_from_config(target_temperature_datapoint, TEMPERATURE_DP_TYPES),
                                          offset,
                                          multiplier)
            )
    if current_temperature_config := config.get(CONF_CURRENT_TEMPERATURE):
        multiplier = current_temperature_config.get(CONF_MULTIPLIER, 1.0)
        offset = current_temperature_config.get(CONF_OFFSET, 0.0)
        current_temperature_datapoint = current_temperature_config.get(CONF_DATAPOINT)
        cg.add(
            var.set_current_temperature_id(await matching_datapoint_from_config(current_temperature_datapoint, TEMPERATURE_DP_TYPES),
                                          offset,
                                          multiplier)
            )

    if config[CONF_REPORTS_FAHRENHEIT]:
        cg.add(var.set_reports_fahrenheit())

    if preset_config := config.get(CONF_PRESET, {}):
        if eco_config := preset_config.get(CONF_ECO, {}):
            cg.add(var.set_eco_id(await matching_datapoint_from_config(eco_config.get(CONF_DATAPOINT), ECO_DP_TYPES), eco_config[CONF_INVERTED]))
            if eco_temperature := eco_config.get(CONF_TEMPERATURE):
                cg.add(var.set_eco_temperature(eco_temperature))
        if sleep_config := preset_config.get(CONF_SLEEP, {}):
            cg.add(var.set_sleep_id(await matching_datapoint_from_config(sleep_config.get(CONF_DATAPOINT), SLEEP_DP_TYPES), eco_config[CONF_INVERTED]))

    if swing_mode_config := config.get(CONF_SWING_MODE):
        if vertical_config := swing_mode_config.get(CONF_VERTICAL):
            cg.add(var.set_swing_vertical_id(await matching_datapoint_from_config(vertical_config, SWING_DP_TYPES)))
        if horizontal_config := swing_mode_config.get(CONF_HORIZONTAL):
            cg.add(var.set_swing_horizontal_id(await matching_datapoint_from_config(horizontal_config, SWING_DP_TYPES)))

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
