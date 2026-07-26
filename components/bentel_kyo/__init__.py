"""Bentel KYO alarm panel hub component."""

import subprocess
from pathlib import Path

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

CODEOWNERS = ["@espkyogate"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["alarm_control_panel", "binary_sensor", "button", "switch", "text_sensor"]
MULTI_CONF = False

CONF_BENTEL_KYO_ID = "bentel_kyo_id"

bentel_kyo_ns = cg.esphome_ns.namespace("bentel_kyo")
BentelKyo = bentel_kyo_ns.class_("BentelKyo", cg.PollingComponent, uart.UARTDevice)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(BentelKyo),
        }
    )
    .extend(cv.polling_component_schema("500ms"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


def _get_source_commit() -> str:
    """Git commit of this component's source tree (whatever external_components
    checked out — git ref, branch, or local path), so bug reports can pin down the
    exact revision from the boot log alone instead of bisecting versions."""
    component_dir = Path(__file__).resolve().parent
    try:
        rev = subprocess.run(
            ["git", "-C", str(component_dir), "rev-parse", "--short=12", "HEAD"],
            capture_output=True,
            text=True,
            timeout=5,
            check=False,
        )
        if rev.returncode != 0:
            return "unknown"
        commit = rev.stdout.strip()

        status = subprocess.run(
            ["git", "-C", str(component_dir), "status", "--porcelain"],
            capture_output=True,
            text=True,
            timeout=5,
            check=False,
        )
        if status.returncode == 0 and status.stdout.strip():
            commit += "-dirty"
        return commit
    except (OSError, subprocess.SubprocessError):
        return "unknown"


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    cg.add(var.set_source_commit(_get_source_commit()))
