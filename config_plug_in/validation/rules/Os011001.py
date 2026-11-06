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

class Os011001Rule(BasicRule):
    """
    重复校验: 同一Ioc下OsIocReceivingOsApplicationRef 和 OsIocSendingOsApplicationRef 不能有相同的application
    """
    ruleset_name = "Os011001"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "Validation OsIocReceivingOsApplicationRef and OsIocSendingOsApplicationRef difference in one ioc"
        self.durable_init()

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            with ruleset(self.__class__.ruleset_name):

                @when_all(+m.ioc_send)
                def check_no_matching_elements(c):
                    #IOC中send:receive=n:1 或者send:receive=1:1，因此取存在于send但不存在于receive的差集，如果差集存在则pass
                    if set(c.m.ioc_send).difference(set(c.m.ioc_receive)) or set(c.m.ioc_receive).difference(
                      set(c.m.ioc_send)):
                        self.__class__.validation_result = True

    @timing_decorator
    def validation_run(self):
        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            if os_instance["DefinitionRef"] == "/Os/OsIoc" or "/Os/OsIoc/OsIocCommunication" in os_instance["DefinitionRef"]:
                for ioc_name, ioc_item in os_instance.items():
                    validation_data = {}
                    OsIocReceivingOsApplicationRef_list = []
                    OsIocSendingOsApplicationRef_list = []
                    if not isinstance(ioc_item, dict):
                        continue
                    for ioc_content_k, ioc_content_v in ioc_item.items():
                        if not isinstance(ioc_content_v, dict):
                            continue
                        if "OsIoc/OsIocCommunication/OsIocReceiverProperties" in ioc_content_v[
                          'DefinitionRef'] and "OsIocReceivingOsApplicationRef" in ioc_content_v and OsRuleutils(
                          ).get_last_item_verify(ioc_content_v["OsIocReceivingOsApplicationRef"].get("ValueRef", "")):
                            OsApplicationRef = OsRuleutils().get_last_item_verify(
                              ioc_content_v["OsIocReceivingOsApplicationRef"]["ValueRef"])
                            OsIocReceivingOsApplicationRef_list.append(OsApplicationRef)
                        if "OsIoc/OsIocCommunication/OsIocSenderProperties" in ioc_content_v[
                          'DefinitionRef'] and "OsIocSendingOsApplicationRef" in ioc_content_v and OsRuleutils(
                          ).get_last_item_verify(ioc_content_v["OsIocSendingOsApplicationRef"].get("ValueRef", "")):
                            OsApplicationRef = OsRuleutils().get_last_item_verify(
                              ioc_content_v["OsIocSendingOsApplicationRef"]["ValueRef"])
                            OsIocSendingOsApplicationRef_list.append(OsApplicationRef)
                    validation_data['ioc_receive'] = OsIocReceivingOsApplicationRef_list
                    validation_data['ioc_send'] = OsIocSendingOsApplicationRef_list
                    param_path = "/ActiveEcuC/Os/" + ioc_name
                    try:
                        self.__class__.validation_result = False
                        post(self.__class__.ruleset_name, validation_data)
                    except Exception as e:
                        # print('assert_fact() exception message {0}'.format(e.args))
                        pass
                    # 格式："错误码", "摘要", "错误信息", "错误索引路径" "结果"
                    if self.__class__.validation_result:
                        self.vlog(self.validation_code, self.validation_summary, f"validation_data:{validation_data}",
                                  param_path, "pass")
                    else:
                        self.vlog(self.validation_code, self.validation_summary, f"validation_data:{validation_data}",
                                  param_path, "fail")
