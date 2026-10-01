"""Bentel KYO alarm panel hub component."""

import os
import re
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


def _git(*args: str) -> str | None:
    """Run git in this component's source tree; stdout, or None if it failed."""
    component_dir = Path(__file__).resolve().parent
    try:
        res = subprocess.run(
            ["git", "-C", str(component_dir), *args],
            capture_output=True,
            text=True,
            timeout=5,
            check=False,
            # Never block the build on a credentials prompt (ls-remote on a private fork)
            env={**os.environ, "GIT_TERMINAL_PROMPT": "0"},
        )
    except (OSError, subprocess.SubprocessError):
        return None
    return res.stdout.strip() if res.returncode == 0 else None


def _get_source_commit() -> str:
    """Git commit of this component's source tree (whatever external_components
    checked out — git ref, branch, or local path), so bug reports can pin down the
    exact revision from the boot log alone instead of bisecting versions."""
    commit = _git("rev-parse", "--short=12", "HEAD")
    if commit is None:
        return "unknown"
    if _git("status", "--porcelain"):
        commit += "-dirty"
    return commit


def _get_fetched_ref(head: str) -> tuple[str | None, str] | None:
    """(kind, name) of the ref external_components fetched, kind being "tag", "branch"
    or None (a PR ref or a commit sha), if it's still what's checked out.

    It checks out github://...@<ref> with `git fetch --depth=1 origin <ref>` +
    `git reset --hard FETCH_HEAD`, which doesn't create the tag locally (so
    `git describe` can't see it): the ref name only survives in FETCH_HEAD, as
    "<sha>\\t\\ttag 'v2026.9.27' of https://github.com/...".
    """
    git_dir = _git("rev-parse", "--absolute-git-dir")
    if git_dir is None:
        return None
    try:
        first_line = (Path(git_dir) / "FETCH_HEAD").read_text().partition("\n")[0]
    except OSError:
        return None  # first clone without a ref: nothing was ever fetched by name
    m = re.match(r"([0-9a-f]+)\t[^\t]*\t(?:(tag|branch) )?'(.+)' of ", first_line)
    # Stale if the checkout moved since (e.g. ESPHome reverted a failed update)
    if m is None or m.group(1) != head:
        return None
    return m.group(2), m.group(3)


def _get_remote_tag(head: str) -> str | None:
    """Newest tag pointing at HEAD on the remote, for when the local clone doesn't know
    it: a tag created after its commit was fetched never arrives with `git fetch origin
    master` (github://...@master), nor within the `refresh` window, where ESPHome doesn't
    fetch at all — so building master right after tagging a release reported
    "master@<commit>". Needs the network: best effort, None when offline."""
    out = _git("ls-remote", "--tags", "--sort=-version:refname", "origin")
    if not out:
        return None
    for line in out.splitlines():
        sha, _, ref = line.partition("\t")
        if sha == head:
            # Annotated tags match on their peeled "refs/tags/<name>^{}" line
            return ref.removeprefix("refs/tags/").removesuffix("^{}")
    return None


def _get_component_version(commit: str) -> str:
    """Release of espkyogate this firmware was built from: the git tag the source was
    fetched at (github://lorenzo-deluca/espkyogate@v2026.9.27 -> "v2026.9.27"), so users
    can tell which component release is running — ESPHome itself (its `version` text
    sensor, the boot log) only reports the ESPHome version (issue #133). Untagged
    checkouts (master, a PR, a local tree) report "<ref>@<commit>" instead."""
    head = _git("rev-parse", "HEAD")
    if head is None:
        return "unknown"
    dirty = "-dirty" if commit.endswith("-dirty") else ""

    fetched = _get_fetched_ref(head)
    if fetched is not None and fetched[0] == "tag":
        return fetched[1] + dirty

    # A tag the local repo does know (a full checkout, or a shallow clone whose tip was
    # already tagged), else one the remote has on this commit
    tag = _git("describe", "--tags", "--exact-match", "HEAD") or _get_remote_tag(head)
    if tag:
        return tag + dirty

    # Not a release. After `reset --hard FETCH_HEAD` the local branch is still the
    # clone's default one whatever was fetched, so only trust it if nothing was.
    if fetched is not None:
        ref = fetched[1]
    else:
        ref = _git("rev-parse", "--abbrev-ref", "HEAD")  # "HEAD" when detached
    if not ref or ref == "HEAD" or head.startswith(ref):
        return commit
    return f"{ref}@{commit}"


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    commit = _get_source_commit()
    cg.add(var.set_source_commit(commit))
    cg.add(var.set_component_version(_get_component_version(commit)))
