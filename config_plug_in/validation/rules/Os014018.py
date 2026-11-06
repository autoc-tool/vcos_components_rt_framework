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


class Os014018Rule(BasicRule):
    """task访问的applcation数量需要>=1, 至少包含owner application"""
    ruleset_name = "Os014018"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "The number of applications accessed by the task must be >=1, including at least the task's owner application"
        self.durable_init()

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            with ruleset(self.__class__.ruleset_name):

                @when_all(+m.OsTaskName)
                def action(c):
                    self.__class__.validation_result = True

            pass

    @timing_decorator
    def validation_run(self):
        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            # 遍历OsTask获取数据
            if "/Os/OsTask" in os_instance["DefinitionRef"] and "OsTaskAccessingApplication" in os_instance:
                OsTaskAccessingApplication = OsRuleutils().get_module_property_valueref(
                  "OsTask", os_key, "OsTaskAccessingApplication", self.ecuc_os_dict)
                validation_data = {}
                validation_data.update({"OsTaskName": os_key})
                validation_data.update({"OsTaskAccessingApplicationList": OsTaskAccessingApplication})
                param_path = "/ActiveEcuC/Os/" + os_key
                try:
                    if len(OsTaskAccessingApplication) >= 1:
                        post(self.__class__.ruleset_name, validation_data)
                    else:
                        self.__class__.validation_result = False
                except Exception as e:
                    pass
                # 格式："错误码", "摘要", "错误信息", "错误索引路径" "结果"
                if self.__class__.validation_result:
                    self.vlog(self.validation_code, self.validation_summary, f"validation_data:{validation_data}",
                              param_path, "pass")
                else:
                    self.vlog(self.validation_code, self.validation_summary, f"validation_data:{validation_data}",
                              param_path, "fail")
