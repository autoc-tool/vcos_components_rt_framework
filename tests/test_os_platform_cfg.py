############################################################################
#  Copyright (c) 2025 Li Auto Inc. and its affiliates
#  Licensed under the Apache License, Version 2.0(the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at

#      http://www.apache.org/licenses/LICENSE-2.0

#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
############################################################################
"""Unit tests for config_plug_in/os_platform_cfg.py (no VCOS Studio needed)."""
import copy
import importlib.util
import json
import os

import pytest

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PLUGIN_CFG_DIR = os.path.join(REPO_ROOT, "tests", "fixtures", "plugin_cfg")
# components/configs/configurator_plug_in/components_plug_in_cfg.json as found on
# the box: upstream HaloOS plus a PC_Common 'target' entry, without platform data.
BOX_CONFIGS_CFG = os.path.join(PLUGIN_CFG_DIR, "configs_components_plug_in_cfg.box.json")
# components/autoc_mcal/configurator_plug_in/components_plug_in_cfg.json (PC_Common owner pack).
AUTOC_MCAL_CFG = os.path.join(PLUGIN_CFG_DIR, "autoc_mcal_components_plug_in_cfg.json")

_spec = importlib.util.spec_from_file_location(
    "os_platform_cfg", os.path.join(REPO_ROOT, "config_plug_in", "os_platform_cfg.py")
)
opc = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(opc)


def load(path):
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def narrowed(sys_target_dict, target):
    """Shape produced by VCOS Studio's load_project when the target has an entry."""
    narrowed_dict = copy.deepcopy(sys_target_dict)
    narrowed_dict["validate"]["ignore_option"] = narrowed_dict["validate"]["ignore_option"][target]
    return narrowed_dict


def expected_limits(sys_target_dict, target):
    os_cfg = sys_target_dict["validate"]["ignore_option"][target]["Os"]
    return {k: os_cfg[k] for k in opc.INT_LEVEL_KEYS}


class TestPcCommonValid:
    def test_owner_pack_raw_map(self):
        cfg = load(AUTOC_MCAL_CFG)
        got = opc.resolve_os_int_level_cfg(cfg, "PC_Common", "/vcos/components/autoc_mcal/configurator_plug_in")
        assert got == expected_limits(cfg, "PC_Common")

    def test_owner_pack_narrowed_by_studio(self):
        cfg = load(AUTOC_MCAL_CFG)
        got = opc.resolve_os_int_level_cfg(narrowed(cfg, "PC_Common"), "PC_Common")
        assert got == expected_limits(cfg, "PC_Common")


class TestPcCommonMissing:
    """The box state that produced `KeyError: 'max_interrupt_priority'`."""

    SYSTEM_PATH = "/home/box/haloos/haloosspace/vcos/components/configs/configurator_plug_in"

    def test_raises_explanatory_error(self):
        cfg = load(BOX_CONFIGS_CFG)
        assert "PC_Common" in cfg["target"]
        assert "PC_Common" not in cfg["validate"]["ignore_option"]

        with pytest.raises(opc.OsPlatformCfgError) as exc_info:
            opc.resolve_os_int_level_cfg(cfg, "PC_Common", self.SYSTEM_PATH)

        err = exc_info.value
        assert not isinstance(err, KeyError)
        cfg_file = os.path.join(self.SYSTEM_PATH, "components_plug_in_cfg.json")
        assert err.location == f"{cfg_file} -> validate.ignore_option.PC_Common.Os"
        assert "target 'PC_Common'" in err.problem
        assert "declared under 'target'" in err.problem
        for known in ("AURIX2G_TC397", "E3650", "FC7300F8MDT", "J6E", "Sim_Common"):
            assert known in err.problem
        assert "system_path" in err.fix
        assert "Do not copy another target's values" in err.fix
        for key in opc.INT_LEVEL_KEYS:
            assert key in err.fix
        assert cfg_file in str(err)

    def test_undeclared_target(self):
        cfg = load(BOX_CONFIGS_CFG)
        del cfg["target"]["PC_Common"]
        with pytest.raises(opc.OsPlatformCfgError) as exc_info:
            opc.resolve_os_int_level_cfg(cfg, "PC_Common", self.SYSTEM_PATH)
        assert "declared under 'target'" not in exc_info.value.problem

    @pytest.mark.parametrize("sys_target_dict", [None, {}, {"validate": {}}, {"validate": {"ignore_option": None}}])
    def test_no_ignore_option_at_all(self, sys_target_dict):
        with pytest.raises(opc.OsPlatformCfgError, match="only has: none"):
            opc.resolve_os_int_level_cfg(sys_target_dict, "PC_Common", self.SYSTEM_PATH)

    def test_no_target(self):
        with pytest.raises(opc.OsPlatformCfgError, match="no target"):
            opc.resolve_os_int_level_cfg(load(AUTOC_MCAL_CFG), "", self.SYSTEM_PATH)

    def test_unknown_system_path_still_names_file(self):
        with pytest.raises(opc.OsPlatformCfgError) as exc_info:
            opc.resolve_os_int_level_cfg(load(BOX_CONFIGS_CFG), "PC_Common", "")
        assert "<project system_path>/components_plug_in_cfg.json" in exc_info.value.location


class TestInvalidPlatformData:
    @staticmethod
    def pc_cfg(mutate):
        cfg = load(AUTOC_MCAL_CFG)
        mutate(cfg["validate"]["ignore_option"]["PC_Common"])
        return cfg

    def test_entry_without_os(self):
        cfg = self.pc_cfg(lambda entry: entry.pop("Os"))
        with pytest.raises(opc.OsPlatformCfgError, match="no 'Os' object"):
            opc.resolve_os_int_level_cfg(cfg, "PC_Common")

    @pytest.mark.parametrize("key", opc.INT_LEVEL_KEYS)
    def test_missing_key(self, key):
        cfg = self.pc_cfg(lambda entry: entry["Os"].pop(key))
        with pytest.raises(opc.OsPlatformCfgError, match=f"missing keys \\['{key}'\\]"):
            opc.resolve_os_int_level_cfg(cfg, "PC_Common")

    @pytest.mark.parametrize("value", ["255", 255.0, True, None])
    def test_non_integer(self, value):
        cfg = self.pc_cfg(lambda entry: entry["Os"].__setitem__("max_cat2_interrupt_priority", value))
        with pytest.raises(opc.OsPlatformCfgError, match="non-integer values"):
            opc.resolve_os_int_level_cfg(cfg, "PC_Common")

    def test_max_equals_min(self):
        cfg = self.pc_cfg(lambda entry: entry["Os"].__setitem__("min_interrupt_priority", 255))
        with pytest.raises(opc.OsPlatformCfgError, match="must differ"):
            opc.resolve_os_int_level_cfg(cfg, "PC_Common")

    def test_category_outside_span(self):
        cfg = self.pc_cfg(lambda entry: entry["Os"].__setitem__("max_cat1_interrupt_priority", 300))
        with pytest.raises(opc.OsPlatformCfgError, match="max_cat1_interrupt_priority=300"):
            opc.resolve_os_int_level_cfg(cfg, "PC_Common")


class TestExistingPlatformsUnaffected:
    """Every target already shipped by components/configs keeps its own limits."""

    TARGETS = ["AURIX2G_TC397", "FC7300F8MDT", "E3650", "J6E", "Sim_Common"]

    @pytest.mark.parametrize("target", TARGETS)
    def test_raw_map(self, target):
        cfg = load(BOX_CONFIGS_CFG)
        assert opc.resolve_os_int_level_cfg(cfg, target) == expected_limits(cfg, target)

    @pytest.mark.parametrize("target", TARGETS)
    def test_narrowed_by_studio(self, target):
        cfg = load(BOX_CONFIGS_CFG)
        assert opc.resolve_os_int_level_cfg(narrowed(cfg, target), target) == expected_limits(cfg, target)

    def test_descending_priority_platform(self):
        # ARM targets use 0 as the highest priority (max < min); FC7300's levels
        # are not monotonic across categories and must stay accepted.
        cfg = load(BOX_CONFIGS_CFG)
        got = opc.resolve_os_int_level_cfg(cfg, "FC7300F8MDT")
        assert got["max_interrupt_priority"] < got["min_interrupt_priority"]
