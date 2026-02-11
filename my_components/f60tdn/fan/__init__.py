import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan, remote_base
from esphome.const import CONF_OUTPUT_ID, CONF_SPEED_COUNT, CONF_OSCILLATING
from .. import f60tdn_ns

F60tdnFan = f60tdn_ns.class_("F60tdnFan", cg.Component, fan.Fan, remote_base.RemoteReceiverListener,
                             remote_base.RemoteTransmittable,)
CONF_HAS_OSCILLATING = "has_oscillating"
CONFIG_SCHEMA = (fan.FAN_SCHEMA.extend(
    {
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(F60tdnFan),
        cv.Optional(CONF_SPEED_COUNT, default=9): cv.int_range(min=1, max=256),
        cv.Optional(CONF_HAS_OSCILLATING, default=True): cv.boolean,
        cv.Optional(remote_base.CONF_RECEIVER_ID): cv.use_id(
            remote_base.RemoteReceiverBase),           
    }
    
).extend(cv.COMPONENT_SCHEMA)
 .extend(remote_base.REMOTE_TRANSMITTABLE_SCHEMA))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await fan.register_fan(var, config)
    await remote_base.register_transmittable(var, config)
    cg.add(var.set_has_oscillating(config[CONF_HAS_OSCILLATING]))
    if CONF_SPEED_COUNT in config:
        cg.add(var.set_speed_count(config[CONF_SPEED_COUNT]))
    if remote_base.CONF_RECEIVER_ID in config:
        await remote_base.register_listener(var, config)
        

 


   