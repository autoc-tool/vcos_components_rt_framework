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
from collections import Counter


class Os011003Rule(BasicRule):
    """
    重复校验:OsIsrInterruptSource中断号定义不能重复
    """
    ruleset_name = "Os011003"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.ecuc_dict = validation_data.bsw_ecuc_dict['EcuC']['ArPackages']['ActiveEcuC']['EcuC']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "No repeating of OsIsrInterruptSource for each Core"
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
            # print('get_ruleset() exception message: {0}'.format(e.args))
            with ruleset(self.__class__.ruleset_name):

                @when_all(+m.coreid)
                def fact(c):
                    self.__class__.validation_result = True

    @timing_decorator
    def validation_run(self):
        """011表示:重复配置校验,003表示:os模块子校验码(001~999)"""
        # 1）准备校验数据并依次校验
        coreid_dic = {}
        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            # 遍历application根据coreid分组中断
            if "/Os/OsApplication" in os_instance["DefinitionRef"]:
                coreid_str = ""
                if "OsApplicationCoreRef" in os_instance and "/ActiveEcuC/EcuC/EcucHardware/" in os_instance[
                  "OsApplicationCoreRef"].get("ValueRef", "") and "OsAppIsrRef" in os_instance and isinstance(
                      os_instance["OsAppIsrRef"].get("ValueRef", ""), list):
                    coreid_str = str(OsRuleutils().get_core_number(os_key, self.ecuc_os_dict, self.ecuc_dict))
                    for i in os_instance["OsAppIsrRef"]["ValueRef"]:
                        if coreid_str in coreid_dic:
                            coreid_dic[coreid_str].append(OsRuleutils().get_last_item_verify(i))
                        else:
                            coreid_dic.update({coreid_str: [OsRuleutils().get_last_item_verify(i)]})
                elif "OsApplicationCoreRef" in os_instance and "/ActiveEcuC/EcuC/EcucHardware/" in os_instance[
                  "OsApplicationCoreRef"].get("ValueRef", "") and "OsAppIsrRef" in os_instance and isinstance(
                      os_instance["OsAppIsrRef"].get("ValueRef", ""), str):
                    coreid_str = str(OsRuleutils().get_core_number(os_key, self.ecuc_os_dict, self.ecuc_dict))
                    if coreid_str in coreid_dic:
                        coreid_dic[coreid_str].append(OsRuleutils().get_last_item_verify(os_instance["OsAppIsrRef"].get(
                          "ValueRef", "")))
                    else:
                        coreid_dic.update({
                          coreid_str: [
                            OsRuleutils().get_last_item_verify(os_instance["OsAppIsrRef"].get("ValueRef", ""))
                          ]
                        })
        #遍历isr各核获取同核OsIsrInterruptSource
        for coreid, isr_list_value in coreid_dic.items():
            OsIsrInterruptSource_coreid = []
            isr_name_map = {}
            # 只遍历本core的ISR名称
            for isr_name in isr_list_value:
                os_instance = self.ecuc_os_dict.get(isr_name, {})
                if not isinstance(os_instance, dict):
                    continue
                if "/Os/OsIsr" in os_instance.get("DefinitionRef", ""):
                    isr_id = os_instance.get("OsIsrInterruptSource", {}).get("Value", None)
                    if isr_id is not None:
                        OsIsrInterruptSource_coreid.append(isr_id)
                        if isr_id in isr_name_map:
                            isr_name_map[isr_id].append(isr_name)
                        else:
                            isr_name_map[isr_id] = [isr_name]
            validation_data = {}
            param_path = "/ActiveEcuC/Os/Core" + coreid
            validation_data_list = []
            validation_data.update({"OsIsrInterruptSourcelist": OsIsrInterruptSource_coreid})
            validation_data.update({"coreid": coreid})
            validation_data_list.append(validation_data)
            try:
                self.__class__.validation_result = False
                # 只校验本core内是否有重复ID
                if len(OsIsrInterruptSource_coreid) == len(set(OsIsrInterruptSource_coreid)):
                    assert_fact(self.__class__.ruleset_name, validation_data)
                else:
                    self.__class__.validation_result = False
            except Exception as e:
                pass
            # 格式："错误码", "摘要", "错误信息", "错误索引路径" "结果"
            if self.__class__.validation_result:
                self.vlog(self.validation_code, self.validation_summary, f"validation_data:{validation_data}",
                          param_path, "pass")
            else:
                counter = Counter(OsIsrInterruptSource_coreid)
                duplicates = [isr_id for isr_id, count in counter.items() if count > 1]
                duplicate_isr_info = []
                for isr_id in duplicates:
                    for isr_name in isr_name_map[isr_id]:
                        duplicate_isr_info.append(f"{isr_name}:{isr_id}")
                duplicate_info_str = ", ".join(duplicate_isr_info)
                self.vlog(self.validation_code, self.validation_summary, f"Repeated ISR: {duplicate_info_str}",
                          param_path, "fail")
            # 2）清空ruleset中的所有fact，为下一次校验做准备
            if validation_data_list:
                retract_facts(self.__class__.ruleset_name, validation_data_list)
            pass
