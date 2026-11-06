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

class Os016003Rule(BasicRule):
    """调度表启动方式为相对启动OsScheduleTableStartValue="RELATIVE",
    调度表初试偏移值OsScheduleTableStartValue < counter的OsCounterMaxAllowedValue"""
    ruleset_name = "Os016003"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "If OsScheduleTableAutostartType=RELATIVE, OsScheduleTableStartValue must be less than OsCounter's OsCounterMaxAllowedValue"
        self.durable_init()

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            with ruleset(self.__class__.ruleset_name):
                @when_all(m.OsScheduleTableStartValue < m.OsCounterMaxAllowedValue)
                def action(c):
                    self.__class__.validation_result = True

    @timing_decorator
    def validation_run(self):
        ecuc_os_dict = self.ecuc_os_dict
        for os_key, os_instance in ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            # 遍历OsScheduleTable获取数据
            if "/Os/OsScheduleTable" in os_instance["DefinitionRef"]:
                for scheduletable_name, scheduletable_value in os_instance.items():
                    if "DefinitionRef" in scheduletable_value and "Os/OsScheduleTable/OsScheduleTableAutostart" in scheduletable_value[
                      "DefinitionRef"]:
                        OsCounterMaxAllowedValue,OsScheduleTableStartValue = 0,0
                        OsScheduleTableAutostartType = scheduletable_value["OsScheduleTableAutostartType"].get("Value","")
                        if OsScheduleTableAutostartType == "RELATIVE":
                            ScheduleTable_counter = OsRuleutils().get_module_property_valueref("OsScheduleTable", os_key, "OsScheduleTableCounterRef", ecuc_os_dict)
                            if ScheduleTable_counter:
                                OsCounterMaxAllowedValue = OsRuleutils().get_counter_OsCounterMaxAllowedValue(ScheduleTable_counter[0], ecuc_os_dict)
                            OsScheduleTableStartValue = scheduletable_value["OsScheduleTableStartValue"].get("Value",0)
                            OsScheduleTable = os_key
                            validation_data = {}
                            param_path = "/ActiveEcuC/Os/" + os_key + "/" + scheduletable_name + "/OsScheduleTableStartValue"
                            validation_data.update({"OsScheduleTable": OsScheduleTable})
                            validation_data.update({"OsCounterMaxAllowedValue": OsCounterMaxAllowedValue})
                            validation_data.update({"OsScheduleTableStartValue": OsScheduleTableStartValue})
                            try:
                                self.__class__.validation_result = False
                                # if OsScheduleTableStartValue < OsCounterMaxAllowedValue:
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
