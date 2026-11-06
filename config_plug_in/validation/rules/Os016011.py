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

class Os016011Rule(BasicRule):
    """检验硬件驱动的Counter配置"""
    ruleset_name = "Os016011"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "If it is a hardware-driven counter, check the counter configuration"
        self.durable_init()

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            with ruleset(self.__class__.ruleset_name):
                @when_all(+m.counter_name == '')
                def action(c):
                    self.__class__.validation_result = True

    @timing_decorator
    def validation_run(self):
        ecuc_os_dict = self.ecuc_os_dict
        validation_data = {}
        publish_info = {}
        counter_isr_list = list()
        isr_dict = {}
        param_path = ''

        for os_key, os_instance in ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            if "DefinitionRef" in os_instance and "/Os/OsPublishedInformation" in os_instance["DefinitionRef"]:
                publish_info = os_instance["OsDerivativeInformation"]
            if "DefinitionRef" in os_instance and "Os/OsIsr" in os_instance["DefinitionRef"]:
                isr_dict[os_key] = os_instance

        for os_key, os_instance in ecuc_os_dict.items():
            param_path = "/ActiveEcuC/Os/" + os_key
            validation_data.update({"counter_name": ''})
            if not isinstance(os_instance, dict):
                continue
            if "DefinitionRef" in os_instance and "/Os/OsCounter" in os_instance["DefinitionRef"]:
                counter_type = os_instance.get("OsCounterType",{}).get("Value","")
                if counter_type == "HARDWARE":
                    if "OsDriver" not in os_instance:
                        validation_data.update({"counter_name": os_key})
                        validation_data.update({"error_detail": "hardware-driven counter must have a driver"})
                        break
                    else:
                        hardware_timer_name = OsRuleutils().get_last_item_verify(os_instance.get("OsDriver",{}).get("OsHardwareTimerChannelRef",{}).get("ValueRef",""))
                        isr_name = OsRuleutils().get_last_item_verify(os_instance.get("OsDriver",{}).get("OsIsrRef",{}).get("ValueRef",""))
                        hardware_channel_name = publish_info.get(hardware_timer_name,{}).get("OsHardWareTimerChannel",{}).get("Value","")
                        hardware_frequency = publish_info.get(hardware_timer_name,{}).get("OsHardWareTimerClockFrequency",{}).get("Value","")
                        second_per_tick = os_instance.get("OsSecondsPerTick",{}).get("Value","")
                        if second_per_tick > float(format(0.000001/hardware_frequency, '.15f')) or second_per_tick < 0.000001/hardware_frequency:
                            validation_data.update({"counter_name": os_key})
                            validation_data.update({"second_per_tick": format(0.000001/hardware_frequency, '.15f')})
                            validation_data.update({"error_detail": "OsSecondsPerTick is filled in incorrectly, it is recommended to fill in the value of second_per_tick"})
                            break
                        if isr_name in counter_isr_list:
                            validation_data.update({"counter_name": os_key})
                            validation_data.update({"isr_name": isr_name})
                            validation_data.update({"error_detail": "the isr is used to driver multiple counter"})
                            break
                        else:
                            counter_isr_list.append(isr_name)
                        if isr_dict.get(isr_name,{}).get("OsIsrInitialEnableInterruptSource",{}).get("Value",""):
                            validation_data.update({"counter_name": os_key})
                            validation_data.update({"isr_name": isr_name})
                            validation_data.update({"error_detail": "the isr has been initial enabled"})
                            break
                        isr_source = isr_dict.get(isr_name,{}).get("OsIsrInterruptSource",{}).get("Value","")
                        if hardware_channel_name.startswith("STM") and ((768 + int(hardware_channel_name[-1]) * 8) != isr_source):
                            validation_data.update({"counter_name": os_key})
                            validation_data.update({"isr_name": isr_name})
                            validation_data.update({"isr_source": 768 + int(hardware_channel_name[-1]) * 8})
                            validation_data.update({"error_detail": "OsIsrInterruptSource is filled in incorrectly, it is recommended to fill in the value of isr_source"})
                            break
                        elif hardware_channel_name.startswith("GTM") and (30 != isr_source):
                            validation_data.update({"counter_name": os_key})
                            validation_data.update({"isr_name": isr_name})
                            validation_data.update({"isr_source": 30})
                            validation_data.update({"error_detail": "OsIsrInterruptSource is filled in incorrectly, it is recommended to fill in the value of isr_source"})
                            break
                        else:
                            """新适配架构需要加校验"""
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
