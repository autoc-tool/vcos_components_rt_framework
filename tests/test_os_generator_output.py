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
"""Os generator file-output behaviour, with the VCOS Studio modules it imports stubbed.

Template content is covered by tests/integration (real Studio + real project).
"""
import importlib
import json
import os
import sys
import types

import pytest

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PLUGIN_CFG_DIR = os.path.join(REPO_ROOT, "tests", "fixtures", "plugin_cfg")


def _stub(name, **attrs):
    module = types.ModuleType(name)
    module.__path__ = []
    module.__dict__.update(attrs)
    return module


class _Logger:
    def __getattr__(self, _name):
        return lambda *args, **kwargs: None


@pytest.fixture
def generator(tmp_path, monkeypatch):
    """Import config_plug_in.generator.generator as components.rt_framework.* like Studio does."""
    pkg_root = tmp_path / "pkg_root"
    (pkg_root / "components").mkdir(parents=True)
    os.symlink(REPO_ROOT, pkg_root / "components" / "rt_framework")
    monkeypatch.syspath_prepend(str(pkg_root))

    logger = _Logger()
    stubs = {
        "func_module": _stub("func_module"),
        "func_module.health_monitor": _stub("func_module.health_monitor"),
        "func_module.health_monitor.logger": _stub("func_module.health_monitor.logger", logger=logger),
        "func_module.generator": _stub("func_module.generator"),
        "func_module.generator.common": _stub("func_module.generator.common"),
        "func_module.generator.common.common": _stub("func_module.generator.common.common", FunctionBase=object),
        "func_module.generator.common.user_code_extract": _stub(
            "func_module.generator.common.user_code_extract", UserCodeExtract=object
        ),
        "components.memory": _stub("components.memory"),
        "components.memory.mempool": _stub("components.memory.mempool"),
        "components.memory.mempool.config_plug_in": _stub("components.memory.mempool.config_plug_in"),
        "components.memory.mempool.config_plug_in.generator": _stub(
            "components.memory.mempool.config_plug_in.generator"
        ),
        "components.memory.mempool.config_plug_in.generator.utils": _stub(
            "components.memory.mempool.config_plug_in.generator.utils", get_os_mempool_all_config_dict=None
        ),
    }
    for name, module in stubs.items():
        monkeypatch.setitem(sys.modules, name, module)

    yield importlib.import_module("components.rt_framework.config_plug_in.generator.generator")

    for name in list(sys.modules):
        if name == "components" or name.startswith("components."):
            del sys.modules[name]


def _sys_config(tmp_path, plugin_cfg_file, target="PC_Common"):
    with open(os.path.join(PLUGIN_CFG_DIR, plugin_cfg_file), encoding="utf-8") as f:
        sys_target_dict = json.load(f)
    gendata = tmp_path / "gen_data"
    (gendata / "Os").mkdir(parents=True)
    return {
        "target": target,
        "gendata_path": str(gendata),
        "path": {
            "bswcallout_path": str(tmp_path / "apps" / "bsw_user_callout"),
            "system_path": str(tmp_path / "system_pack"),
        },
        "sys_target_dict": sys_target_dict,
    }


class _FakeTemplate:
    def __init__(self, name, fail_on):
        self.name = name
        self.fail_on = fail_on

    def render(self, config):
        if self.name == self.fail_on:
            raise KeyError("boom")
        return f"{self.name}:{config['sys_cfg']['target']}\n"


def _fake_environment(fail_on=None):
    class FakeEnv:
        def __init__(self, *args, **kwargs):
            self.filters = {}
            self.globals = {}

        def get_template(self, name):
            return _FakeTemplate(name, fail_on)

    return FakeEnv


def _files(path):
    return sorted(p.name for p in path.iterdir()) if path.exists() else []


def test_missing_platform_data_writes_nothing(generator, tmp_path):
    opc = sys.modules["components.rt_framework.config_plug_in.os_platform_cfg"]
    sys_config = _sys_config(tmp_path, "configs_components_plug_in_cfg.box.json")

    with pytest.raises(opc.OsPlatformCfgError) as exc_info:
        generator.generate_dynamic_code(sys_config, {})

    assert "validate.ignore_option.PC_Common.Os" in str(exc_info.value)
    assert _files(tmp_path / "gen_data" / "Os") == []
    assert not (tmp_path / "apps").exists()


def test_template_failure_writes_nothing(generator, tmp_path, monkeypatch):
    sys_config = _sys_config(tmp_path, "autoc_mcal_components_plug_in_cfg.json")
    monkeypatch.setattr(generator, "prepare_os_base_config",
                        lambda sys_cfg, cfg: {"sys_cfg": sys_cfg})
    monkeypatch.setattr(generator, "Environment", _fake_environment(fail_on="rt_framework.config.jinja2"))

    with pytest.raises(KeyError):
        generator.generate_dynamic_code(sys_config, {})

    assert _files(tmp_path / "gen_data" / "Os") == []
    assert _files(tmp_path / "apps" / "bsw_user_callout") == []


def test_success_writes_all_files(generator, tmp_path, monkeypatch):
    sys_config = _sys_config(tmp_path, "autoc_mcal_components_plug_in_cfg.json")
    monkeypatch.setattr(generator, "prepare_os_base_config",
                        lambda sys_cfg, cfg: {"sys_cfg": sys_cfg})
    monkeypatch.setattr(generator, "Environment", _fake_environment())
    callout_dir = tmp_path / "apps" / "bsw_user_callout"
    callout_dir.mkdir(parents=True)
    (callout_dir / "hook_user_callout.c").write_text("user code\n")

    assert generator.generate_dynamic_code(sys_config, {}) is True

    os_files = _files(tmp_path / "gen_data" / "Os")
    assert "rt_framework.config" in os_files
    assert "task_cfg.c" in os_files
    assert "tricore_irq_dispatch.c" not in os_files
    assert (tmp_path / "gen_data" / "Os" / "rt_framework.config").read_text() == "rt_framework.config.jinja2:PC_Common\n"
    assert (callout_dir / "hook_user_callout.c.bak").read_text() == "user code\n"
    assert (callout_dir / "hook_user_callout.c").read_text().startswith("hook_user_callout.c.jinja2:")


def test_aurix_still_emits_irq_dispatch(generator, tmp_path, monkeypatch):
    sys_config = _sys_config(tmp_path, "configs_components_plug_in_cfg.box.json", target="AURIX2G_TC397")
    monkeypatch.setattr(generator, "prepare_os_base_config",
                        lambda sys_cfg, cfg: {"sys_cfg": sys_cfg})
    monkeypatch.setattr(generator, "Environment", _fake_environment())

    assert generator.generate_dynamic_code(sys_config, {}) is True
    assert "tricore_irq_dispatch.c" in _files(tmp_path / "gen_data" / "Os")


def test_helpers_used_by_memmap_keep_their_signatures(generator, tmp_path):
    """components/memory/memmap imports prepare_os_base_config and render_os_template."""
    import inspect

    assert list(inspect.signature(generator.prepare_os_base_config).parameters) == ["sys_config", "config_dict"]
    assert list(inspect.signature(generator.render_os_template).parameters) == [
        "template", "out_dir", "file_name", "config"]

    generator.render_os_template(_FakeTemplate("ok", None), str(tmp_path), "ok.h", {"sys_cfg": {"target": "T"}})
    assert (tmp_path / "ok.h").read_text() == "ok:T\n"

    with pytest.raises(KeyError):
        generator.render_os_template(_FakeTemplate("bad", "bad"), str(tmp_path), "bad.h", {})
    assert not (tmp_path / "bad.h").exists()
