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
from func_module.validation.common.basic_rule import BasicRule, timing_decorator
from components.rt_framework.config_plug_in.os_platform_cfg import OsPlatformCfgError, resolve_os_int_level_cfg


class Os018001Rule(BasicRule):
    """The project target must provide usable Os interrupt-level limits (consumed by codegen and Os013008)."""
    ruleset_name = "Os018001"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        sys_info = validation_data.sys_info_class
        self.target = getattr(sys_info, "target", "")
        self.system_path = getattr(sys_info, "system_path", "")
        self.system_target_info = getattr(sys_info, "system_target_info", {})
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "Os018001: target platform Os interrupt-level limits must be defined."

    @timing_decorator
    def validation_run(self):
        param_path = "/ActiveEcuC/Os"
        try:
            int_level_cfg = resolve_os_int_level_cfg(self.system_target_info, self.target, self.system_path)
        except OsPlatformCfgError as e:
            self.__class__.validation_result = False
            self.vlog(
                self.validation_code,
                self.validation_summary,
                f"{e.problem} Location: {e.location}.",
                param_path,
                "fail",
                [],
                [e.fix],
            )
            return

        self.__class__.validation_result = True
        self.vlog(
            self.validation_code,
            self.validation_summary,
            f"Target '{self.target}' interrupt-level limits: {int_level_cfg}.",
            param_path,
            "pass",
        )
