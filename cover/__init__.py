import esphome.codegen as cg
from esphome.components import cover
import esphome.config_validation as cv
from esphome.const import CONF_MAX_VALUE, CONF_MIN_VALUE, CONF_RESTORE_MODE, CONF_INVERTED, CONF_NUMBER

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

CONF_CONTROL = "control"
CONF_OPEN_VALUE = "open_value"
CONF_CLOSE_VALUE = "close_value"
CONF_STOP_VALUE = "stop_value"

CONF_DIRECTION = "direction"

CONF_POSITION = "position"

CONF_POSITION_DATAPOINT = "position_datapoint"
CONF_POSITION_REPORT_DATAPOINT = "position_report_datapoint"

CONF_UNCALIBRATED_VALUE = "uncalibrated_value"

UyatCover = uyat_ns.class_("UyatCover", cover.Cover, cg.Component)
ControlDpValueMapping = uyat_ns.struct("ControlDpValueMapping")

UyatCoverRestoreMode = uyat_ns.enum("UyatCoverRestoreMode")
RESTORE_MODES = {
    "NO_RESTORE": UyatCoverRestoreMode.COVER_NO_RESTORE,
    "RESTORE": UyatCoverRestoreMode.COVER_RESTORE,
    "RESTORE_AND_CALL": UyatCoverRestoreMode.COVER_RESTORE_AND_CALL,
}


def validate_range(config):
    if config[CONF_MIN_VALUE] > config[CONF_MAX_VALUE]:
        raise cv.Invalid(
            f"min_value ({config[CONF_MIN_VALUE]}) cannot be greater than max_value ({config[CONF_MAX_VALUE]})"
        )
    return config

CONTROL_DP_TYPES = {
   "allowed": [
        DPTYPE_DETECT,
        DPTYPE_UINT,
        DPTYPE_ENUM,
    ],
    "default": DPTYPE_ENUM
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

POSITION_DP_TYPES = {
   "allowed": [
        DPTYPE_DETECT,
        DPTYPE_UINT,
        DPTYPE_ENUM,
    ],
    "default": DPTYPE_UINT
}

POSITION_REPORT_DP_TYPES = {
   "allowed": [
        DPTYPE_DETECT,
        DPTYPE_UINT,
        DPTYPE_ENUM,
    ],
    "default": DPTYPE_UINT
}

CONTROL_CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=CONTROL_DP_TYPES["default"]): cv.one_of(
                        *CONTROL_DP_TYPES["allowed"], lower=True
                    ),
                })
        ),
        cv.Optional(CONF_OPEN_VALUE): cv.uint32_t,
        cv.Optional(CONF_CLOSE_VALUE): cv.uint32_t,
        cv.Optional(CONF_STOP_VALUE): cv.uint32_t
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
                    ),
                })
        ),
        cv.Optional(CONF_INVERTED, default=False): cv.boolean,
    }
)

POSITION_CONFIG_SCHEMA = cv.All(
    cv.Schema(
    {
        cv.Optional(CONF_POSITION_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=POSITION_DP_TYPES["default"]): cv.one_of(
                        *POSITION_DP_TYPES["allowed"], lower=True
                    ),
                })
        ),
        cv.Optional(CONF_POSITION_REPORT_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=POSITION_REPORT_DP_TYPES["default"]): cv.one_of(
                        *POSITION_REPORT_DP_TYPES["allowed"], lower=True
                    ),
                })
        ),
        cv.Optional(CONF_INVERTED, default=False): cv.boolean,
        cv.Optional(CONF_MIN_VALUE, default=0): cv.uint32_t,
        cv.Optional(CONF_MAX_VALUE, default=100): cv.uint32_t,
        cv.Optional(CONF_UNCALIBRATED_VALUE): cv.uint32_t,
    }),
    validate_range,
    cv.has_at_least_one_key(CONF_POSITION_DATAPOINT, CONF_POSITION_REPORT_DATAPOINT),
)

CONFIG_SCHEMA = cv.All(
    cover.cover_schema(UyatCover)
    .extend(
        {
            cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
            cv.Optional(CONF_CONTROL): CONTROL_CONFIG_SCHEMA,
            cv.Optional(CONF_DIRECTION): DIRECTION_CONFIG_SCHEMA,
            cv.Required(CONF_POSITION): POSITION_CONFIG_SCHEMA,
            cv.Optional(CONF_RESTORE_MODE, default="RESTORE"): cv.enum(
                RESTORE_MODES, upper=True
            ),
        },
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await cover.new_cover(config)
    await cg.register_component(var, config)

    if control_config := config.get(CONF_CONTROL):
        matching_dp = await matching_datapoint_from_config(control_config[CONF_DATAPOINT], CONTROL_DP_TYPES)
        mapping = cg.StructInitializer(ControlDpValueMapping,
                                       ("open_value", control_config.get(CONF_OPEN_VALUE)),
                                       ("close_value", control_config.get(CONF_CLOSE_VALUE)),
                                       ("stop_value", control_config.get(CONF_STOP_VALUE)),
                                       )
        cg.add(var.configure_control(matching_dp, mapping))
    if direction_config := config.get(CONF_DIRECTION):
        cg.add(var.configure_direction(await matching_datapoint_from_config(direction_config[CONF_DATAPOINT], DIRECTION_DP_TYPES),
                                       direction_config[CONF_INVERTED]))

    position_config = config.get(CONF_POSITION)
    if CONF_POSITION_DATAPOINT in position_config:
        position_dp = await matching_datapoint_from_config(position_config[CONF_POSITION_DATAPOINT], POSITION_DP_TYPES)
    else:
        position_dp = cg.RawExpression("{}")
    if CONF_POSITION_REPORT_DATAPOINT in config:
        position_report_dp = await matching_datapoint_from_config(position_config[CONF_POSITION_REPORT_DATAPOINT], POSITION_REPORT_DP_TYPES)
    else:
        position_report_dp = cg.RawExpression("{}")
    if CONF_UNCALIBRATED_VALUE in position_config:
        uncalibrated_value = config[CONF_UNCALIBRATED_VALUE]
    else:
        uncalibrated_value = cg.RawExpression("{}")

    cg.add(var.configure_position(position_dp, position_report_dp,
                                position_config[CONF_INVERTED],
                                position_config[CONF_MIN_VALUE], position_config[CONF_MAX_VALUE],
                                uncalibrated_value))

    cg.add(var.set_restore_mode(config[CONF_RESTORE_MODE]))
    paren = await cg.get_variable(config[CONF_UYAT_ID])
    cg.add(var.set_uyat_parent(paren))
