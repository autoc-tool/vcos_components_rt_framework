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


class Os014007Rule(BasicRule):
    """Application属性OsAppEcucPartitionRef为空时,warning告警 """
    ruleset_name = "Os014007"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "OsAppEcucPartitionRef can not be null"
        self.durable_init()

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            with ruleset(self.__class__.ruleset_name):

                @when_all(+m.OsAppEcucPartitionRef)
                def action(c):
                    self.__class__.validation_result = True

            pass

    @timing_decorator
    def validation_run(self):
        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            # OsAppEcucPartitionRef
            if "DefinitionRef" in os_instance and "/Os/OsApplication" in os_instance[
              "DefinitionRef"] and "OsAppEcucPartitionRef" in os_instance:
                OsAppEcucPartitionRef = OsRuleutils().get_last_item_verify(os_instance["OsAppEcucPartitionRef"].get(
                  "ValueRef", ""))
                param_path = "/ActiveEcuC/Os/" + os_key
                validation_data = {}
                try:
                    self.__class__.validation_result = (OsAppEcucPartitionRef and OsAppEcucPartitionRef != "<empty>")

                    if self.__class__.validation_result:
                        validation_data = {
                            "OsAppEcucPartitionRef": OsAppEcucPartitionRef
                        }
                        post(self.__class__.ruleset_name, validation_data)
                except Exception as e:
                    self.__class__.validation_result = False

                # 格式："错误码", "摘要", "错误信息", "错误索引路径" "结果"
                self.vlog(self.validation_code, self.validation_summary, f"validation_data:{param_path}",
                          param_path, "pass" if self.__class__.validation_result else "fail")
