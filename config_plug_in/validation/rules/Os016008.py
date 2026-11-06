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

class Os016008Rule(BasicRule):
    """ARM R52 芯片, MPU的地址范围不能重复"""
    ruleset_name = "Os016008"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "The mpu regions of the R52 architecture cannot overlap"
        self.durable_init()

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            with ruleset(self.__class__.ruleset_name):
                @when_all(+m.regions_addr_range)
                def action(c):
                    self.__class__.validation_result = True

    @timing_decorator
    def validation_run(self):
        mpu_r52_active = True
        ecuc_os_dict = self.ecuc_os_dict

        # 非SC3或者SC4,则不需要检查
        for os_key, os_instance in ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            if "/Os/OsOS" in os_instance["DefinitionRef"] and "OsScalabilityClass" in os_instance:
                if not os_instance["OsScalabilityClass"].get("Value","") in ("SC3","SC4"):
                    mpu_r52_active = False

        # 非R52架构,则不需要检查
        if mpu_r52_active == True:
            if "Armv8rE3650" not in ecuc_os_dict:
                mpu_r52_active = False

        # 获取MPU region list
        region_list = {}
        for os_key, os_instance in ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            # 遍历OsMemoryProtection获取数据
            if "/Os/OsMemoryProtection" in os_instance["DefinitionRef"]:
                region_list = os_instance

        # 校验R52 MPU 地址是否重复
        regions_addr_range = {}
        for region_name, region_value in region_list.items():
            if not isinstance(region_value, dict):
                continue
            if mpu_r52_active:
                start_addr_str = region_value.get("OsMemoryRegionStartAddress",{}).get("Value","")
                end_addr_str = region_value.get("OsMemoryRegionEndAddress",{}).get("Value","")
                protection_unit = OsRuleutils().get_last_item_verify(region_value.get("OsMemoryRegionProtectionUnit",{}).get("ValueRef",""))
                # 只有配置为0x或者0X开启的数值才参与校验
                if (start_addr_str.startswith("0x") or start_addr_str.startswith("0X")) and (end_addr_str.startswith("0x") or end_addr_str.startswith("0X")):
                    start_addr = int(start_addr_str, 16)
                    end_addr = int(end_addr_str, 16)
                    # 检查新地址范围是否与已有的任何范围重叠
                    has_address_overlapped = False
                    for existing_region, (existing_start, existing_end, existing_protection_unit) in regions_addr_range.items():
                        if (existing_protection_unit == protection_unit) and \
                            ((existing_start < start_addr < existing_end) or (existing_start < end_addr < existing_end)):
                            has_address_overlapped = True
                            validation_data = {}
                            param_path = "/ActiveEcuC/Os/OsMemoryProtection/" + region_name
                            validation_data.update({"existing_region": existing_region})
                            validation_data.update({"overlap_region": region_name})
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
                    # 如果没有重叠，则添加新的区域
                    if has_address_overlapped == False:
                        regions_addr_range[region_name] = (start_addr, end_addr, protection_unit)