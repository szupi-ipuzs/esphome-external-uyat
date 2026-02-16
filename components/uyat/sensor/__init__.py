import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_NUMBER, CONF_TYPE

from .. import (
   CONF_UYAT_ID,
   CONF_DATAPOINT,
   CONF_DATAPOINT_TYPE,
   Uyat,
   uyat_ns,
   DPTYPE_RAW,
   DPTYPE_BOOL,
   DPTYPE_UINT,
   DPTYPE_ENUM,
   DPTYPE_BITMAP,
   DPTYPE_DETECT,
   matching_datapoint_from_config
)

DEPENDENCIES = ["uyat"]
CODEOWNERS = ["@szupi_ipuzs"]

UyatSensor = uyat_ns.class_("UyatSensor", sensor.Sensor, cg.Component)
UyatSensorConfig = uyat_ns.struct("UyatSensor::Config")
UyatSensorVAP = uyat_ns.class_("UyatSensorVAP", sensor.Sensor, cg.Component)
UyatSensorVAPConfig = uyat_ns.struct("UyatSensorVAP::Config")
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
CONF_VAP_VALUE_TYPE = "vap_value_type"

SENSOR_DP_TYPES = {
   "allowed": [
        DPTYPE_DETECT,
        DPTYPE_BOOL,
        DPTYPE_UINT,
        DPTYPE_ENUM,
        DPTYPE_BITMAP,
    ],
    "default": DPTYPE_DETECT,
}

VAP_DP_TYPES = {
   "allowed": [
        DPTYPE_RAW
    ],
    "default": DPTYPE_RAW,
}

CONFIG_SCHEMA = cv.typed_schema(
    {
       CONF_TYPE_NUMBER: sensor.sensor_schema(UyatSensor)
        .extend(
            {
                cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
                cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
                    cv.Schema(
                    {
                        cv.Required(CONF_NUMBER): cv.uint8_t,
                        cv.Optional(CONF_DATAPOINT_TYPE, default=SENSOR_DP_TYPES["default"]): cv.one_of(
                            *SENSOR_DP_TYPES["allowed"], lower=True
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
                cv.Required(CONF_DATAPOINT): cv.Any(cv.uint8_t,
                    cv.Schema(
                    {
                        cv.Required(CONF_NUMBER): cv.uint8_t,
                        cv.Optional(CONF_DATAPOINT_TYPE, default=VAP_DP_TYPES["default"]): cv.one_of(
                            *VAP_DP_TYPES["allowed"], lower=True
                        )
                    })
                ),
                cv.Required(CONF_VAP_VALUE_TYPE): cv.one_of(
                    *VAP_VALUE_TYPES.keys(), lower=True,
                ),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
    },
    default_type=CONF_TYPE_NUMBER,
    lower=True,
)

async def to_code(config):
    if config[CONF_TYPE] == CONF_TYPE_NUMBER:
        config_struct = cg.StructInitializer(UyatSensorConfig,
                                            ("matching_dp", await matching_datapoint_from_config(config[CONF_DATAPOINT], SENSOR_DP_TYPES)))
        var = cg.new_Pvariable(config[CONF_ID], await cg.get_variable(config[CONF_UYAT_ID]), config_struct)
    if config[CONF_TYPE] == CONF_TYPE_VAP:
        config_struct = cg.StructInitializer(UyatSensorVAPConfig,
                                            ("matching_dp", await matching_datapoint_from_config(config[CONF_DATAPOINT], SENSOR_DP_TYPES)),
                                            ("value_type", VAP_VALUE_TYPES[config[CONF_VAP_VALUE_TYPE]]))
        var = cg.new_Pvariable(config[CONF_ID], await cg.get_variable(config[CONF_UYAT_ID]), config_struct)

    await cg.register_component(var, config)
    await sensor.register_sensor(var, config)
