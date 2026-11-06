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
import os

class Os016002Rule(BasicRule):
    """获取所有的Ioc支持的datatype类型列表校验, datatype包括Os_Types.h中自定义类型和标准类型basic_datatype"""
    ruleset_name = "Os016002"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.system_path = validation_data.sys_info_class.system_path
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "Ioc datatypes should include basic datatypes and user defined from Os_types.h."
        self.durable_init()

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            with ruleset(self.__class__.ruleset_name):
                @when_all(+m.OsIoc)
                def action(c):
                    self.__class__.validation_result = True

            pass

    @timing_decorator
    def validation_run(self):
        os_types_path = os.path.abspath(os.path.join(self.system_path, "../../include/autosar/Os_Types.h"))
        OsIocDataType_validation_list = []
        OsIocDataType_validation_list = OsRuleutils().get_ioc_datatype(os_types_path)
        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            # 遍历OsIoc获取数据
            if "/Os/OsIoc" in os_instance["DefinitionRef"]:
                for ioc_key, ioc_value in os_instance.items():
                    OsIocDataType = ""
                    if not isinstance(ioc_value, dict):
                        continue
                    if "Os/OsIoc/OsIocCommunication" in ioc_value["DefinitionRef"]:
                        for ioc_communication_k,ioc_communication_v in ioc_value.items():
                            if not isinstance(ioc_communication_v,dict):
                                continue
                            if "OsIocCommunication/OsIocDataProperties" in ioc_communication_v["DefinitionRef"] and "OsIocDataType" in ioc_communication_v and ioc_communication_v["OsIocDataType"]["Value"] is not None:
                                OsIocDataType = ioc_communication_v["OsIocDataType"]["Value"]
                    validation_data = {}
                    param_path = "/ActiveEcuC/Os/OsIoc/OsIocCommunication/"+ioc_key
                    validation_data.update({"OsIoc": os_key})
                    validation_data.update({"OsIocDataType": OsIocDataType})
                    validation_data.update({"OsIocDataType_validation_list": OsIocDataType_validation_list})
                    try:
                        self.__class__.validation_result = False
                        if not OsIocDataType or OsIocDataType in OsIocDataType_validation_list:
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