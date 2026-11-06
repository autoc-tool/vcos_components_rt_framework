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


class Os014006Rule(BasicRule):
    """一个Task属于一个Application,同一个Task不能被多个Applicaton引用 """
    ruleset_name = "Os014006"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "A Task must belong to an Application, and the same Task cannot be referenced by multiple Applications."
        self.durable_init()

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            with ruleset(self.__class__.ruleset_name):

                @when_all(+m.task_application_list)
                def action(c):
                    self.__class__.validation_result = True

            pass

    @timing_decorator
    def validation_run(self):
        """014表示:附加约束,006表示:os模块子校验码(001~999)"""
        task_application_list = []
        task_list = []
        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            # 遍历获取OsAppTaskRef list和OsTask list
            if "DefinitionRef" in os_instance and "/Os/OsApplication" in os_instance[
              "DefinitionRef"] and "OsAppTaskRef" in os_instance:
                if isinstance(os_instance["OsAppTaskRef"].get("ValueRef", ""), list):
                    for i in os_instance["OsAppTaskRef"]["ValueRef"]:
                        task_application_list.append(OsRuleutils().get_last_item_verify(i))
                if isinstance(os_instance["OsAppTaskRef"].get("ValueRef", ""),
                              str) and os_instance["OsAppTaskRef"]["ValueRef"] != "<empty>":
                    task_application_list.append(OsRuleutils().get_last_item_verify(
                      os_instance["OsAppTaskRef"]["ValueRef"]))
        for os_key, os_instance in self.ecuc_os_dict.items():
            if "DefinitionRef" in os_instance and "/Os/OsTask" in os_instance["DefinitionRef"]:
                task_list.append(os_key)
                param_path = "/ActiveEcuC/Os/" + os_key

                try:
                    # 检查task是否在task_application_list中出现且仅出现一次
                    occurrence_count = task_application_list.count(os_key)
                    self.__class__.validation_result = (occurrence_count == 1)

                    if self.__class__.validation_result:
                        validation_data = {
                            "task_application_list": task_application_list,
                            "task_list": task_list
                        }
                        post(self.__class__.ruleset_name, validation_data)
                except Exception as e:
                    self.__class__.validation_result = False

                self.vlog(
                    self.validation_code,
                    self.validation_summary,
                    f"validation_data:{param_path}",
                    param_path,
                    "pass" if self.__class__.validation_result else "fail"
                )
