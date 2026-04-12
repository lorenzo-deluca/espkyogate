"""Bentel KYO alarm_control_panel platform."""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import alarm_control_panel, text_sensor
from esphome.const import CONF_ID, ENTITY_CATEGORY_DIAGNOSTIC

from . import bentel_kyo_ns, BentelKyo, CONF_BENTEL_KYO_ID

DEPENDENCIES = ["bentel_kyo"]

CONF_PARTITION = "partition"
CONF_ENTRY_DELAY = "entry_delay"
CONF_EXIT_DELAY = "exit_delay"
CONF_SIREN_TIMER = "siren_timer"
CONF_CODES = "codes"
CONF_REQUIRES_CODE_TO_ARM = "requires_code_to_arm"
CONF_REQUIRES_CODE_TO_DISARM = "requires_code_to_disarm"

BentelKyoAlarmPanel = bentel_kyo_ns.class_(
    "BentelKyoAlarmPanel",
    alarm_control_panel.AlarmControlPanel,
    cg.Component,
)

TextSensorType = bentel_kyo_ns.enum("TextSensorType")


def _validate_code_config(config):
    if config.get(CONF_REQUIRES_CODE_TO_ARM, False) and not config.get(CONF_CODES, []):
        raise cv.Invalid(
            f"{CONF_REQUIRES_CODE_TO_ARM} cannot be set without codes."
        )
    return config


CONFIG_SCHEMA = cv.All(
    alarm_control_panel.alarm_control_panel_schema(BentelKyoAlarmPanel)
    .extend(
        {
            cv.GenerateID(CONF_BENTEL_KYO_ID): cv.use_id(BentelKyo),
            cv.Required(CONF_PARTITION): cv.int_range(min=1, max=8),
            cv.Optional(CONF_CODES): cv.ensure_list(cv.string_strict),
            cv.Optional(CONF_REQUIRES_CODE_TO_ARM): cv.boolean,
            cv.Optional(CONF_REQUIRES_CODE_TO_DISARM): cv.boolean,
            cv.Optional(CONF_ENTRY_DELAY): text_sensor.text_sensor_schema(
                icon="mdi:timer-sand",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_EXIT_DELAY): text_sensor.text_sensor_schema(
                icon="mdi:timer-sand",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_SIREN_TIMER): text_sensor.text_sensor_schema(
                icon="mdi:timer-alert",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
    _validate_code_config,
)


async def to_code(config):
    var = await alarm_control_panel.new_alarm_control_panel(config)
    await cg.register_component(var, config)

    hub = await cg.get_variable(config[CONF_BENTEL_KYO_ID])
    cg.add(var.set_parent(hub))
    cg.add(var.set_partition(config[CONF_PARTITION]))
    cg.add(hub.register_alarm_panel(var))

    if CONF_CODES in config:
        for code in config[CONF_CODES]:
            cg.add(var.add_code(code))
        if CONF_REQUIRES_CODE_TO_ARM in config:
            cg.add(var.set_requires_code_to_arm(config[CONF_REQUIRES_CODE_TO_ARM]))

    partition_index = config[CONF_PARTITION] - 1  # 0-based

    if CONF_ENTRY_DELAY in config:
        ts = await text_sensor.new_text_sensor(config[CONF_ENTRY_DELAY])
        cg.add(ts.set_disabled_by_default(True))
        cg.add(hub.register_text_sensor(ts, TextSensorType.TEXT_PARTITION_ENTRY_DELAY, partition_index))

    if CONF_EXIT_DELAY in config:
        ts = await text_sensor.new_text_sensor(config[CONF_EXIT_DELAY])
        cg.add(ts.set_disabled_by_default(True))
        cg.add(hub.register_text_sensor(ts, TextSensorType.TEXT_PARTITION_EXIT_DELAY, partition_index))

    if CONF_SIREN_TIMER in config:
        ts = await text_sensor.new_text_sensor(config[CONF_SIREN_TIMER])
        cg.add(ts.set_disabled_by_default(True))
        cg.add(hub.register_text_sensor(ts, TextSensorType.TEXT_PARTITION_SIREN_TIMER, partition_index))
