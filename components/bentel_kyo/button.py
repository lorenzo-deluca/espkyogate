"""Bentel KYO button platform — reread panel config."""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import CONF_ID, ENTITY_CATEGORY_CONFIG

from . import bentel_kyo_ns, BentelKyo, CONF_BENTEL_KYO_ID

BentelKyoRereadConfigButton = bentel_kyo_ns.class_(
    "BentelKyoRereadConfigButton", button.Button, cg.Component
)

CONFIG_SCHEMA = (
    button.button_schema(
        BentelKyoRereadConfigButton,
        icon="mdi:refresh",
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
    var = await button.new_button(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_BENTEL_KYO_ID])
    cg.add(var.set_parent(parent))
