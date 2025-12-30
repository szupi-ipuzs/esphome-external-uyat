import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import CONF_SWITCH_DATAPOINT, CONF_NUMBER

from .. import CONF_UYAT_ID, CONF_DATAPOINT_TYPE, uyat_ns, Uyat, DPTYPE_BOOL, DPTYPE_UINT, DPTYPE_ENUM

DEPENDENCIES = ["uyat"]
CODEOWNERS = ["@szupi_ipuzs"]

UyatSwitch = uyat_ns.class_("UyatSwitch", switch.Switch, cg.Component)

SWITCH_DP_TYPES = [
    DPTYPE_BOOL,
    DPTYPE_UINT,
    DPTYPE_ENUM
]

CONFIG_SCHEMA = (
    switch.switch_schema(UyatSwitch)
    .extend(
        {
            cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
            cv.Required(CONF_SWITCH_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_BOOL): cv.one_of(
                        *SWITCH_DP_TYPES, lower=True
                    )
                })
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await switch.new_switch(config)
    await cg.register_component(var, config)

    paren = await cg.get_variable(config[CONF_UYAT_ID])
    cg.add(var.set_uyat_parent(paren))

    dp_config = config[CONF_SWITCH_DATAPOINT]
    if not isinstance(dp_config, dict):
        cg.add(var.configure_bool_dp(dp_config))
    else:
        if dp_config[CONF_DATAPOINT_TYPE]==DPTYPE_BOOL:
            cg.add(var.configure_bool_dp(dp_config[CONF_NUMBER]))
        elif dp_config[CONF_DATAPOINT_TYPE]==DPTYPE_UINT:
            cg.add(var.configure_uint_dp(dp_config[CONF_NUMBER]))
        elif dp_config[CONF_DATAPOINT_TYPE]==DPTYPE_ENUM:
            cg.add(var.configure_enum_dp(dp_config[CONF_NUMBER]))
