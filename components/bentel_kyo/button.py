"""Bentel KYO button platform — reread config, arm/disarm all, arm presets, output control, zone control, datetime sync."""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import CONF_ID, CONF_TYPE, ENTITY_CATEGORY_CONFIG

from . import bentel_kyo_ns, BentelKyo, CONF_BENTEL_KYO_ID

BentelKyoRereadConfigButton = bentel_kyo_ns.class_(
    "BentelKyoRereadConfigButton", button.Button, cg.Component
)

BentelKyoArmAllButton = bentel_kyo_ns.class_(
    "BentelKyoArmAllButton", button.Button, cg.Component
)

BentelKyoDisarmAllButton = bentel_kyo_ns.class_(
    "BentelKyoDisarmAllButton", button.Button, cg.Component
)

BentelKyoArmPresetButton = bentel_kyo_ns.class_(
    "BentelKyoArmPresetButton", button.Button, cg.Component
)

BentelKyoResetAlarmsButton = bentel_kyo_ns.class_(
    "BentelKyoResetAlarmsButton", button.Button, cg.Component
)

BentelKyoReadEventLogButton = bentel_kyo_ns.class_(
    "BentelKyoReadEventLogButton", button.Button, cg.Component
)

BentelKyoMemoryScanButton = bentel_kyo_ns.class_(
    "BentelKyoMemoryScanButton", button.Button, cg.Component
)

BentelKyoDumpConfigButton = bentel_kyo_ns.class_(
    "BentelKyoDumpConfigButton", button.Button, cg.Component
)

BentelKyoActivateOutputButton = bentel_kyo_ns.class_(
    "BentelKyoActivateOutputButton", button.Button, cg.Component
)

BentelKyoDeactivateOutputButton = bentel_kyo_ns.class_(
    "BentelKyoDeactivateOutputButton", button.Button, cg.Component
)

BentelKyoPulseOutputButton = bentel_kyo_ns.class_(
    "BentelKyoPulseOutputButton", button.Button, cg.Component
)

BentelKyoIncludeZoneButton = bentel_kyo_ns.class_(
    "BentelKyoIncludeZoneButton", button.Button, cg.Component
)

BentelKyoExcludeZoneButton = bentel_kyo_ns.class_(
    "BentelKyoExcludeZoneButton", button.Button, cg.Component
)

BentelKyoSyncDatetimeButton = bentel_kyo_ns.class_(
    "BentelKyoSyncDatetimeButton", button.Button, cg.Component
)

CONF_ARM_TYPE = "arm_type"
CONF_PARTITIONS = "partitions"
CONF_OUTPUT_NUMBER = "output_number"
CONF_PULSE_TIME = "pulse_time"
CONF_ZONE_NUMBER = "zone_number"

BUTTON_TYPES = {
    "reread_config": (BentelKyoRereadConfigButton, "mdi:refresh", ENTITY_CATEGORY_CONFIG),
    "arm_all_away": (BentelKyoArmAllButton, "mdi:shield-lock", None),
    "arm_all_home": (BentelKyoArmAllButton, "mdi:shield-home", None),
    "arm_all_night": (BentelKyoArmAllButton, "mdi:shield-moon", None),
    "disarm_all": (BentelKyoDisarmAllButton, "mdi:shield-off", None),
    "arm_preset": (BentelKyoArmPresetButton, "mdi:shield-account", None),
    "reset_alarms": (BentelKyoResetAlarmsButton, "mdi:bell-cancel", None),
    "read_event_log": (BentelKyoReadEventLogButton, "mdi:history", ENTITY_CATEGORY_CONFIG),
    "memory_scan": (BentelKyoMemoryScanButton, "mdi:magnify-scan", ENTITY_CATEGORY_CONFIG),
    "dump_config": (BentelKyoDumpConfigButton, "mdi:file-document-outline", ENTITY_CATEGORY_CONFIG),
    "activate_output": (BentelKyoActivateOutputButton, "mdi:electric-switch", None),
    "deactivate_output": (BentelKyoDeactivateOutputButton, "mdi:electric-switch-closed", None),
    "pulse_output": (BentelKyoPulseOutputButton, "mdi:pulse", None),
    "include_zone": (BentelKyoIncludeZoneButton, "mdi:shield-plus", None),
    "exclude_zone": (BentelKyoExcludeZoneButton, "mdi:shield-minus", None),
    "sync_datetime": (BentelKyoSyncDatetimeButton, "mdi:clock-check", ENTITY_CATEGORY_CONFIG),
}

ARM_TYPE_MAP = {
    "arm_all_away": 1,
    "arm_all_home": 2,
    "arm_all_night": 3,
}

PARTITION_MODE_OPTIONS = ["away", "home", "stay", "night", "disarm"]


def _validate_partitions(value):
    if not isinstance(value, dict):
        raise cv.Invalid("Expected a mapping of partition: mode")
    result = {}
    for k, v in value.items():
        try:
            partition = int(k)
        except (ValueError, TypeError):
            raise cv.Invalid(f"Invalid partition number: {k}")
        if partition < 1 or partition > 8:
            raise cv.Invalid(f"Partition {partition} must be between 1 and 8")
        v = str(v).lower()
        if v not in PARTITION_MODE_OPTIONS:
            raise cv.Invalid(
                f"Invalid mode '{v}' for partition {partition}. "
                f"Options: {PARTITION_MODE_OPTIONS}"
            )
        result[partition] = v
    return result


def _button_schema(type_str):
    cls, icon, entity_cat = BUTTON_TYPES[type_str]
    kwargs = {"icon": icon}
    if entity_cat is not None:
        kwargs["entity_category"] = entity_cat

    base = (
        button.button_schema(cls, **kwargs)
        .extend(
            {
                cv.GenerateID(CONF_BENTEL_KYO_ID): cv.use_id(BentelKyo),
            }
        )
        .extend(cv.COMPONENT_SCHEMA)
    )

    if type_str == "arm_preset":
        base = base.extend(
            {
                cv.Required(CONF_PARTITIONS): _validate_partitions,
            }
        )

    if type_str in ("activate_output", "deactivate_output"):
        base = base.extend(
            {
                cv.Required(CONF_OUTPUT_NUMBER): cv.int_range(min=1, max=8),
            }
        )

    if type_str == "pulse_output":
        base = base.extend(
            {
                cv.Required(CONF_OUTPUT_NUMBER): cv.int_range(min=1, max=8),
                cv.Optional(CONF_PULSE_TIME, default="1000ms"): cv.positive_time_period_milliseconds,
            }
        )

    if type_str in ("include_zone", "exclude_zone"):
        base = base.extend(
            {
                cv.Required(CONF_ZONE_NUMBER): cv.int_range(min=1, max=32),
            }
        )

    return base


CONFIG_SCHEMA = cv.typed_schema(
    {t: _button_schema(t) for t in BUTTON_TYPES},
    key=CONF_TYPE,
)


async def to_code(config):
    var = await button.new_button(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_BENTEL_KYO_ID])
    cg.add(var.set_parent(parent))

    type_str = config[CONF_TYPE]
    if type_str in ARM_TYPE_MAP:
        cg.add(var.set_arm_type(ARM_TYPE_MAP[type_str]))
    elif type_str == "arm_preset":
        partitions = config[CONF_PARTITIONS]
        total = 0
        partial = 0
        partial_d0 = 0
        for partition, mode in partitions.items():
            bit = 1 << (partition - 1)
            if mode == "away":
                total |= bit
            elif mode in ("home", "stay"):
                partial |= bit
            elif mode == "night":
                partial_d0 |= bit
            # "disarm" — bit stays 0 in all masks
        cg.add(var.set_masks(total, partial, partial_d0))
    elif type_str in ("activate_output", "deactivate_output"):
        cg.add(var.set_output_number(config[CONF_OUTPUT_NUMBER]))
    elif type_str == "pulse_output":
        cg.add(var.set_output_number(config[CONF_OUTPUT_NUMBER]))
        cg.add(var.set_pulse_time(config[CONF_PULSE_TIME].total_milliseconds))
    elif type_str in ("include_zone", "exclude_zone"):
        cg.add(var.set_zone_number(config[CONF_ZONE_NUMBER]))
