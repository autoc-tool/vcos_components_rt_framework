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
"""Os validate + generate through real VCOS Studio on the minimal_system_re (PC_Common) project.

Opt-in; needs a VCOS checkout and a Python with Studio's requirements:
  VCOS_STUDIO_CONFIGURATOR  .../vcos/vcos_studio/configurator
  VCOS_COMPONENTS_ROOT      .../vcos/components (configs, autoc_mcal, runtime, communication,
                            memory, security, maintaining, ...); rt_framework is taken from this repo
  VCOS_STUDIO_PYTHON        interpreter for Studio (default: the one running pytest)

The project fixture is the AutoC-generated config verbatim; it is copied into a temporary
tree laid out like the box (examples/<project>/system_cfg/pc) so its relative paths resolve.
"""
import copy
import json
import os
import shutil
import subprocess
import sys

import pytest

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
FIXTURES = os.path.join(REPO_ROOT, "tests", "fixtures")
PROJECT_FIXTURE = os.path.join(FIXTURES, "minimal_system_re_pc")
BOX_CONFIGS_CFG = os.path.join(FIXTURES, "plugin_cfg", "configs_components_plug_in_cfg.box.json")
RUNNER = os.path.join(os.path.dirname(os.path.abspath(__file__)), "studio_runner.py")

STUDIO = os.environ.get("VCOS_STUDIO_CONFIGURATOR", "")
COMPONENTS = os.environ.get("VCOS_COMPONENTS_ROOT", "")
PYTHON = os.environ.get("VCOS_STUDIO_PYTHON", sys.executable)

pytestmark = pytest.mark.skipif(
    not (os.path.isdir(STUDIO) and os.path.isdir(COMPONENTS)),
    reason="set VCOS_STUDIO_CONFIGURATOR and VCOS_COMPONENTS_ROOT to run Studio integration tests",
)

INT_LEVEL_MACROS = {
    "max_interrupt_priority": "CONFIG_RT_FRAMEWORK_MAX_INT_LEVEL",
    "min_interrupt_priority": "CONFIG_RT_FRAMEWORK_MIN_INT_LEVEL",
    "max_cat2_interrupt_priority": "CONFIG_RT_FRAMEWORK_MAX_CAT2_INT_LEVEL",
    "max_cat1_interrupt_priority": "CONFIG_RT_FRAMEWORK_MAX_CAT1_INT_LEVEL",
    "max_tp_interrupt_priority": "CONFIG_RT_FRAMEWORK_MAX_TP_INT_LEVEL",
    "max_cat0_interrupt_priority": "CONFIG_RT_FRAMEWORK_MAX_CAT0_INT_LEVEL",
}


def _load_json(path):
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def _owner_pc_common_os():
    owner = _load_json(os.path.join(COMPONENTS, "autoc_mcal", "configurator_plug_in", "components_plug_in_cfg.json"))
    return owner["validate"]["ignore_option"]["PC_Common"]


def _build_tree(tmp_path, configs_plugin_cfg):
    vcos = tmp_path / "vcos"
    components = vcos / "components"
    components.mkdir(parents=True)
    os.symlink(os.path.dirname(os.path.abspath(STUDIO)), vcos / "vcos_studio")
    for name in os.listdir(COMPONENTS):
        if name in ("rt_framework", "configs"):
            continue
        os.symlink(os.path.join(COMPONENTS, name), components / name)
    os.symlink(REPO_ROOT, components / "rt_framework")
    shutil.copytree(os.path.join(COMPONENTS, "configs"), components / "configs", symlinks=True)
    cfg_file = components / "configs" / "configurator_plug_in" / "components_plug_in_cfg.json"
    cfg_file.write_text(json.dumps(configs_plugin_cfg, indent=4), encoding="utf-8")

    example = vcos / "examples" / "minimal_system_re"
    project_dir = example / "system_cfg" / "pc"
    shutil.copytree(PROJECT_FIXTURE, project_dir)
    (project_dir / "gen_data").mkdir()
    (project_dir / "log").mkdir()
    (example / "apps" / "asw_swc").mkdir(parents=True)
    (example / "apps" / "bsw_user_callout").mkdir(parents=True)
    return project_dir, str(cfg_file)


def _run(project_dir, *modules):
    proc = subprocess.run(
        [PYTHON, RUNNER, os.path.abspath(STUDIO), str(project_dir / "minimal_system_re.vcosproject"), *modules],
        capture_output=True, text=True, timeout=900,
    )
    lines = [line for line in proc.stdout.splitlines() if line.startswith("RESULT_JSON=")]
    assert lines, f"runner produced no result\nstdout:\n{proc.stdout[-4000:]}\nstderr:\n{proc.stderr[-4000:]}"
    return json.loads(lines[-1][len("RESULT_JSON="):])


def _os_files(project_dir):
    os_dir = project_dir / "gen_data" / "Os"
    return {p.name: p.stat().st_size for p in os_dir.iterdir()} if os_dir.exists() else {}


def test_box_state_pc_common_without_platform_data(tmp_path):
    """components/configs declares PC_Common but has no validate.ignore_option.PC_Common (box state)."""
    project_dir, cfg_file = _build_tree(tmp_path, _load_json(BOX_CONFIGS_CFG))

    result = _run(project_dir, "Os")

    assert result["load"] is True
    assert result["validate"] == {"Os": False}
    os018001 = [f for f in result["failures"] if f["mark"] == "Os018001"]
    assert len(os018001) == 1, result["failures"]
    assert f"{cfg_file} -> validate.ignore_option.PC_Common.Os" in os018001[0]["message"]
    assert "system_path" in os018001[0]["fix"][0]

    assert result["generate"] is False
    assert any("OsPlatformCfgError" in e for e in result["errors"]), result["errors"]
    assert not any("KeyError" in e for e in result["errors"]), result["errors"]
    assert _os_files(project_dir) == {}, "failed Os generation must not leave partial or empty files"


def test_pc_common_with_platform_data(tmp_path):
    configs_cfg = copy.deepcopy(_load_json(BOX_CONFIGS_CFG))
    pc_common = _owner_pc_common_os()
    configs_cfg["validate"]["ignore_option"]["PC_Common"] = pc_common
    project_dir, _ = _build_tree(tmp_path, configs_cfg)

    # Memmap imports rt_framework's generator helpers, so it is generated alongside.
    result = _run(project_dir, "Os", "Memmap")

    assert result["load"] is True
    assert result["validate"] == {"Os": True, "Memmap": True}, result["failures"]
    assert result["generate"] is True, result["errors"]

    files = _os_files(project_dir)
    assert {"rt_framework.config", "task_cfg.c", "isr_cfg.c", "core_cfg.c"} <= set(files)
    assert not [name for name, size in files.items() if size == 0]
    assert any((project_dir / "gen_data" / "Memmap").iterdir())
    rt_cfg = (project_dir / "gen_data" / "Os" / "rt_framework.config").read_text().splitlines()
    for key, macro in INT_LEVEL_MACROS.items():
        assert f"{macro}={pc_common['Os'][key]}" in rt_cfg
