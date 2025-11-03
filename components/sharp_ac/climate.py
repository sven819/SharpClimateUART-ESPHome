import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, uart, select, switch, text_sensor, button
from esphome.const import CONF_ID

AUTO_LOAD = ["climate", "uart", "select", "switch", "text_sensor", "button"]
CODEOWNERS = ["@sven819"]

CONF_SHARP_ID = "sharp_id"

sharp_ac_ns = cg.esphome_ns.namespace("sharp_ac")
SharpAc = sharp_ac_ns.class_("SharpAc", climate.Climate,uart.UARTDevice, cg.Component)

VaneSelectVertical = sharp_ac_ns.class_("VaneSelectVertical", select.Select, cg.Component)
VaneSelectHorizontal = sharp_ac_ns.class_("VaneSelectHorizontal", select.Select, cg.Component)

IonSwitch = sharp_ac_ns.class_("IonSwitch", switch.Switch)
ConnectionStatusSensor = sharp_ac_ns.class_("ConnectionStatusSensor", text_sensor.TextSensor, cg.Component)
ReconnectButton = sharp_ac_ns.class_("ReconnectButton", button.Button, cg.Component)

CONF_HORIZONTAL_SWING_SELECT = "horizontal_vane_select"
CONF_VERTICAL_SWING_SELECT = "vertical_vane_select"
CONF_ION_SWITCH = "ion_switch"
CONF_CONNECTION_STATUS = "connection_status"
CONF_RECONNECT_BUTTON = "reconnect_button"

HORIZONTAL_SWING_OPTIONS = ["swing","left","center","right"]
VERTICAL_SWING_OPTIONS = ["auto", "swing" , "up" , "up_center", "center", "down_center", "down"]

SELECT_SCHEMA_VERTICAL = select.select_schema(VaneSelectVertical).extend(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(VaneSelectVertical),
    }
)

SELECT_SCHEMA_HORIZONTAL = select.select_schema(VaneSelectHorizontal).extend(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(VaneSelectHorizontal),
    }
)

ION_SCHEMA = switch.switch_schema(IonSwitch).extend(
    {cv.GenerateID(CONF_ID): cv.declare_id(IonSwitch)}
)

CONNECTION_STATUS_SCHEMA = text_sensor.text_sensor_schema(ConnectionStatusSensor).extend(
    {cv.GenerateID(CONF_ID): cv.declare_id(ConnectionStatusSensor)}
)

RECONNECT_BUTTON_SCHEMA = button.button_schema(ReconnectButton).extend(
    {cv.GenerateID(CONF_ID): cv.declare_id(ReconnectButton)}
)

CONFIG_SCHEMA = climate.climate_schema(SharpAc).extend(
    {
        cv.GenerateID(): cv.declare_id(SharpAc),
        cv.Optional(CONF_HORIZONTAL_SWING_SELECT): SELECT_SCHEMA_HORIZONTAL,
        cv.Optional(CONF_VERTICAL_SWING_SELECT): SELECT_SCHEMA_VERTICAL,
        cv.Optional(CONF_ION_SWITCH): ION_SCHEMA,
        cv.Optional(CONF_CONNECTION_STATUS): CONNECTION_STATUS_SCHEMA,
        cv.Optional(CONF_RECONNECT_BUTTON): RECONNECT_BUTTON_SCHEMA
    }
).extend(uart.UART_DEVICE_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    if CONF_HORIZONTAL_SWING_SELECT in config:
        conf = config[CONF_HORIZONTAL_SWING_SELECT]
        swing_select = await select.new_select(conf, options=HORIZONTAL_SWING_OPTIONS)
        await cg.register_parented(swing_select, var)
        cg.add(var.setVaneHorizontalSelect(swing_select))
    
    if CONF_VERTICAL_SWING_SELECT in config:
        conf = config[CONF_VERTICAL_SWING_SELECT]
        swing_select = await select.new_select(conf, options=VERTICAL_SWING_OPTIONS)
        await cg.register_parented(swing_select, var)
        cg.add(var.setVaneVerticalSelect(swing_select))

    if CONF_ION_SWITCH in config: 
        conf = config[CONF_ION_SWITCH]
        swt = await switch.new_switch(conf)
        cg.add(var.setIonSwitch(swt))
        await cg.register_parented(swt, var)

    if CONF_CONNECTION_STATUS in config:
        conf = config[CONF_CONNECTION_STATUS]
        sens = await text_sensor.new_text_sensor(conf)
        cg.add(var.setConnectionStatusSensor(sens))
        await cg.register_parented(sens, var)

    if CONF_RECONNECT_BUTTON in config:
        conf = config[CONF_RECONNECT_BUTTON]
        btn = await button.new_button(conf)
        cg.add(btn.set_parent(var))
        cg.add(var.setReconnectButton(btn))
        await cg.register_parented(btn, var)

    await uart.register_uart_device(var, config)
    await climate.register_climate(var, config)
    await cg.register_component(var, config)
