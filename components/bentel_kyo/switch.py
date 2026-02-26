"""Bentel KYO switch platform — polling control."""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID, ENTITY_CATEGORY_CONFIG

from . import bentel_kyo_ns, BentelKyo, CONF_BENTEL_KYO_ID

BentelKyoPollingSwitch = bentel_kyo_ns.class_(
    "BentelKyoPollingSwitch", switch.Switch, cg.Component
)

CONFIG_SCHEMA = (
    switch.switch_schema(
        BentelKyoPollingSwitch,
        icon="mdi:connection",
        entity_category=ENTITY_CATEGORY_CONFIG,
    )
    .extend(
        {
            cv.GenerateID(CONF_BENTEL_KYO_ID): cv.use_id(BentelKyo),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await switch.new_switch(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_BENTEL_KYO_ID])
    cg.add(var.set_parent(parent))
