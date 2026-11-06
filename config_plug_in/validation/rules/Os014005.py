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


class Os014005Rule(BasicRule):
    """一个ScheduleTable属于一个Application,同一个ScheduleTable不能被多个Applicaton引用 """
    ruleset_name = "Os014005"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "A ScheduleTable must belong to an Application, and the same ScheduleTable cannot be referenced by multiple Applications"
        self.durable_init()

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            with ruleset(self.__class__.ruleset_name):

                @when_all(+m.scheduletable_application_list)
                def action(c):
                    self.__class__.validation_result = True

            pass

    @timing_decorator
    def validation_run(self):
        """014表示:附加约束,005表示:os模块子校验码(001~999)"""
        scheduletable_application_list = []
        scheduletable_list = []
        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            # 遍历获取OsAppScheduleTableRef list和OsScheduleTable list
            if "DefinitionRef" in os_instance and "/Os/OsApplication" in os_instance[
              "DefinitionRef"] and "OsAppScheduleTableRef" in os_instance:
                if isinstance(os_instance["OsAppScheduleTableRef"].get("ValueRef", ""), list):
                    for i in os_instance["OsAppScheduleTableRef"]["ValueRef"]:
                        scheduletable_application_list.append(OsRuleutils().get_last_item_verify(i))
                if isinstance(os_instance["OsAppScheduleTableRef"].get("ValueRef", ""),
                              str) and os_instance["OsAppScheduleTableRef"]["ValueRef"] != "<empty>":
                    scheduletable_application_list.append(OsRuleutils().get_last_item_verify(
                      os_instance["OsAppScheduleTableRef"]["ValueRef"]))
        for os_key, os_instance in self.ecuc_os_dict.items():
            if "DefinitionRef" in os_instance and "/Os/OsScheduleTable" in os_instance["DefinitionRef"]:
                scheduletable_list.append(os_key)
                # check OsScheduleTable 在scheduletable_application_list中出现次数<=1次
                param_path = "/ActiveEcuC/Os/" + os_key
                validation_data = {}
                validation_data.update({"scheduletable_application_list": scheduletable_application_list})
                validation_data.update({"scheduletable_list": scheduletable_list})
                try:
                    # 检查scheduletable是否在application列表中出现且仅出现一次
                    occurrence_count = scheduletable_application_list.count(os_key)
                    self.__class__.validation_result = (occurrence_count == 1)

                    if self.__class__.validation_result:
                        validation_data = {
                            "scheduletable_application_list": scheduletable_application_list,
                            "scheduletable_list": scheduletable_list
                        }
                        post(self.__class__.ruleset_name, validation_data)
                except Exception as e:
                    self.__class__.validation_result = False

                # 格式："错误码", "摘要", "错误信息", "错误索引路径" "结果"
                if self.__class__.validation_result:
                    self.vlog(self.validation_code, self.validation_summary, f"validation_data:{param_path}",
                              param_path, "pass")
                else:
                    self.vlog(self.validation_code, self.validation_summary, f"validation_data:{param_path}",
                              param_path, "fail")
