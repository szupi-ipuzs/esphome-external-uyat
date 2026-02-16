import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_MAX_VALUE,
    CONF_MIN_VALUE,
    CONF_STEP,
    CONF_NUMBER,
    CONF_OFFSET
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

CONF_SCALE = "scale"
CONF_MULTIPLIER = "multiplier"

UyatNumber = uyat_ns.class_("UyatNumber", number.Number, cg.Component)
UyatNumberConfig = uyat_ns.struct("UyatNumber::Config")

NUMBER_DP_TYPES = {
    "allowed": [
        DPTYPE_DETECT,
        DPTYPE_BOOL,
        DPTYPE_UINT,
        DPTYPE_ENUM,
        ],
    "default": DPTYPE_UINT
}

def validate_min_max(config):
    max_value = config[CONF_MAX_VALUE]
    min_value = config[CONF_MIN_VALUE]
    if max_value <= min_value:
        raise cv.Invalid("max_value must be greater than min_value")
    # if (
    #     (hidden_config := config.get(CONF_DATAPOINT_HIDDEN))
    #     and (initial_value := hidden_config.get(CONF_INITIAL_VALUE, None)) is not None
    #     and ((initial_value > max_value) or (initial_value < min_value))
    # ):
    #     raise cv.Invalid(
    #         f"{CONF_INITIAL_VALUE} must be a value between {CONF_MAX_VALUE} and {CONF_MIN_VALUE}"
    #     )
    return config


CONFIG_SCHEMA = cv.All(
    number.number_schema(UyatNumber)
    .extend(
        {
            cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
            cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=NUMBER_DP_TYPES["default"]): cv.one_of(
                        *NUMBER_DP_TYPES["allowed"], lower=True
                    )
                })
            ),
            cv.Required(CONF_MAX_VALUE): cv.float_,
            cv.Required(CONF_MIN_VALUE): cv.float_,
            cv.Required(CONF_STEP): cv.positive_float,
            cv.Exclusive(CONF_SCALE, "scaling"): cv.int_range(max=7),
            cv.Exclusive(CONF_MULTIPLIER, "scaling"): cv.positive_float,
            cv.Optional(CONF_OFFSET, default=0.0): cv.float_,
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
    validate_min_max,
)


async def to_code(config):
    multiplier = 1.0
    if CONF_MULTIPLIER in config:
        multiplier = config[CONF_MULTIPLIER]
    elif CONF_SCALE in config:
        multiplier = 10 ** config[CONF_SCALE]

    var = cg.new_Pvariable(config[CONF_ID],
                           await cg.get_variable(config[CONF_UYAT_ID]),
                           cg.StructInitializer(UyatNumberConfig,
                                                ("matching_dp", await matching_datapoint_from_config(config[CONF_DATAPOINT], NUMBER_DP_TYPES)),
                                                ("offset", config[CONF_OFFSET]),
                                                ("multiplier", multiplier),
                                                )
                          )
    await cg.register_component(var, config)
    await number.register_number(
        var,
        config,
        min_value=config[CONF_MIN_VALUE],
        max_value=config[CONF_MAX_VALUE],
        step=config[CONF_STEP],
    )

