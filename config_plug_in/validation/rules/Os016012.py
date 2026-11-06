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

class Os016012Rule(BasicRule):
    """检验MPU Region数量是否超过范围"""
    ruleset_name = "Os016012"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "The number of mpu regions has a limited range"
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
        ecuc_os_dict = self.ecuc_os_dict
        region_list = {}
        app_dict = {}
        task_list = list()
        isr_list = list()
        app_cnt_dict = {}
        core_dict = {}
        set0_dict = {}
        validation_data = {}
        param_path = ''

        for os_key, os_instance in ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            if "/Os/OsOS" in os_instance["DefinitionRef"] and os_instance["OsScalabilityClass"].get("Value","") not in ("SC3","SC4"):
                return
            elif "/Os/OsMemoryProtection" in os_instance["DefinitionRef"]:
                region_list = os_instance
            elif "/Os/OsApplication" in os_instance["DefinitionRef"]:
                app_dict[os_key] = os_instance

        for app_name, app_value in app_dict.items():
            coreid = app_value.get("OsApplicationCoreRef",{}).get("ValueRef","")[-1]
            task_list = OsRuleutils().get_ref_list(app_value.get("OsAppTaskRef",{}))
            isr_list = OsRuleutils().get_ref_list(app_value.get("OsAppIsrRef",{}))
            core_dict[coreid] = 1
            if coreid not in set0_dict:
                set0_dict[coreid] = 0
            app_cnt_dict[app_name] = {}
            for region_name, region_value in region_list.items():
                if not isinstance(region_value, dict):
                    continue
                owner = OsRuleutils().get_last_item_verify(region_value.get("OsMemoryRegionOwner",{}).get("ValueRef",""))
                core = OsRuleutils().get_last_item_verify(region_value.get("OsMemoryRegionProtectionUnit",{}).get("ValueRef",""))
                access_rights = OsRuleutils().get_last_item_verify(region_value.get("OsMemoryRegionAccessRights",{}).get("ValueRef",""))
                if "TriCore397" not in ecuc_os_dict or '_R' in access_rights:
                    if "Core" + coreid in core and (owner in task_list or owner in isr_list):
                        if owner in app_cnt_dict[app_name]:
                            app_cnt_dict[app_name][owner] += 1
                        else:
                            app_cnt_dict[app_name][owner] = 1
                    elif "Core" + coreid in core and owner == app_name:
                        if app_value.get("OsAppMemoryProtectionIdentifier",{}).get("Value","") == 0:
                            set0_dict[coreid] += 1
                        else:
                            if owner in app_cnt_dict[app_name]:
                                app_cnt_dict[app_name][owner] += 1
                            else:
                                app_cnt_dict[app_name][owner] = 1
                    elif "Core" + coreid in core and owner == '':
                        core_dict[coreid] += 1

        for app_name, app_value in app_dict.items():
            coreid = app_value.get("OsApplicationCoreRef",{}).get("ValueRef","")[-1]
            validation_data.update({"app_name": ''})
            max_value = 0
            if app_name in app_cnt_dict:
                for owner_name, owner_value in app_cnt_dict[app_name].items():
                    if owner_name != app_name and owner_value > max_value:
                        max_value = owner_value
                if app_name in app_cnt_dict[app_name]:
                    max_value = app_cnt_dict[app_name][app_name] + max_value + core_dict[coreid] + set0_dict[coreid]
                else:
                    max_value = max_value + core_dict[coreid] + set0_dict[coreid]
            else:
                max_value = core_dict[coreid] + set0_dict[coreid]
            if "TriCore397" in ecuc_os_dict and max_value > 18:
                validation_data.update({"app_name": app_name})
                validation_data.update({"max_value": max_value})
                break
            elif "Armv8rE3650" in ecuc_os_dict and max_value > 24:
                validation_data.update({"app_name": app_name})
                validation_data.update({"max_value": max_value})
                break

        try:
            self.__class__.validation_result = False
            post(self.__class__.ruleset_name, validation_data)
        except Exception as e:
            pass
        # 格式："错误码", "摘要", "错误信息", "错误索引路径" "结果"
        if self.__class__.validation_result:
            self.vlog(self.validation_code, self.validation_summary,
                f"validation_data:{validation_data}", param_path, "pass")
        else:
            self.vlog(self.validation_code, self.validation_summary,
                f"validation_data:{validation_data}", param_path, "fail")
