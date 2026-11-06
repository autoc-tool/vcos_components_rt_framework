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

class Os016007Rule(BasicRule):
    """scheduletable只能关联本核的counter"""
    ruleset_name = "Os016007"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "Scheduletable can only associate Counter with the same Core and OsScheduleTableCounterRef must be config"
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
                @when_all((m.OsScheduleTableCounterRefCore == m.OsScheduleTableCore) & (+m.OsScheduleTableCounter))
                def action(c):
                    self.__class__.validation_result = True
            pass

    @timing_decorator
    def validation_run(self):
        ecuc_os_dict = self.ecuc_os_dict
        ruleutils = OsRuleutils()
        for os_key, os_instance in ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            # 只校验调度表
            if "/Os/OsScheduleTable" in os_instance.get("DefinitionRef", "") and "OsScheduleTableCounterRef" in os_instance:
                # 获取调度表关联的Counter
                counter_field = os_instance.get("OsScheduleTableCounterRef", "")
                if isinstance(counter_field, dict):
                    counter_ref = counter_field.get("ValueRef", "")
                else:
                    counter_ref = counter_field
                counter_name = ruleutils.get_last_item_verify(counter_ref)
                # 获取调度表所属OsApplication
                app_sched_name = ruleutils.find_application_by_schedule_table(os_key, ecuc_os_dict)
                # 获取Counter所属OsApplication
                app_counter_name = ruleutils.find_application_by_counter(counter_name, ecuc_os_dict)
                if not app_sched_name or not app_counter_name:
                    self.vlog(self.validation_code, self.validation_summary, f"ScheduleTable or Counter cannot find its OsApplication", os_key, "fail")
                    continue
                # 获取两个OsApplication的CoreRef
                def get_core_ref(app_name):
                    app = ecuc_os_dict.get(app_name, {})
                    core_field = app.get("OsApplicationCoreRef", "")
                    if isinstance(core_field, dict):
                        core_ref = core_field.get("ValueRef", "")
                    else:
                        core_ref = core_field
                    return ruleutils.get_last_item_verify(core_ref)
                core_sched = get_core_ref(app_sched_name)
                core_counter = get_core_ref(app_counter_name)
                if core_sched != core_counter:
                    self.vlog(self.validation_code, self.validation_summary, f"ScheduleTable({os_key}) and Counter({counter_name}) are not on the same Core: {core_sched} vs {core_counter}", os_key, "fail")