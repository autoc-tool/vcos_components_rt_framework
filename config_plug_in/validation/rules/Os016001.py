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

class Os016001Rule(BasicRule):
    """OsSpinlockAccessingApplication属于同核则进行告警提示"""
    ruleset_name = "Os016001"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.ecuc_dict = validation_data.bsw_ecuc_dict['EcuC']['ArPackages']['ActiveEcuC']['EcuC']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "Spinlocks are used to co-ordinate concurrent access by Tasks/ISRs on different cores to a shared resource."
        self.durable_init()

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            pass

    @timing_decorator
    def validation_run(self):
        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            if "/Os/OsSpinlock" in os_instance["DefinitionRef"]:
                OsSpinlockName = os_key
                spinlock_app_list = OsRuleutils().get_module_property_valueref("OsSpinlock", OsSpinlockName,
                                                                               "OsSpinlockAccessingApplication",
                                                                               self.ecuc_os_dict)
                spinlock_coreid_dict = {}
                for app in spinlock_app_list:
                    coreid = OsRuleutils().get_core_number(app, self.ecuc_os_dict, self.ecuc_dict)
                    spinlock_coreid_dict[coreid] = spinlock_coreid_dict.get(coreid, 0) + 1

                param_path = "/ActiveEcuC/Os/" + OsSpinlockName + "/OsSpinlockAccessingApplication"
                try:
                    self.__class__.validation_result = True
                    validation_data = {}
                    if len(spinlock_coreid_dict.keys()) == 1:
                        spinlock_key = list(spinlock_coreid_dict.keys())[0]
                        if spinlock_coreid_dict[spinlock_key] > 1:
                            self.__class__.validation_result = False
                    if not self.__class__.validation_result:
                        validation_data = {
                            'spinlock_name': OsSpinlockName,
                            'spinlock_coreid_dict': spinlock_coreid_dict
                        }
                        post(self.__class__.ruleset_name, validation_data)

                except Exception as e:
                    pass
                # 格式："错误码", "摘要", "错误信息", "错误索引路径" "结果"
                self.vlog(
                    self.validation_code,
                    self.validation_summary,
                    f"validation_data:{validation_data}",
                    param_path,
                    "pass" if self.__class__.validation_result else "fail"
                )
