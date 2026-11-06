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

class Os003001Rule(BasicRule):
    """Application/Task/Counter/Event/Resource/Isr name长度范围[1,127]"""
    ruleset_name = "Os003001"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "Length of Application/Task/Counter/Event/Resource/Isr name is [1,127]"
        self.durable_init()

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            with ruleset(self.__class__.ruleset_name):

                @when_all(+m.application)
                def action(c):
                    self.__class__.validation_result = True
            pass

    @timing_decorator
    def validation_run(self):
        app_name_dict = list(OsRuleutils().get_module_cfg("OsApplication",self.ecuc_os_dict).keys())
        task_name_dict = list(OsRuleutils().get_module_cfg("OsTask",self.ecuc_os_dict).keys())
        counter_name_dict = list(OsRuleutils().get_module_cfg("OsCounter",self.ecuc_os_dict).keys())
        event_name_dict = list(OsRuleutils().get_module_cfg("OsEvent",self.ecuc_os_dict).keys())
        resource_name_dict = list(OsRuleutils().get_module_cfg("OsResource",self.ecuc_os_dict).keys())
        isr_name_dict = list(OsRuleutils().get_module_cfg("OsIsr",self.ecuc_os_dict).keys())
        validation_data = {}
        param_path = "/ActiveEcuC/Os/"
        validation_data.update({"application": app_name_dict})
        validation_data.update({"task": task_name_dict})
        validation_data.update({"counter": counter_name_dict})
        validation_data.update({"event": event_name_dict})
        validation_data.update({"resource": resource_name_dict})
        validation_data.update({"isr": isr_name_dict})
        invalid_names = {}
        try:
            check_result = True
            for key, value in validation_data.items():
                if not isinstance(value, list):
                    check_result = False
                    invalid_names[key] = ["Not a list"]
                    continue
                for i in value:
                    if not isinstance(i, str) or not (0 < len(i) < 128):
                        check_result = False
                        if key not in invalid_names:
                            invalid_names[key] = []
                        invalid_names[key].append(i)
            self.__class__.validation_result = check_result
        except Exception as e:
            invalid_names = {"Exception": str(e)}
            self.__class__.validation_result = False

        # 格式："错误码", "摘要", "错误信息", "错误索引路径" "结果"
        if self.__class__.validation_result:
            self.vlog(self.validation_code, self.validation_summary,
                        f"validation_data:{validation_data}", param_path, "pass")
        else:
            self.vlog(self.validation_code, self.validation_summary,
                        f"invalid_names:{invalid_names}", param_path, "fail")
