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


class Os014008Rule(BasicRule):
    """每个核上OsTask实例个数要求>=1(Idle Task)"""
    ruleset_name = "Os014008"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.ecuc_dict = validation_data.bsw_ecuc_dict['EcuC']['ArPackages']['ActiveEcuC']['EcuC']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "Number of OsTask instances on each core >=1(Idle Task)"
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

                @when_all(+m.task_coreid)
                def action(c):
                    self.__class__.validation_result = True

            pass

    @timing_decorator
    def validation_run(self):
        coreid_dic = {}
        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            # 遍历application根据coreid分组中断
            if "/Os/OsApplication" in os_instance["DefinitionRef"]:
                coreid_str = ""
                if "OsApplicationCoreRef" in os_instance and "/ActiveEcuC/EcuC/EcucHardware/" in os_instance[
                  "OsApplicationCoreRef"].get("ValueRef", "") and "OsAppTaskRef" in os_instance and isinstance(
                      os_instance["OsAppTaskRef"].get("ValueRef", ""), list) and os_instance["OsAppTaskRef"].get("ValueRef", ""):
                    coreid_str = str(OsRuleutils().get_core_number(os_key, self.ecuc_os_dict, self.ecuc_dict))
                    for i in os_instance["OsAppTaskRef"].get("ValueRef", ""):
                        if coreid_str in coreid_dic:
                            coreid_dic[coreid_str].append(OsRuleutils().get_last_item_verify(i))
                        else:
                            coreid_dic.update({coreid_str: [OsRuleutils().get_last_item_verify(i)]})
                elif "OsApplicationCoreRef" in os_instance and "/ActiveEcuC/EcuC/EcucHardware/" in os_instance[
                  "OsApplicationCoreRef"].get("ValueRef", "") and "OsAppTaskRef" in os_instance and isinstance(
                      os_instance["OsAppTaskRef"].get("ValueRef", ""), str) and os_instance["OsAppTaskRef"].get("ValueRef", ""):
                    coreid_str = str(OsRuleutils().get_core_number(os_key, self.ecuc_os_dict, self.ecuc_dict))
                    if coreid_str in coreid_dic:
                        coreid_dic[coreid_str].append(OsRuleutils().get_last_item_verify(
                          os_instance["OsAppTaskRef"].get("ValueRef", "")))
                    else:
                        coreid_dic.update({
                          coreid_str: [
                            OsRuleutils().get_last_item_verify(os_instance["OsAppTaskRef"].get("ValueRef", ""))
                          ]
                        })
        #遍历task各核获取同核OsTask列表
        for coreid, task_list_value in coreid_dic.items():
            validation_data = {}
            param_path = "/ActiveEcuC/Os/Core" + coreid
            validation_data_list = []
            validation_data.update({"task_coreid": task_list_value})
            validation_data.update({"coreid": coreid})
            validation_data_list.append(validation_data)
            try:
                self.__class__.validation_result = False

                unique_tasks = set(validation_data['task_coreid'])
                self.__class__.validation_result = (len(unique_tasks) >= 1)

                if self.__class__.validation_result:
                    validation_data = {
                        "task_coreid": list(unique_tasks),
                        "coreid": coreid
                    }
                    assert_fact(self.__class__.ruleset_name, validation_data)
            except Exception as e:
                pass
            # 格式："错误码", "摘要", "错误信息", "错误索引路径" "结果"
            if self.__class__.validation_result:
                self.vlog(self.validation_code, self.validation_summary, f"validation_data:Core{coreid}", param_path,
                          "pass")
            else:
                self.vlog(self.validation_code, self.validation_summary, f"validation_data:Core{coreid}", param_path,
                          "fail")
            # 2）清空ruleset中的所有fact，为下一次校验做准备
            if validation_data_list:
                retract_facts(self.__class__.ruleset_name, validation_data_list)
            pass
