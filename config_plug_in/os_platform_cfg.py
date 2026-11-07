# This Python file uses the following encoding: utf-8
############################################################################
# Copyright (c) 2025 Li Auto Inc. and its affiliates
# Licensed under the Apache License, Version 2.0(the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
############################################################################
"""Target-platform Os interrupt-level limits.

The limits are platform data owned by the configurator plugin pack that the
project's ``system_path`` points at, under
``components_plug_in_cfg.json -> validate.ignore_option.<target>.Os``.
On project load VCOS Studio narrows ``ignore_option`` to ``<target>`` only when
the pack has an entry for that target; otherwise the whole per-target map is
passed through, so both shapes have to be handled here.

This module must stay free of VCOS Studio imports: it is shared by the
generator and the validation rules and is unit-tested standalone.
"""
import os

PLUGIN_CFG_FILE = "components_plug_in_cfg.json"

INT_LEVEL_KEYS = (
    "max_interrupt_priority",
    "min_interrupt_priority",
    "max_cat0_interrupt_priority",
    "max_tp_interrupt_priority",
    "max_cat1_interrupt_priority",
    "max_cat2_interrupt_priority",
)


class OsPlatformCfgError(ValueError):
    """Platform Os data for the project target is missing or unusable."""

    def __init__(self, problem, location, fix):
        self.problem = problem
        self.location = location
        self.fix = fix
        super().__init__(f"{problem} Location: {location}. Fix: {fix}")


def _cfg_file(system_path):
    if not system_path:
        return f"<project system_path>/{PLUGIN_CFG_FILE}"
    return os.path.join(system_path, PLUGIN_CFG_FILE)


def _select_target_os_cfg(ignore_option, target):
    """Return (found, os_cfg) for both the raw and the Studio-narrowed shape."""
    if isinstance(ignore_option.get(target), dict):
        return True, ignore_option[target].get("Os")
    if "Os" in ignore_option and not any(isinstance(v, dict) and "Os" in v for v in ignore_option.values()):
        return True, ignore_option["Os"]
    return False, None


def _check_values(os_cfg):
    problems = []
    missing = [k for k in INT_LEVEL_KEYS if k not in os_cfg]
    if missing:
        problems.append(f"missing keys {missing}")
    bad_type = [
        f"{k}={os_cfg[k]!r}"
        for k in INT_LEVEL_KEYS
        if k in os_cfg and (isinstance(os_cfg[k], bool) or not isinstance(os_cfg[k], int))
    ]
    if bad_type:
        problems.append(f"non-integer values {bad_type}")
    if problems:
        return problems

    highest = os_cfg["max_interrupt_priority"]
    lowest = os_cfg["min_interrupt_priority"]
    if highest == lowest:
        problems.append(
            f"max_interrupt_priority and min_interrupt_priority are both {highest}; "
            "they must differ to define the priority direction"
        )
        return problems
    span_lo, span_hi = min(highest, lowest), max(highest, lowest)
    out_of_span = [
        f"{k}={os_cfg[k]}"
        for k in INT_LEVEL_KEYS[2:]
        if not span_lo <= os_cfg[k] <= span_hi
    ]
    if out_of_span:
        problems.append(
            f"values outside the [min_interrupt_priority, max_interrupt_priority] span "
            f"[{lowest}, {highest}]: {out_of_span}"
        )
    return problems


def resolve_os_int_level_cfg(sys_target_dict, target, system_path=""):
    """Return the validated interrupt-level limits of ``target``.

    Raises OsPlatformCfgError naming the config file, the JSON path, the
    problem and how to fix it.
    """
    cfg_file = _cfg_file(system_path)
    json_path = f"validate.ignore_option.{target or '<target>'}.Os"
    location = f"{cfg_file} -> {json_path}"
    required = ", ".join(INT_LEVEL_KEYS)

    if not target:
        raise OsPlatformCfgError(
            "The project has no target, so the Os interrupt-level limits cannot be selected.",
            location,
            "Set 'target' in the .vcosproject through VCOS Studio / AutoC project settings.",
        )

    sys_target_dict = sys_target_dict if isinstance(sys_target_dict, dict) else {}
    ignore_option = sys_target_dict.get("validate", {}).get("ignore_option")
    if not isinstance(ignore_option, dict):
        ignore_option = {}

    found, os_cfg = _select_target_os_cfg(ignore_option, target)
    if not found:
        known = sorted(k for k, v in ignore_option.items() if isinstance(v, dict))
        declared = target in (sys_target_dict.get("target") or {})
        declared_note = (
            f" Target '{target}' is declared under 'target' in that file, but its platform data is not."
            if declared
            else ""
        )
        raise OsPlatformCfgError(
            f"Os interrupt-level limits for target '{target}' are not defined by the configurator plugin pack "
            f"at system_path (validate.ignore_option only has: {', '.join(known) or 'none'}).{declared_note}",
            location,
            f"Point the project's system_path at the configurator plugin pack that owns target '{target}' "
            f"(the one that defines {json_path}), or have the owner of this pack add {json_path} with "
            f"{required} taken from the '{target}' interrupt controller. Do not copy another target's values.",
        )

    if not isinstance(os_cfg, dict):
        raise OsPlatformCfgError(
            f"Target '{target}' has a validate.ignore_option entry but no 'Os' object in it.",
            location,
            f"Add {json_path} with {required} to the plugin pack that owns target '{target}'.",
        )

    problems = _check_values(os_cfg)
    if problems:
        raise OsPlatformCfgError(
            f"Os interrupt-level limits for target '{target}' are invalid: {'; '.join(problems)}.",
            location,
            f"Correct {json_path} in the plugin pack that owns target '{target}' so it defines integer "
            f"{required}, with max != min and every category limit inside [min, max].",
        )

    return {k: os_cfg[k] for k in INT_LEVEL_KEYS}
