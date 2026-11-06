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

class Os013008Rule(BasicRule):
    """中断类型和中断优先级不匹配则进行告警提示"""
    ruleset_name = "Os013008"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_cfg = (
            validation_data.sys_info_class.system_target_info.get("validate", {})
            .get("ignore_option", {})
            .get("Os", {})
        )
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "OsIsrCategory and OsIsrInterruptPriority not match."
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
            if "/Os/OsIsr" in os_instance["DefinitionRef"]:
                os_isr_name = os_key
                isr_category = OsRuleutils().get_module_property_value("OsIsr", os_isr_name,
                                                                        "OsIsrCategory", self.ecuc_os_dict)
                isr_priority = OsRuleutils().get_module_property_value("OsIsr", os_isr_name,
                                                                     'OsIsrInterruptPriority', self.ecuc_os_dict)

                check_passed = False
                if self.validation_cfg['max_interrupt_priority'] > self.validation_cfg['min_interrupt_priority']:
                    priority_offset = 1
                else:
                    priority_offset = -1
                if isr_category == 'CATEGORY_0':
                    limit_a = self.validation_cfg['max_cat0_interrupt_priority']
                    limit_b = self.validation_cfg['max_tp_interrupt_priority'] + priority_offset
                elif isr_category == 'CATEGORY_TP':
                    limit_a = self.validation_cfg['max_tp_interrupt_priority']
                    limit_b = self.validation_cfg['max_cat1_interrupt_priority'] + priority_offset
                elif isr_category == 'CATEGORY_1':
                    limit_a = self.validation_cfg['max_cat1_interrupt_priority']
                    limit_b = self.validation_cfg['max_cat2_interrupt_priority'] + priority_offset
                elif isr_category == 'CATEGORY_2':
                    limit_a = self.validation_cfg['max_cat2_interrupt_priority']
                    limit_b = self.validation_cfg['min_interrupt_priority']
                else:
                    limit_a, limit_b = -1, -1

                if limit_a >= 0 and limit_b >= 0:
                    upper_limit = max(limit_a, limit_b)
                    lower_limit = min(limit_a, limit_b)
                    if isr_priority >= lower_limit and isr_priority <= upper_limit:
                        check_passed = True
                param_path = "/ActiveEcuC/Os/" + os_isr_name + "/OsIsrInterruptPriority"
                try:
                    self.__class__.validation_result = check_passed
                    validation_data = {}
                    if not self.__class__.validation_result:
                        validation_data = {
                            'isr_name': os_isr_name,
                            'isr_category': isr_category,
                            'isr_priority': isr_priority,
                            'acceptable_upper_limit': limit_a,
                            'acceptable_lower_limit': limit_b
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
