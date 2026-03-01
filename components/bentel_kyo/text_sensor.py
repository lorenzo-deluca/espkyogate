"""Bentel KYO text_sensor platform."""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import (
    CONF_ID,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import bentel_kyo_ns, BentelKyo, CONF_BENTEL_KYO_ID

DEPENDENCIES = ["bentel_kyo"]

CONF_FIRMWARE_VERSION = "firmware_version"
CONF_ALARM_MODEL = "alarm_model"
CONF_KEYFOBS = "keyfobs"
CONF_SLOT = "slot"
CONF_PANEL_NAME = "panel_name"
CONF_PARTITIONS = "partitions"
CONF_PARTITION = "partition"
CONF_CODES = "codes"
CONF_CODE = "code"

TextSensorType = bentel_kyo_ns.enum("TextSensorType")

PARTITION_NAME_SCHEMA = text_sensor.text_sensor_schema(
    icon="mdi:form-textbox",
    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
).extend(
    {
        cv.Required(CONF_PARTITION): cv.int_range(min=1, max=8),
    }
)

CODE_NAME_SCHEMA = text_sensor.text_sensor_schema(
    icon="mdi:form-textbox",
    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
).extend(
    {
        cv.Required(CONF_CODE): cv.int_range(min=1, max=24),
    }
)

KEYFOB_SCHEMA = text_sensor.text_sensor_schema(
    icon="mdi:key-wireless",
    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
).extend(
    {
        cv.Required(CONF_SLOT): cv.int_range(min=1, max=16),
        cv.Optional(CONF_PANEL_NAME): text_sensor.text_sensor_schema(
            icon="mdi:form-textbox",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_BENTEL_KYO_ID): cv.use_id(BentelKyo),
        cv.Optional(CONF_FIRMWARE_VERSION): text_sensor.text_sensor_schema(
            icon="mdi:tag",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_ALARM_MODEL): text_sensor.text_sensor_schema(
            icon="mdi:shield-check",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_KEYFOBS): cv.ensure_list(KEYFOB_SCHEMA),
        cv.Optional(CONF_PARTITIONS): cv.ensure_list(PARTITION_NAME_SCHEMA),
        cv.Optional(CONF_CODES): cv.ensure_list(CODE_NAME_SCHEMA),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_BENTEL_KYO_ID])

    if CONF_FIRMWARE_VERSION in config:
        var = await text_sensor.new_text_sensor(config[CONF_FIRMWARE_VERSION])
        cg.add(var.set_disabled_by_default(True))
        cg.add(hub.set_firmware_version_text_sensor(var))

    if CONF_ALARM_MODEL in config:
        var = await text_sensor.new_text_sensor(config[CONF_ALARM_MODEL])
        cg.add(var.set_disabled_by_default(True))
        cg.add(hub.set_alarm_model_text_sensor(var))

    if CONF_KEYFOBS in config:
        for keyfob_conf in config[CONF_KEYFOBS]:
            slot_index = keyfob_conf[CONF_SLOT] - 1  # 0-based
            var = await text_sensor.new_text_sensor(keyfob_conf)
            cg.add(var.set_disabled_by_default(True))
            cg.add(hub.register_text_sensor(var, TextSensorType.TEXT_KEYFOB_ESN, slot_index))
            if CONF_PANEL_NAME in keyfob_conf:
                name_var = await text_sensor.new_text_sensor(keyfob_conf[CONF_PANEL_NAME])
                cg.add(name_var.set_disabled_by_default(True))
                cg.add(hub.register_text_sensor(name_var, TextSensorType.TEXT_KEYFOB_NAME, slot_index))

    if CONF_PARTITIONS in config:
        for part_conf in config[CONF_PARTITIONS]:
            part_index = part_conf[CONF_PARTITION] - 1  # 0-based
            var = await text_sensor.new_text_sensor(part_conf)
            cg.add(var.set_disabled_by_default(True))
            cg.add(hub.register_text_sensor(var, TextSensorType.TEXT_PARTITION_NAME, part_index))

    if CONF_CODES in config:
        for code_conf in config[CONF_CODES]:
            code_index = code_conf[CONF_CODE] - 1  # 0-based
            var = await text_sensor.new_text_sensor(code_conf)
            cg.add(var.set_disabled_by_default(True))
            cg.add(hub.register_text_sensor(var, TextSensorType.TEXT_CODE_NAME, code_index))
