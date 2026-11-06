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


class Os011002Rule(BasicRule):
    """
    重复校验: OsIocSenderId在同一个Ioc下不能重复
    """
    ruleset_name = "Os011002"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "No repeating of OsIocSenderId in one Ioc"
        self.durable_init()

    def durable_init(self):
        # 注：固定名称的规则集只能初始化一次，否则会异常报错
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            # print('get_ruleset() exception message: {0}'.format(e.args))
            with ruleset(self.__class__.ruleset_name):

                @when_all(+m.OsIocSenderId)
                def fact(c):
                    self.__class__.validation_result = True

    @timing_decorator
    def validation_run(self):
        """011表示:重复配置校验,002表示:os模块子校验码(001~999)"""
        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            if "/Os/OsIoc" in os_instance["DefinitionRef"]:
                for ioc_name, ioc_item in os_instance.items():
                    validation_data = {}
                    validation_data_list = []
                    OsIocSenderId_list = []
                    if not isinstance(ioc_item, dict):
                        continue
                    for ioc_content_k, ioc_content_v in ioc_item.items():
                        if not isinstance(ioc_content_v, dict):
                            continue
                        if "OsIoc/OsIocCommunication/OsIocSenderProperties" in ioc_content_v[
                          'DefinitionRef'] and "OsIocSenderId" in ioc_content_v:
                            OsIocSenderId_list.append(ioc_content_v["OsIocSenderId"]["Value"])
                    validation_data.update({"OsIocSenderId": OsIocSenderId_list})
                    validation_data.update({"ioc_name": ioc_name})
                    param_path = "/ActiveEcuC/Os/" + ioc_name
                    validation_data_list.append(validation_data)
                    try:
                        self.__class__.validation_result = False
                        if len(validation_data['OsIocSenderId']) == len(set(validation_data['OsIocSenderId'])):
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
                        self.vlog(self.validation_code, self.validation_summary, f"validation_data:{validation_data}",
                                  param_path, "fail")
                    if validation_data_list:
                        retract_facts(self.__class__.ruleset_name, validation_data_list)
                    pass
