import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv

from .. import CONF_UYAT_ID, Uyat, uyat_ns

DEPENDENCIES = ["uyat"]
CODEOWNERS = ["@szupi_ipuzs"]

CONF_TRIGGER_PAYLOAD = "trigger_payload"
CONF_BUTTON_DATAPOINT = "button_datapoint"

UyatButton = uyat_ns.class_("UyatButton", button.Button, cg.Component)

CONFIG_SCHEMA = (
    button.button_schema(UyatButton)
    .extend(
        {
            cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
            cv.Required(CONF_BUTTON_DATAPOINT): cv.uint8_t,
            cv.Optional(CONF_TRIGGER_PAYLOAD, default=True): cv.boolean,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    var = await button.new_button(config)
    await cg.register_component(var, config)

    paren = await cg.get_variable(config[CONF_UYAT_ID])
    cg.add(var.set_uyat_parent(paren))

    cg.add(var.set_datapoint_id(config[CONF_BUTTON_DATAPOINT]))
    cg.add(var.set_trigger_payload(config[CONF_TRIGGER_PAYLOAD]))
