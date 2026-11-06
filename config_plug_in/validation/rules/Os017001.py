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
from durable.lang import *
from durable.engine import *
from .Os_ruleutils import OsRuleutils

class Os017001Rule(BasicRule):
    """检查Bsw App和OsCoreEcucIdRef是否一致"""
    ruleset_name = "Os017001"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.ecuc_dict = validation_data.bsw_ecuc_dict['EcuC']['ArPackages']['ActiveEcuC']['EcuC']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = (
            "Os017001: OsCoreBswApplicationRef must resolve to the same core id as OsCoreEcucIdRef "
            "(BSW OsApplication must be the local-core app for that EcucCoreDefinition)."
        )
        self.durable_init()

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            with ruleset(self.__class__.ruleset_name):
                @when_all(+m.app_name == '')
                def action(c):
                    self.__class__.validation_result = True

    @timing_decorator
    def validation_run(self):

        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            if "/Os/OsCore" not in os_instance.get("DefinitionRef", ""):
                continue

            path = "/ActiveEcuC/Os/" + os_key
            bsw_ref = (os_instance.get("OsCoreBswApplicationRef") or {}).get("ValueRef")
            ecuc_ref = (os_instance.get("OsCoreEcucIdRef") or {}).get("ValueRef")

            if not bsw_ref or not ecuc_ref:
                missing = []
                if not bsw_ref:
                    missing.append("OsCoreBswApplicationRef")
                if not ecuc_ref:
                    missing.append("OsCoreEcucIdRef")
                self.__class__.validation_result = False
                self.vlog(
                    self.validation_code,
                    self.validation_summary,
                    (
                        f"OsCore '{os_key}': missing {', '.join(missing)}. "
                        f"Set OsCoreBswApplicationRef to the OsApplication that owns BSW on this core, "
                        f"and OsCoreEcucIdRef to the matching EcucCoreDefinition "
                        f"(comparison: BSW-app core id must equal EcucCore id)."
                    ),
                    path,
                    "fail",
                    [],
                    [
                        f"On {path}: set OsCoreBswApplicationRef and OsCoreEcucIdRef so both point at the same core."
                    ],
                )
                continue

            bsw_app_name = bsw_ref.split('/')[-1]
            ecuc_core = ecuc_ref.split('/')[-1]
            try:
                bsw_app_core_id = OsRuleutils().get_core_number(bsw_app_name, self.ecuc_os_dict, self.ecuc_dict)
                ecuc_core_id = OsRuleutils().get_core_id_from_ecuc_core_def(self.ecuc_dict, ecuc_core)
            except Exception as e:
                self.__class__.validation_result = False
                self.vlog(
                    self.validation_code,
                    self.validation_summary,
                    (
                        f"OsCore '{os_key}': cannot resolve core ids for "
                        f"OsCoreBswApplicationRef='{bsw_app_name}' / OsCoreEcucIdRef='{ecuc_core}' "
                        f"({type(e).__name__}: {e}). "
                        f"Check OsApplication/OsAppEcucPartitionRef and EcucCoreDefinition linkage."
                    ),
                    path,
                    "fail",
                    [],
                    [
                        f"Fix refs on {path}: BswApp='{bsw_app_name}', EcucCore='{ecuc_core}' must resolve to numeric core ids."
                    ],
                )
                continue

            ok = (bsw_app_core_id == ecuc_core_id)
            self.__class__.validation_result = ok
            if ok:
                self.vlog(
                    self.validation_code,
                    self.validation_summary,
                    (
                        f"OsCore '{os_key}': comparison_key=(BswAppCoreId, EcucCoreId)=({bsw_app_core_id}, {ecuc_core_id}); "
                        f"OsCoreBswApplicationRef='{bsw_app_name}', OsCoreEcucIdRef='{ecuc_core}': match."
                    ),
                    path,
                    "pass",
                )
            else:
                self.vlog(
                    self.validation_code,
                    self.validation_summary,
                    (
                        f"OsCore '{os_key}': comparison_key=(BswAppCoreId, EcucCoreId): "
                        f"OsCoreBswApplicationRef='{bsw_app_name}' → core {bsw_app_core_id}, "
                        f"OsCoreEcucIdRef='{ecuc_core}' → core {ecuc_core_id}. They must be equal. "
                        f"Fix: point OsCoreBswApplicationRef at the OsApplication whose partition/core "
                        f"matches EcucCoreDefinition '{ecuc_core}', or correct OsCoreEcucIdRef."
                    ),
                    path,
                    "fail",
                    [],
                    [
                        f"On {path}: make BswApp '{bsw_app_name}' (core {bsw_app_core_id}) and "
                        f"EcucCore '{ecuc_core}' (core {ecuc_core_id}) refer to the same core."
                    ],
                )
