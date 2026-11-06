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

class Os016009Rule(BasicRule):
    """检验配置MPU Region地址是否正确"""
    ruleset_name = "Os016009"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "The MPU region address requires specific alignment"
        self.durable_init()

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            with ruleset(self.__class__.ruleset_name):
                @when_all(+m.region_name == '')
                def action(c):
                    self.__class__.validation_result = True

    @timing_decorator
    def validation_run(self):
        ecuc_os_dict = self.ecuc_os_dict
        mpu_active = False
        validation_data = {}
        param_path = ''

        # 非SC3或者SC4,则不需要检查
        for os_key, os_instance in ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            if "/Os/OsOS" in os_instance["DefinitionRef"] and "OsScalabilityClass" in os_instance:
                if os_instance["OsScalabilityClass"].get("Value","") in ("SC3","SC4"):
                    mpu_active = True

        # 获取MPU region list
        region_list = {}
        for os_key, os_instance in ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            if "/Os/OsMemoryProtection" in os_instance["DefinitionRef"]:
                region_list = os_instance

        # 校验MPU地址是否符合规范
        for region_name, region_value in region_list.items():
            if not isinstance(region_value, dict):
                continue
            if mpu_active:
                param_path = "/ActiveEcuC/Os/OsMemoryProtection/" + region_name
                start_addr_str = region_value.get("OsMemoryRegionStartAddress",{}).get("Value","")
                end_addr_str = region_value.get("OsMemoryRegionEndAddress",{}).get("Value","")
                access_rights = OsRuleutils().get_last_item_verify(region_value.get("OsMemoryRegionAccessRights",{}).get("ValueRef",""))
                flags = OsRuleutils().get_last_item_verify(region_value.get("OsMemoryRegionFlags",{}).get("ValueRef",""))
                validation_data.update({"region_name": ''})
                # 只有配置为0x或者0X开启的数值才参与校验
                if (start_addr_str.startswith("0x") or start_addr_str.startswith("0X")) and (end_addr_str.startswith("0x") or end_addr_str.startswith("0X")):
                    start_addr = int(start_addr_str, 16)
                    end_addr = int(end_addr_str, 16)
                    if start_addr >= end_addr:
                        validation_data.update({"region_name": region_name})
                        validation_data.update({"error_detail": "region start address is greater than end address"})
                    elif "TriCore397" in ecuc_os_dict:
                        if '_R' in access_rights and (start_addr % 8 != 0 or end_addr % 8 != 0):
                            validation_data.update({"region_name": region_name})
                            validation_data.update({"error_detail": "data region address requires 8-byte alignment"})
                        if '_X' in access_rights or '_RX' in access_rights and (start_addr % 32 != 0 or end_addr % 32 != 0):
                            validation_data.update({"region_name": region_name})
                            validation_data.update({"error_detail": "code region address requires 64-byte alignment"})
                    elif "Armv8rE3650" in ecuc_os_dict:
                        if (start_addr % 64 != 0 or (end_addr + 1) % 64 != 0):
                            validation_data.update({"region_name": region_name})
                            validation_data.update({"error_detail": "start address requires 64-byte alignment and (end address + 1) requires 64-byte alignment"})
                        if flags == None or flags == '':
                            validation_data.update({"region_name": region_name})
                            validation_data.update({"error_detail": "OsMemoryRegionFlags must be selected for the region"})
                try:
                    self.__class__.validation_result = False
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
