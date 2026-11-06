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


class Os013010Rule(BasicRule):
    """一个调试表下的OsScheduleTblExpPointOffset不能相同"""
    ruleset_name = "Os013010"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "The OsScheduleTblExpPointOffset value cannot be the same under the same OsScheduleTableExpiryPoint "
        self.durable_init()

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            with ruleset(self.__class__.ruleset_name):

                @when_all(+m.OsScheduleTable)
                def action(c):
                    self.__class__.validation_result = True

            pass

    @timing_decorator
    def validation_run(self):
        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            if "/Os/OsScheduleTable" in os_instance["DefinitionRef"]:
                OsScheduleTblExpPointOffset_list = []
                for osscheduletable_k, osscheduletable_v in os_instance.items():
                    if not isinstance(osscheduletable_v, dict):
                        continue
                    if "Os/OsScheduleTable/OsScheduleTableExpiryPoint" in osscheduletable_v["DefinitionRef"]:
                        OsScheduleTblExpPointOffset = osscheduletable_v["OsScheduleTblExpPointOffset"].get("Value", "")
                        if OsScheduleTblExpPointOffset:
                            OsScheduleTblExpPointOffset_list.append(OsScheduleTblExpPointOffset)
                validation_data = {}
                param_path = "/ActiveEcuC/Os/" + os_key
                validation_data.update({"OsScheduleTable": os_key})
                validation_data.update({"OsScheduleTblExpPointOffsetList": OsScheduleTblExpPointOffset_list})
                try:
                    self.__class__.validation_result = False
                    if len(OsScheduleTblExpPointOffset_list) == len(set(OsScheduleTblExpPointOffset_list)):
                        post(self.__class__.ruleset_name, validation_data)
                except Exception as e:
                    pass
                # 格式："错误码", "摘要", "错误信息", "错误索引路径" "结果"
                if self.__class__.validation_result:
                    self.vlog(self.validation_code, self.validation_summary, f"validation_data:{validation_data}",
                              param_path, "pass")
                else:
                    self.vlog(self.validation_code, self.validation_summary, f"validation_data:{validation_data}",
                              param_path, "fail")
