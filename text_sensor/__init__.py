import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import CONF_SENSOR_DATAPOINT, CONF_NUMBER, CONF_OPTIONS, CONF_TYPE

from .. import CONF_UYAT_ID, CONF_DATAPOINT_TYPE, Uyat, uyat_ns, DPTYPE_RAW, DPTYPE_STRING, DPTYPE_ENUM, DPTYPE_BOOL, DPTYPE_UINT

DEPENDENCIES = ["uyat"]
CODEOWNERS = ["@szupi_ipuzs"]

UyatTextSensor = uyat_ns.class_("UyatTextSensor", text_sensor.TextSensor, cg.Component)
UyatTextSensorMapped = uyat_ns.class_("UyatTextSensorMapped", text_sensor.TextSensor, cg.Component)

CONF_BASE64_ENCODED = "base64_encoded"
CONF_AS_HEX = "as_hex"

CONF_TYPE_TEXT = "text"
CONF_TYPE_MAPPED = "mapped"

TEXT_SENSOR_DP_TYPES = [
    DPTYPE_RAW,
    DPTYPE_STRING
]

MAPPED_TEXT_SENSOR_DP_TYPES = [
    DPTYPE_ENUM,
    DPTYPE_BOOL,
    DPTYPE_UINT,
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

CONFIG_SCHEMA = cv.typed_schema(
    {
        CONF_TYPE_TEXT: text_sensor.text_sensor_schema(UyatTextSensor)
        .extend(
            {
                cv.GenerateID(): cv.declare_id(UyatTextSensor),
                cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
                cv.Required(CONF_SENSOR_DATAPOINT): cv.Any(cv.uint8_t,
                    cv.Schema(
                    {
                        cv.Required(CONF_NUMBER): cv.uint8_t,
                        cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_STRING): cv.one_of(
                            *TEXT_SENSOR_DP_TYPES, lower=True
                        ),
                    })
                ),
                cv.Optional(CONF_BASE64_ENCODED, default=False): cv.boolean,
                cv.Optional(CONF_AS_HEX, default=False): cv.boolean,
            }
        )
        .extend(cv.COMPONENT_SCHEMA),

        CONF_TYPE_MAPPED: text_sensor.text_sensor_schema()
        .extend(
            {
                cv.GenerateID(): cv.declare_id(UyatTextSensorMapped),
                cv.GenerateID(CONF_UYAT_ID): cv.use_id(Uyat),
                cv.Required(CONF_SENSOR_DATAPOINT): cv.Any(cv.uint8_t,
                    cv.Schema(
                    {
                        cv.Required(CONF_NUMBER): cv.uint8_t,
                        cv.Optional(CONF_DATAPOINT_TYPE, default=DPTYPE_STRING): cv.one_of(
                            *MAPPED_TEXT_SENSOR_DP_TYPES, lower=True
                        )
                    })
                ),
                cv.Required(CONF_OPTIONS): ensure_option_map,
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
    },
    default_type=CONF_TYPE_TEXT,
    lower=True,
)

async def to_code(config):
    var = await text_sensor.new_text_sensor(config)
    await cg.register_component(var, config)

    paren = await cg.get_variable(config[CONF_UYAT_ID])
    cg.add(var.set_uyat_parent(paren))

    if config[CONF_TYPE] == CONF_TYPE_TEXT:
        dp_config = config[CONF_SENSOR_DATAPOINT]
        if not isinstance(dp_config, dict):
            cg.add(var.configure_string_dp(dp_config, config[CONF_BASE64_ENCODED], config[CONF_AS_HEX]))
        else:
            if dp_config[CONF_DATAPOINT_TYPE]==DPTYPE_RAW:
                cg.add(var.configure_raw_dp(dp_config[CONF_NUMBER], config[CONF_BASE64_ENCODED], config[CONF_AS_HEX]))
            elif dp_config[CONF_DATAPOINT_TYPE]==DPTYPE_STRING:
                paren = await cg.get_variable(config[CONF_UYAT_ID])
                cg.add(var.configure_string_dp(dp_config[CONF_NUMBER], config[CONF_BASE64_ENCODED], config[CONF_AS_HEX]))
    if config[CONF_TYPE] == CONF_TYPE_MAPPED:
        dp_config = config[CONF_SENSOR_DATAPOINT]
        if not isinstance(dp_config, dict):
            cg.add(var.configure_enum_dp(dp_config))
        else:
            if dp_config[CONF_DATAPOINT_TYPE]==DPTYPE_BOOL:
                cg.add(var.configure_bool_dp(dp_config[CONF_NUMBER]))
            elif dp_config[CONF_DATAPOINT_TYPE]==DPTYPE_ENUM:
                cg.add(var.configure_enum_dp(dp_config[CONF_NUMBER]))
            elif dp_config[CONF_DATAPOINT_TYPE]==DPTYPE_UINT:
                cg.add(var.configure_uint_dp(dp_config[CONF_NUMBER]))

        options_map = config[CONF_OPTIONS]
        for option, mapping in options_map.items():
            cg.add(var.add_mapping(option, mapping))
            # todo: how to add whole mapping in one call?
#        cg.add(var.set_select_mappings(list(options_map.keys())))