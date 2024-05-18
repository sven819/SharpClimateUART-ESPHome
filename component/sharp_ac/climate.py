import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, uart
from esphome.const import CONF_ID

AUTO_LOAD = ["climate", "uart"]
CODEOWNERS = ["@sven819"]

sharp_ac_ns = cg.esphome_ns.namespace("sharp_ac")
SharpAc = sharp_ac_ns.class_("SharpAc", climate.Climate,uart.UARTDevice, cg.Component)

CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(SharpAc),
    }
).extend(uart.UART_DEVICE_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await uart.register_uart_device(var, config)
    await climate.register_climate(var, config)
    await cg.register_component(var, config)
