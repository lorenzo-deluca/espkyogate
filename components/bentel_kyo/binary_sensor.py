"""Bentel KYO binary_sensor platform."""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, text_sensor
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    CONF_DEVICE_CLASS,
    DEVICE_CLASS_CONNECTIVITY,
    DEVICE_CLASS_PROBLEM,
    DEVICE_CLASS_SAFETY,
    DEVICE_CLASS_SOUND,
    DEVICE_CLASS_TAMPER,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_POWER,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import bentel_kyo_ns, BentelKyo, CONF_BENTEL_KYO_ID

DEPENDENCIES = ["bentel_kyo"]

# Config keys
CONF_ZONES = "zones"
CONF_ZONE = "zone"
CONF_ZONE_TAMPER = "zone_tamper"
CONF_ZONE_BYPASS = "zone_bypass"
CONF_ZONE_ALARM_MEMORY = "zone_alarm_memory"
CONF_ZONE_TAMPER_MEMORY = "zone_tamper_memory"
CONF_WARNINGS = "warnings"
CONF_COMMUNICATION = "communication"
CONF_SIREN = "siren"
CONF_TAMPER = "tamper"
CONF_PARTITION_ALARM = "partition_alarm"
CONF_PARTITION = "partition"
CONF_OUTPUT_STATE = "output_state"
CONF_OUTPUT_NUMBER = "output_number"

# Zone diagnostic text sensor keys (nested inside zone entries)
CONF_ZONE_TYPE = "zone_type"
CONF_PANEL_NAME = "panel_name"
CONF_ZONE_PARTITION = "zone_partition"
CONF_ZONE_ESN = "esn"

# Output diagnostic text sensor key (nested inside output_state entries)
CONF_OUTPUT_PANEL_NAME = "panel_name"

# Warning keys
CONF_MAINS_FAILURE = "mains_failure"
CONF_BPI_MISSING = "bpi_missing"
CONF_FUSE_FAULT = "fuse_fault"
CONF_LOW_BATTERY = "low_battery"
CONF_PHONE_LINE_FAULT = "phone_line_fault"
CONF_DEFAULT_CODES = "default_codes"
CONF_WIRELESS_FAULT = "wireless_fault"

# Tamper keys
CONF_TAMPER_ZONE = "tamper_zone"
CONF_TAMPER_FALSE_KEY = "tamper_false_key"
CONF_TAMPER_BPI = "tamper_bpi"
CONF_TAMPER_SYSTEM = "tamper_system"
CONF_TAMPER_RF_JAM = "tamper_rf_jam"
CONF_TAMPER_WIRELESS = "tamper_wireless"

# C++ enum references
BinarySensorType = bentel_kyo_ns.enum("BinarySensorType")
SENSOR_TYPES = {
    "ZONE": BinarySensorType.ZONE,
    "ZONE_TAMPER": BinarySensorType.ZONE_TAMPER,
    "ZONE_BYPASS": BinarySensorType.ZONE_BYPASS,
    "ZONE_ALARM_MEMORY": BinarySensorType.ZONE_ALARM_MEMORY,
    "ZONE_TAMPER_MEMORY": BinarySensorType.ZONE_TAMPER_MEMORY,
    "PARTITION_ALARM": BinarySensorType.PARTITION_ALARM,
    "WARNING_MAINS_FAILURE": BinarySensorType.WARNING_MAINS_FAILURE,
    "WARNING_BPI_MISSING": BinarySensorType.WARNING_BPI_MISSING,
    "WARNING_FUSE_FAULT": BinarySensorType.WARNING_FUSE_FAULT,
    "WARNING_LOW_BATTERY": BinarySensorType.WARNING_LOW_BATTERY,
    "WARNING_PHONE_LINE_FAULT": BinarySensorType.WARNING_PHONE_LINE_FAULT,
    "WARNING_DEFAULT_CODES": BinarySensorType.WARNING_DEFAULT_CODES,
    "WARNING_WIRELESS_FAULT": BinarySensorType.WARNING_WIRELESS_FAULT,
    "TAMPER_ZONE": BinarySensorType.TAMPER_ZONE,
    "TAMPER_FALSE_KEY": BinarySensorType.TAMPER_FALSE_KEY,
    "TAMPER_BPI": BinarySensorType.TAMPER_BPI,
    "TAMPER_SYSTEM": BinarySensorType.TAMPER_SYSTEM,
    "TAMPER_RF_JAM": BinarySensorType.TAMPER_RF_JAM,
    "TAMPER_WIRELESS": BinarySensorType.TAMPER_WIRELESS,
    "SIREN": BinarySensorType.SIREN,
    "COMMUNICATION": BinarySensorType.COMMUNICATION,
    "OUTPUT_STATE": BinarySensorType.OUTPUT_STATE,
}

TextSensorType = bentel_kyo_ns.enum("TextSensorType")
TEXT_SENSOR_TYPES = {
    "TEXT_ZONE_TYPE": TextSensorType.TEXT_ZONE_TYPE,
    "TEXT_ZONE_NAME": TextSensorType.TEXT_ZONE_NAME,
    "TEXT_ZONE_AREA": TextSensorType.TEXT_ZONE_AREA,
    "TEXT_ZONE_ESN": TextSensorType.TEXT_ZONE_ESN,
    "TEXT_OUTPUT_NAME": TextSensorType.TEXT_OUTPUT_NAME,
}

# ========================================
# Schemas with default device_class & icon
# ========================================

# Zone sensors: no default device_class (user picks motion/door/window/etc)
# Includes optional nested text_sensor diagnostics for zone_type, panel_name, zone_area
ZONE_SENSOR_SCHEMA = binary_sensor.binary_sensor_schema(
    icon="mdi:shield-home",
).extend(
    {
        cv.Required(CONF_ZONE): cv.int_range(min=1, max=32),
        cv.Optional(CONF_ZONE_TYPE): text_sensor.text_sensor_schema(
            icon="mdi:shield-alert-outline",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_PANEL_NAME): text_sensor.text_sensor_schema(
            icon="mdi:form-textbox",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_ZONE_PARTITION): text_sensor.text_sensor_schema(
            icon="mdi:shield-home-outline",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_ZONE_ESN): text_sensor.text_sensor_schema(
            icon="mdi:identifier",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)

# Zone tamper: device_class=tamper
ZONE_TAMPER_SCHEMA = binary_sensor.binary_sensor_schema(
    device_class=DEVICE_CLASS_TAMPER,
    icon="mdi:shield-alert",
).extend(
    {
        cv.Required(CONF_ZONE): cv.int_range(min=1, max=32),
    }
)

# Zone bypass: problem (bypassed = problem state)
ZONE_BYPASS_SCHEMA = binary_sensor.binary_sensor_schema(
    icon="mdi:shield-off",
).extend(
    {
        cv.Required(CONF_ZONE): cv.int_range(min=1, max=32),
    }
)

# Zone alarm memory
ZONE_ALARM_MEMORY_SCHEMA = binary_sensor.binary_sensor_schema(
    icon="mdi:alarm-light",
).extend(
    {
        cv.Required(CONF_ZONE): cv.int_range(min=1, max=32),
    }
)

# Zone tamper memory
ZONE_TAMPER_MEMORY_SCHEMA = binary_sensor.binary_sensor_schema(
    device_class=DEVICE_CLASS_TAMPER,
    icon="mdi:alarm-light",
).extend(
    {
        cv.Required(CONF_ZONE): cv.int_range(min=1, max=32),
    }
)

# Partition alarm: device_class=safety
PARTITION_ALARM_SENSOR_SCHEMA = binary_sensor.binary_sensor_schema(
    device_class=DEVICE_CLASS_SAFETY,
    icon="mdi:bell-ring",
).extend(
    {
        cv.Required(CONF_PARTITION): cv.int_range(min=1, max=8),
    }
)

# Output state
OUTPUT_STATE_SENSOR_SCHEMA = binary_sensor.binary_sensor_schema(
    icon="mdi:electric-switch",
).extend(
    {
        cv.Required(CONF_OUTPUT_NUMBER): cv.int_range(min=1, max=8),
        cv.Optional(CONF_OUTPUT_PANEL_NAME): text_sensor.text_sensor_schema(
            icon="mdi:form-textbox",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)

# Warning sub-schema (each warning with appropriate device_class & icon)
WARNING_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_MAINS_FAILURE): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_POWER,
            icon="mdi:power-plug-off",
        ),
        cv.Optional(CONF_BPI_MISSING): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PROBLEM,
            icon="mdi:chip",
        ),
        cv.Optional(CONF_FUSE_FAULT): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PROBLEM,
            icon="mdi:fuse-alert",
        ),
        cv.Optional(CONF_LOW_BATTERY): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_BATTERY,
            icon="mdi:battery-alert",
        ),
        cv.Optional(CONF_PHONE_LINE_FAULT): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PROBLEM,
            icon="mdi:phone-off",
        ),
        cv.Optional(CONF_DEFAULT_CODES): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PROBLEM,
            icon="mdi:key-alert",
        ),
        cv.Optional(CONF_WIRELESS_FAULT): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PROBLEM,
            icon="mdi:wifi-alert",
        ),
    }
)

# Tamper sub-schema (all tamper-class)
TAMPER_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_TAMPER_ZONE): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_TAMPER,
            icon="mdi:shield-alert",
        ),
        cv.Optional(CONF_TAMPER_FALSE_KEY): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_TAMPER,
            icon="mdi:key-remove",
        ),
        cv.Optional(CONF_TAMPER_BPI): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_TAMPER,
            icon="mdi:chip",
        ),
        cv.Optional(CONF_TAMPER_SYSTEM): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_TAMPER,
            icon="mdi:alert-octagon",
        ),
        cv.Optional(CONF_TAMPER_RF_JAM): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_TAMPER,
            icon="mdi:signal-off",
        ),
        cv.Optional(CONF_TAMPER_WIRELESS): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_TAMPER,
            icon="mdi:wifi-alert",
        ),
    }
)

# Communication: connectivity class
COMMUNICATION_SCHEMA = binary_sensor.binary_sensor_schema(
    device_class=DEVICE_CLASS_CONNECTIVITY,
    icon="mdi:lan-connect",
)

# Siren: sound class
SIREN_SCHEMA = binary_sensor.binary_sensor_schema(
    device_class=DEVICE_CLASS_SOUND,
    icon="mdi:bullhorn",
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_BENTEL_KYO_ID): cv.use_id(BentelKyo),
        cv.Optional(CONF_ZONES): cv.ensure_list(ZONE_SENSOR_SCHEMA),
        cv.Optional(CONF_ZONE_TAMPER): cv.ensure_list(ZONE_TAMPER_SCHEMA),
        cv.Optional(CONF_ZONE_BYPASS): cv.ensure_list(ZONE_BYPASS_SCHEMA),
        cv.Optional(CONF_ZONE_ALARM_MEMORY): cv.ensure_list(ZONE_ALARM_MEMORY_SCHEMA),
        cv.Optional(CONF_ZONE_TAMPER_MEMORY): cv.ensure_list(ZONE_TAMPER_MEMORY_SCHEMA),
        cv.Optional(CONF_PARTITION_ALARM): cv.ensure_list(PARTITION_ALARM_SENSOR_SCHEMA),
        cv.Optional(CONF_WARNINGS): WARNING_SCHEMA,
        cv.Optional(CONF_TAMPER): TAMPER_SCHEMA,
        cv.Optional(CONF_COMMUNICATION): COMMUNICATION_SCHEMA,
        cv.Optional(CONF_SIREN): SIREN_SCHEMA,
        cv.Optional(CONF_OUTPUT_STATE): cv.ensure_list(OUTPUT_STATE_SENSOR_SCHEMA),
    }
)


async def _register_sensor(hub, config, sensor_type_str, index=0):
    """Register a binary sensor with the hub."""
    var = await binary_sensor.new_binary_sensor(config)
    cg.add(hub.register_binary_sensor(var, SENSOR_TYPES[sensor_type_str], index))


async def _register_text_sensor(hub, config, type_str, index):
    """Register a zone diagnostic text sensor with the hub."""
    var = await text_sensor.new_text_sensor(config)
    cg.add(hub.register_text_sensor(var, TEXT_SENSOR_TYPES[type_str], index))


async def to_code(config):
    hub = await cg.get_variable(config[CONF_BENTEL_KYO_ID])

    # Zone sensors (with optional nested diagnostic text sensors)
    if CONF_ZONES in config:
        for zone_conf in config[CONF_ZONES]:
            zone_index = zone_conf[CONF_ZONE] - 1  # Convert to 0-based
            await _register_sensor(hub, zone_conf, "ZONE", zone_index)

            # Nested zone diagnostic text sensors
            if CONF_ZONE_TYPE in zone_conf:
                await _register_text_sensor(hub, zone_conf[CONF_ZONE_TYPE], "TEXT_ZONE_TYPE", zone_index)
            if CONF_PANEL_NAME in zone_conf:
                await _register_text_sensor(hub, zone_conf[CONF_PANEL_NAME], "TEXT_ZONE_NAME", zone_index)
            if CONF_ZONE_PARTITION in zone_conf:
                await _register_text_sensor(hub, zone_conf[CONF_ZONE_PARTITION], "TEXT_ZONE_AREA", zone_index)
            if CONF_ZONE_ESN in zone_conf:
                await _register_text_sensor(hub, zone_conf[CONF_ZONE_ESN], "TEXT_ZONE_ESN", zone_index)

    # Zone tamper, bypass, alarm memory, tamper memory
    zone_type_map = {
        CONF_ZONE_TAMPER: "ZONE_TAMPER",
        CONF_ZONE_BYPASS: "ZONE_BYPASS",
        CONF_ZONE_ALARM_MEMORY: "ZONE_ALARM_MEMORY",
        CONF_ZONE_TAMPER_MEMORY: "ZONE_TAMPER_MEMORY",
    }
    for conf_key, type_str in zone_type_map.items():
        if conf_key in config:
            for zone_conf in config[conf_key]:
                zone_index = zone_conf[CONF_ZONE] - 1
                await _register_sensor(hub, zone_conf, type_str, zone_index)

    # Partition alarm sensors
    if CONF_PARTITION_ALARM in config:
        for part_conf in config[CONF_PARTITION_ALARM]:
            part_index = part_conf[CONF_PARTITION] - 1
            await _register_sensor(hub, part_conf, "PARTITION_ALARM", part_index)

    # Warning sensors
    if CONF_WARNINGS in config:
        warning_map = {
            CONF_MAINS_FAILURE: "WARNING_MAINS_FAILURE",
            CONF_BPI_MISSING: "WARNING_BPI_MISSING",
            CONF_FUSE_FAULT: "WARNING_FUSE_FAULT",
            CONF_LOW_BATTERY: "WARNING_LOW_BATTERY",
            CONF_PHONE_LINE_FAULT: "WARNING_PHONE_LINE_FAULT",
            CONF_DEFAULT_CODES: "WARNING_DEFAULT_CODES",
            CONF_WIRELESS_FAULT: "WARNING_WIRELESS_FAULT",
        }
        for conf_key, type_str in warning_map.items():
            if conf_key in config[CONF_WARNINGS]:
                await _register_sensor(hub, config[CONF_WARNINGS][conf_key], type_str)

    # Tamper sensors
    if CONF_TAMPER in config:
        tamper_map = {
            CONF_TAMPER_ZONE: "TAMPER_ZONE",
            CONF_TAMPER_FALSE_KEY: "TAMPER_FALSE_KEY",
            CONF_TAMPER_BPI: "TAMPER_BPI",
            CONF_TAMPER_SYSTEM: "TAMPER_SYSTEM",
            CONF_TAMPER_RF_JAM: "TAMPER_RF_JAM",
            CONF_TAMPER_WIRELESS: "TAMPER_WIRELESS",
        }
        for conf_key, type_str in tamper_map.items():
            if conf_key in config[CONF_TAMPER]:
                await _register_sensor(hub, config[CONF_TAMPER][conf_key], type_str)

    # Communication sensor
    if CONF_COMMUNICATION in config:
        await _register_sensor(hub, config[CONF_COMMUNICATION], "COMMUNICATION")

    # Siren sensor
    if CONF_SIREN in config:
        await _register_sensor(hub, config[CONF_SIREN], "SIREN")

    # Output state sensors
    if CONF_OUTPUT_STATE in config:
        for out_conf in config[CONF_OUTPUT_STATE]:
            out_index = out_conf[CONF_OUTPUT_NUMBER] - 1
            await _register_sensor(hub, out_conf, "OUTPUT_STATE", out_index)
            if CONF_OUTPUT_PANEL_NAME in out_conf:
                await _register_text_sensor(hub, out_conf[CONF_OUTPUT_PANEL_NAME], "TEXT_OUTPUT_NAME", out_index)
