import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_SENSOR_DATAPOINT, CONF_NUMBER, CONF_TYPE

from .. import CONF_UYAT_ID, CONF_DATAPOINT_TYPE, Uyat, uyat_ns, DPTYPE_ANY, DPTYPE_BOOL, DPTYPE_UINT, DPTYPE_ENUM, DPTYPE_BITMAP, DPTYPE_DETECT

DEPENDENCIES = ["uyat"]
CODEOWNERS = ["@szupi_ipuzs"]

UyatSensor = uyat_ns.class_("UyatSensor", sensor.Sensor, cg.Component)
UyatSensorVAP = uyat_ns.class_("UyatSensorVAP", sensor.Sensor, cg.Component)
UyatVAPValueType = uyat_ns.enum("UyatVAPValueType", is_class=True)

VAP_VALUE_TYPE_VOLTAGE = "voltage"
VAP_VALUE_TYPE_AMPERAGE = "amperage"
VAP_VALUE_TYPE_POWER = "power"

VAP_VALUE_TYPES = {
    VAP_VALUE_TYPE_VOLTAGE: UyatVAPValueType.VOLTAGE,
    VAP_VALUE_TYPE_AMPERAGE: UyatVAPValueType.AMPERAGE,
    VAP_VALUE_TYPE_POWER: UyatVAPValueType.POWER,
}

CONF_TYPE_NUMBER = "number"
CONF_TYPE_VAP = "vap"

SENSOR_DP_TYPES = [
    DPTYPE_DETECT,
    DPTYPE_BOOL,
    DPTYPE_UINT,
    DPTYPE_ENUM,
    DPTYPE_BITMAP,
]

CONFIG_SCHEMA = cv.typed_schema(
    {
       CONF_TYPE_NUMBER: sensor.sensor_schema(UyatSensor)
        .extend(
            {
                cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
                cv.Required(CONF_SENSOR_DATAPOINT): cv.Any(cv.uint8_t,
                    cv.Schema(
                    {
                        cv.Required(CONF_NUMBER): cv.uint8_t,
                        cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_DETECT): cv.one_of(
                            *SENSOR_DP_TYPES, lower=True
                        )
                    })
                ),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),

       CONF_TYPE_VAP: sensor.sensor_schema(UyatSensorVAP)
        .extend(
            {
                cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
                cv.Required(CONF_SENSOR_DATAPOINT): cv.uint8_t,
                cv.Required("vap_value_type"): cv.one_of(
                    *VAP_VALUE_TYPES, lower=True,
                ),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
    },
    default_type=CONF_TYPE_NUMBER,
    lower=True,
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await sensor.register_sensor(var, config)

    paren = await cg.get_variable(config[CONF_UYAT_ID])
    cg.add(var.set_uyat_parent(paren))

    if config[CONF_TYPE] == CONF_TYPE_NUMBER:
        dp_config = config[CONF_SENSOR_DATAPOINT]
        if not isinstance(dp_config, dict):
            cg.add(var.configure_any_dp(dp_config))
        else:
            if dp_config[CONF_DATAPOINT_TYPE]==DPTYPE_DETECT:
                cg.add(var.configure_any_dp(dp_config[CONF_NUMBER]))
            elif dp_config[CONF_DATAPOINT_TYPE]==DPTYPE_BITMAP:
                cg.add(var.configure_bitmap_dp(dp_config[CONF_NUMBER]))
            elif dp_config[CONF_DATAPOINT_TYPE]==DPTYPE_BOOL:
                cg.add(var.configure_bool_dp(dp_config[CONF_NUMBER]))
            elif dp_config[CONF_DATAPOINT_TYPE]==DPTYPE_UINT:
                cg.add(var.configure_uint_dp(dp_config[CONF_NUMBER]))
            elif dp_config[CONF_DATAPOINT_TYPE]==DPTYPE_ENUM:
                cg.add(var.configure_enum_dp(dp_config[CONF_NUMBER]))
    if config[CONF_TYPE] == CONF_TYPE_VAP:
        dp_id = config[CONF_SENSOR_DATAPOINT]
        value_type = VAP_VALUE_TYPES[config["vap_value_type"]]
        cg.add(var.configure_raw_dp(dp_id, value_type))
