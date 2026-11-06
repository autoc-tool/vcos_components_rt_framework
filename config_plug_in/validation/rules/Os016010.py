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

class Os016010Rule(BasicRule):
    """检验App配置的identifier是否正确"""
    ruleset_name = "Os016010"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "The app indentifier must be configured currectly"
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
        mpu_active = False
        validation_data = {}
        param_path = ''

        # 非SC3或者SC4,则不需要检查
        for os_key, os_instance in ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            if "/Os/OsOS" in os_instance["DefinitionRef"] and "OsScalabilityClass" in os_instance:
                if os_instance["OsScalabilityClass"].get("Value","") in ("SC3","SC4"):
                    mpu_active = True

        if mpu_active:
            app_dict = {}
            core_dict = {}
            bsw_app = list()
            for os_key, os_instance in ecuc_os_dict.items():
                if not isinstance(os_instance, dict):
                    continue
                if "/Os/OsApplication" in os_instance["DefinitionRef"]:
                    app_dict[os_key] = os_instance
                if "/Os/OsCore" in os_instance["DefinitionRef"]:
                    core_dict[os_key] = os_instance
            for name, value in core_dict.items():
                bsw_app.append(OsRuleutils().get_last_item_verify(value.get("OsCoreBswApplicationRef",{}).get("ValueRef","")))
            for name, value in app_dict.items():
                param_path = "/ActiveEcuC/Os/" + name
                app_identifier = value.get("OsAppMemoryProtectionIdentifier",{}).get("Value","")
                validation_data.update({"app_name": ''})
                if app_identifier == None or app_identifier == '':
                    validation_data.update({"app_name": name})
                    validation_data.update({"error_detail": "app must have identifier"})
                else:
                    if name in bsw_app and app_identifier != 0:
                        validation_data.update({"app_name": name})
                        validation_data.update({"error_detail": "bsw app's identifier must be 0"})
                    elif "TriCore397" in ecuc_os_dict:
                        if app_identifier > 5:
                            validation_data.update({"app_name": name})
                            validation_data.update({"error_detail": "the range of identifier is [0, 5]"})
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
