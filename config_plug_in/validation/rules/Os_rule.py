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
from func_module.validation.common.validation_def import ValidationData, ExceptionRuleLog
import importlib
from typing import Dict

class OsRule:
    """
    OS模块组件验证规则类
    负责执行各个OS组件的验证规则检查
    """

    validate_rule_groups = {
        "OsApplication": ["001001","001003","001006","002003","003001","014007"],
        "OsResource": ["013001","013006","014012"],
        "OsCounter": ["003003","014003","016011"],
        "OsEvent": ["003004","001002","014017"],
        "OsIsr": ["011003","014004", "013008"],
        "OsIoc": ["001003","003005","003006","003007","011001", "011002", "013011"],
        "OsScheduleTable": ["001007","002002","012003","013010","016003","014005","016004","016007"],
        "OsSpinlock": ["003002", "016001"],
        "OsTask": ["013004","014006","014008","014009","014010","014015","014016","014018","014020","014021","014022","014023"],
        "OsMemoryProtection": ["016008","016009","016010", "016012"],
        "OsCore": ["017001"],
    }

    def __init__(self) -> None:
        """初始化验证日志字典"""
        self.logs: Dict = {}

    def __validate_component_group(self, component_name: str) -> None:
        """
        执行单个组件组的验证
        """

        for validate_mark in self.validate_rule_groups[component_name]:
            try:
                module_path = f'.Os{validate_mark}'
                validate_rule_package = importlib.import_module(module_path, __package__)
                validate_rule_class = getattr(validate_rule_package, f'Os{validate_mark}Rule')

                os_rule_obj = validate_rule_class(self.logs, self.validate_data)
                os_rule_obj.validation_run()

            except Exception as e:
                ExceptionRuleLog.set_rule_exception_log(
                    self.logs, "Os", validate_mark,
                    f"{component_name} validation failed: {type(e).__name__}: {str(e)}"
                )

    def validate(self, validation_data: ValidationData) -> Dict:
        """
        执行所有OS组件的验证
        Args:
            validation_data: 验证数据对象
        Returns:
            Dict: 验证日志字典
        """
        self.validate_data = validation_data

        for component_name in self.validate_rule_groups.keys():
            self.__validate_component_group(component_name)
        return self.logs

    def get_logs(self) -> Dict:
        """
        获取验证日志

        Returns:
            Dict: 验证日志字典
        """
        return self.logs
