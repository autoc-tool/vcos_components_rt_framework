from func_module.validation.common.basic_rule import BasicRule, timing_decorator
import json
import os, shutil
from collections import Counter

import re

class Os014023Rule(BasicRule):
    """
    任务共栈校验:在一个组合中的共栈任务必须具有相同的优先级
    """
    ruleset_name = "Os014023"
    validation_result = True

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "The shared stack tasks within a comb must have the same priority."

    def check_validation_run(self,bsw_ecuc_cfg_class_dict):
        if not self.modules_are_exist(["EcuC"],bsw_ecuc_cfg_class_dict):
            self.vlog(self.validation_code, self.validation_summary, "Data required for validation is missing",
                self.validation_summary, "fail")
            return False
        return True


    @timing_decorator
    def validation_run(self):
        """014表示:附加校验, 023表示:os模块子校验码(001~999)"""

        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            core_task = {} #用于记录本核所有共栈task
            if "/Os/OsCore" in os_instance["DefinitionRef"]:

                for sub_key, sub_instance in os_instance.items():
                    if not isinstance(sub_instance, dict):
                        continue
                    if "/Os/OsCore/OsTaskShareStack" in sub_instance["DefinitionRef"]:
                        task_comb_name = sub_key
                        param_path = "/ActiveEcuC/Os/" + os_key + "/" + task_comb_name
                        task_comb = {}
                        task_comb[param_path] = []
                        task_comb[param_path] = sub_instance.get("OsShareStackTaskComb").get("ValueRef")
                        core_task[task_comb_name] = task_comb
                        task_comb_priority = []
                        for task in task_comb[param_path]:
                            taskname = task.split('/')[-1]
                            task_cfg = self.ecuc_os_dict.get(taskname)
                            task_priority = task_cfg.get("OsTaskPriority").get("Value")
                            task_comb_priority.append(task_priority)
                        if len(set(task_comb_priority)) > 1:
                            self.validation_result = False
                            self.vlog(self.validation_code, self.validation_summary,
                                 f"Invalid comb:{task_comb_name}", param_path, "fail")