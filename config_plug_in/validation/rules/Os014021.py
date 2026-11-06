from func_module.validation.common.basic_rule import BasicRule, timing_decorator
import json
import os, shutil
from collections import Counter
from .Os_ruleutils import OsRuleutils

import re

class Os014021Rule(BasicRule):
    """
    任务共栈校验:一个core下共栈的task 必须都属于本core
    """
    ruleset_name = "Os014021"
    validation_result = True

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.ecuc_dict = validation_data.bsw_ecuc_dict['EcuC']['ArPackages']['ActiveEcuC']['EcuC']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "Task must belong to the core."

    def check_validation_run(self,bsw_ecuc_cfg_class_dict):
        if not self.modules_are_exist(["EcuC"],bsw_ecuc_cfg_class_dict):
            self.vlog(self.validation_code, self.validation_summary, "Data required for validation is missing",
                self.validation_summary, "fail")
            return False
        return True


    @timing_decorator
    def validation_run(self):
        """014表示:附加校验, 021表示:os模块子校验码(001~999)"""

        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            # core_task = {} #用于记录本核所有共栈task
            if "/Os/OsCore" in os_instance["DefinitionRef"]:

                OsCoreBswApplicationRef = os_instance.get("OsCoreBswApplicationRef").get("ValueRef").split('/')[-1]
                for sub_key, sub_instance in os_instance.items():
                    if not isinstance(sub_instance, dict):
                        continue
                    if "/Os/OsCore/OsTaskShareStack" in sub_instance["DefinitionRef"]:
                        task_comb_name = sub_key
                        param_path = "/ActiveEcuC/Os/" + os_key + "/" + task_comb_name
                        task_comb = {}
                        task_comb[param_path] = []
                        task_comb[param_path] = sub_instance.get("OsShareStackTaskComb").get("ValueRef")

                        # core_task[task_comb_name] = task_comb
                        for task in task_comb[param_path]:
                            taskname = task.split('/')[-1]
                            task_app_name = OsRuleutils().get_app_from_task(taskname, self.ecuc_os_dict)
                            taskcoreid = OsRuleutils().get_core_number(task_app_name, self.ecuc_os_dict, self.ecuc_dict)
                            bswappcoreid = OsRuleutils().get_core_number(OsCoreBswApplicationRef, self.ecuc_os_dict, self.ecuc_dict)
                            if taskcoreid != bswappcoreid :
                                self.validation_result = False
                                self.vlog(self.validation_code, self.validation_summary,
                                 f"Invalid task:{taskname}", param_path, "fail")
