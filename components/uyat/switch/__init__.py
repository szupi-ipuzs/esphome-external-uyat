import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import CONF_NUMBER

from .. import (
   CONF_UYAT_ID,
   CONF_DATAPOINT,
   CONF_DATAPOINT_TYPE,
   uyat_ns,
   Uyat,
   DPTYPE_BOOL,
   DPTYPE_UINT,
   DPTYPE_ENUM,
   DPTYPE_DETECT,
   matching_datapoint_from_config,
)

DEPENDENCIES = ["uyat"]
CODEOWNERS = ["@szupi_ipuzs"]

UyatSwitch = uyat_ns.class_("UyatSwitch", switch.Switch, cg.Component)
UyatSwitchConfig = uyat_ns.struct("UyatSwitch::Config")

SWITCH_DP_TYPES = {
   "allowed": [
        DPTYPE_DETECT,
        DPTYPE_BOOL,
        DPTYPE_UINT,
        DPTYPE_ENUM
    ],
    "default": DPTYPE_BOOL,
}

CONFIG_SCHEMA = (
    switch.switch_schema(UyatSwitch)
    .extend(
        {
            cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
            cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
                cv.Schema(
                {
                    cv.Required(CONF_NUMBER): cv.uint8_t,
                    cv.Optional(CONF_DATAPOINT_TYPE, default=SWITCH_DP_TYPES["default"]): cv.one_of(
                        *SWITCH_DP_TYPES["allowed"], lower=True
                    )
                })
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    config_struct = cg.StructInitializer(UyatSwitchConfig,
                                         ("matching_dp", await matching_datapoint_from_config(config[CONF_DATAPOINT], SWITCH_DP_TYPES)))
    var = await switch.new_switch(config, await cg.get_variable(config[CONF_UYAT_ID]), config_struct)
    await cg.register_component(var, config)
