from func_module.validation.common.basic_rule import BasicRule, timing_decorator
import json
import os, shutil
from collections import Counter

import re

class Os014020Rule(BasicRule):
    """
    任务共栈校验:一个task 仅能出现在一个comb 中
    """
    ruleset_name = "Os014020"
    validation_result = True

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "A task can only appear in one comb."


    def check_validation_run(self,bsw_ecuc_cfg_class_dict):
        if not self.modules_are_exist(["EcuC"],bsw_ecuc_cfg_class_dict):
            self.vlog(self.validation_code, self.validation_summary, "Data required for validation is missing",
                self.validation_summary, "fail")
            return False
        return True


    @timing_decorator
    def validation_run(self):
        """014表示:附加校验, 020表示:os模块子校验码(001~999)"""

        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            if "/Os/OsCore" in os_instance["DefinitionRef"]:
                param_path = "/ActiveEcuC/Os/" + os_key
                core_task = [] #用于记录本核所有共栈task
                for sub_key, sub_instance in os_instance.items():
                    if not isinstance(sub_instance, dict):
                        continue
                    if "/Os/OsCore/OsTaskShareStack" in sub_instance["DefinitionRef"]:
                        task_comb_name = sub_key
                        task_comb_list = []
                        task_comb_list = sub_instance.get("OsShareStackTaskComb").get("ValueRef")
                        core_task +=task_comb_list
                """ coretask 中如果存在重复则报错 """
                task_counts = Counter(core_task)
                duplicates = [task for task, count in task_counts.items() if count > 1]

                if duplicates:
                    self.vlog(self.validation_code, self.validation_summary,
                                 f"duplicate task:{duplicates}", param_path, "fail")