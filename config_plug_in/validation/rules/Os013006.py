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


class Os013006Rule(BasicRule):
    """ref同一个resource的task & application不能跨核"""
    ruleset_name = "Os013006"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.ecuc_dict = validation_data.bsw_ecuc_dict['EcuC']['ArPackages']['ActiveEcuC']['EcuC']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "Tasks/Applications that reference the same resource cannot be cross-core"
        self.durable_init()

    def check_validation_run(self,bsw_ecuc_cfg_class_dict):
        if not self.modules_are_exist(["EcuC"],bsw_ecuc_cfg_class_dict):
            self.vlog(self.validation_code, self.validation_summary, "Data required for validation is missing",
                self.validation_summary, "fail")
            return False
        return True

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            with ruleset(self.__class__.ruleset_name):

                @when_all(+m.resource_coreid)
                def action(c):
                    self.__class__.validation_result = True

    @timing_decorator
    def validation_run(self):
        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            # 遍历Resource
            if "/Os/OsResource" in os_instance["DefinitionRef"]:
                resource_name = os_key
                resource_core_set = set()
                resource_app_set = set()
                task_list = OsRuleutils().get_tasklist_from_resource(resource_name, self.ecuc_os_dict)
                for task_name in task_list:
                    app_name = OsRuleutils().get_app_from_task(task_name, self.ecuc_os_dict)
                    if app_name:
                        resource_app_set.add(app_name)
                        coreid = OsRuleutils().get_core_number(app_name, self.ecuc_os_dict, self.ecuc_dict)
                        if coreid is not None:
                            resource_core_set.add(coreid)
                validation_data = {}
                param_path = "/ActiveEcuC/Os/" + os_key
                validation_data.update({"resource": os_key})
                validation_data.update({"resource_coreid": list(resource_core_set)})
                validation_data.update({"resource_app_list": list(resource_app_set)})
                try:
                    self.__class__.validation_result = False
                    if len(resource_core_set) < 2:
                        post(self.__class__.ruleset_name, validation_data)
                    else:
                        self.__class__.validation_result = False
                except Exception as e:
                    pass
                # 格式："错误码", "摘要", "错误信息", "错误索引路径" "结果"
                if self.__class__.validation_result:
                    self.vlog(self.validation_code, self.validation_summary,
                              f"validation_data:{validation_data}", param_path, "pass")
                else:
                    self.vlog(self.validation_code, self.validation_summary,
                              f"validation_data:{validation_data}", param_path, "fail")
        return self.__class__.validation_result
