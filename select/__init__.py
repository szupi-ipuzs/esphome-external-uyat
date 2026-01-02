import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import (
    CONF_OPTIMISTIC,
    CONF_OPTIONS,
    CONF_NUMBER
)

from .. import CONF_UYAT_ID, CONF_DATAPOINT_TYPE, Uyat, uyat_ns, DPTYPE_BOOL, DPTYPE_UINT, DPTYPE_ENUM

CONF_SELECT_DATAPOINT = "select_datapoint"

DEPENDENCIES = ["uyat"]
CODEOWNERS = ["@bearpawmaxim"]

UyatSelect = uyat_ns.class_("UyatSelect", select.Select, cg.Component)

SELECT_DP_TYPES = [
    DPTYPE_BOOL,
    DPTYPE_UINT,
    DPTYPE_ENUM
]

def ensure_option_map(value):
    cv.check_not_templatable(value)
    option = cv.All(cv.int_range(0, 2**8 - 1))
    mapping = cv.All(cv.string_strict)
    options_map_schema = cv.Schema({option: mapping})
    value = options_map_schema(value)

    all_values = list(value.keys())
    unique_values = set(value.keys())
    if len(all_values) != len(unique_values):
        raise cv.Invalid("Mapping values must be unique.")

    return value


CONFIG_SCHEMA = cv.All(
    select.select_schema(UyatSelect)
    .extend(
        {
            cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
            cv.Required(CONF_SELECT_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_ENUM): cv.one_of(
                        *SELECT_DP_TYPES, lower=True
                    )
                })
            ),
            cv.Required(CONF_OPTIONS): ensure_option_map,
            cv.Optional(CONF_OPTIMISTIC, default=False): cv.boolean,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    options_map = config[CONF_OPTIONS]
    var = await select.new_select(config, options=list(options_map.values()))
    await cg.register_component(var, config)
    cg.add(var.set_select_mappings(list(options_map.keys())))
    parent = await cg.get_variable(config[CONF_UYAT_ID])
    cg.add(var.set_uyat_parent(parent))
    cg.add(var.set_optimistic(config[CONF_OPTIMISTIC]))

    dp_config = config[CONF_SELECT_DATAPOINT]
    if not isinstance(dp_config, dict):
        cg.add(var.configure_enum_dp(dp_config))
    else:
        if dp_config[CONF_DATAPOINT_TYPE]==DPTYPE_BOOL:
            cg.add(var.configure_bool_dp(dp_config[CONF_NUMBER]))
        elif dp_config[CONF_DATAPOINT_TYPE]==DPTYPE_UINT:
            cg.add(var.configure_uint_dp(dp_config[CONF_NUMBER]))
        elif dp_config[CONF_DATAPOINT_TYPE]==DPTYPE_ENUM:
            cg.add(var.configure_enum_dp(dp_config[CONF_NUMBER]))
