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

class Os016004Rule(BasicRule):
    """调度表设置为周期执行,OsScheduleTableDuration不能超出OsCounterMaxAllowedValue,但如果调度表的OsScheduleTblSyncStrategy=IMPLICIT,则OsScheduleTableDuration必须等于OsCounter的OsCounterMaxAllowedValue+1。"""
    ruleset_name = "Os016004"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = ("If OsScheduleTableRepeating=true, OsScheduleTableDuration must be less than OsCounter's OsCounterMaxAllowedValue, "
                                   "but if a ScheduleTable has OsScheduleTblSyncStrategy = IMPLICIT, OsScheduleTableDuration must be equal to OsCounter's OsCounterMaxAllowedValue + 1.")
        self.durable_init()

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            with ruleset(self.__class__.ruleset_name):
                @when_all(+m.OsScheduleTableDuration)
                def action(c):
                    self.__class__.validation_result = True

    def get_sync_strategy(self, os_instance):
        """
        Get the OsScheduleTblSyncStrategy value from schedule table instance
        Args:
            os_instance: schedule table instance dict
        Returns:
            sync strategy string (IMPLICIT, EXPLICIT, NONE)
        """
        sync_strategy = "NONE"

        # Check if OsScheduleTableSync container exists
        for key, value in os_instance.items():
            if key == "OsScheduleTableSync" and isinstance(value, dict):
                # Look for OsScheduleTblSyncStrategy in the sync container
                if "OsScheduleTblSyncStrategy" in value:
                    sync_strategy = value["OsScheduleTblSyncStrategy"].get("Value", "NONE")
                break

        return sync_strategy

    @timing_decorator
    def validation_run(self):
        ecuc_os_dict = self.ecuc_os_dict
        for os_key, os_instance in ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            # Check if this is a schedule table with repeating enabled
            if "/Os/OsScheduleTable" in os_instance["DefinitionRef"] and "OsScheduleTableRepeating" in os_instance:
                OsCounterMaxAllowedValue, OsScheduleTableDuration = 0, 0
                OsScheduleTableRepeating = os_instance["OsScheduleTableRepeating"].get("Value", False)

                if OsScheduleTableRepeating:
                    OsScheduleTableDuration = os_instance["OsScheduleTableDuration"].get("Value", 0)
                    ScheduleTable_counter = OsRuleutils().get_module_property_valueref("OsScheduleTable", os_key, "OsScheduleTableCounterRef", ecuc_os_dict)

                    if ScheduleTable_counter:
                        OsCounterMaxAllowedValue = OsRuleutils().get_counter_OsCounterMaxAllowedValue(ScheduleTable_counter[0], ecuc_os_dict)

                    sync_strategy = self.get_sync_strategy(os_instance)

                    validation_data = {}
                    param_path = "/ActiveEcuC/Os/" + os_key
                    validation_data.update({"OsScheduleTable": os_key})
                    validation_data.update({"OsCounterMaxAllowedValue": OsCounterMaxAllowedValue})
                    validation_data.update({"OsScheduleTableDuration": OsScheduleTableDuration})
                    validation_data.update({"OsScheduleTblSyncStrategy": sync_strategy})

                    try:
                        self.__class__.validation_result = False

                        # Apply different validation rules based on sync strategy
                        if sync_strategy == "IMPLICIT":
                            # For IMPLICIT sync: Duration must equal MaxAllowedValue + 1
                            if OsScheduleTableDuration == (OsCounterMaxAllowedValue + 1):
                                post(self.__class__.ruleset_name, validation_data)
                        else:
                            # For other sync strategies: Duration must be less than MaxAllowedValue
                            if OsScheduleTableDuration < OsCounterMaxAllowedValue:
                                post(self.__class__.ruleset_name, validation_data)

                    except Exception as e:
                        pass

                    if self.__class__.validation_result:
                        self.vlog(self.validation_code, self.validation_summary,
                                f"validation_data:{validation_data}", param_path, "pass")
                    else:
                        error_msg = ""
                        if sync_strategy == "IMPLICIT":
                            error_msg = f"IMPLICIT sync strategy: OsScheduleTableDuration({OsScheduleTableDuration}) must equal OsCounterMaxAllowedValue+1({OsCounterMaxAllowedValue + 1})"
                        else:
                            error_msg = f"{sync_strategy} sync strategy: OsScheduleTableDuration({OsScheduleTableDuration}) must be less than OsCounterMaxAllowedValue({OsCounterMaxAllowedValue})"

                        self.vlog(self.validation_code, self.validation_summary,
                                f"validation_data:{validation_data}, {error_msg}", param_path, "fail")
