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


class Os013001Rule(BasicRule):
    """每个resource的被引用task所关联的所有applications必须与OsResourceAccessingApplication一致"""
    ruleset_name = "Os013001"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "Each application associated with tasks that reference a given resource must be included in OsResourceAccessingApplication."
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

                @when_all(+m.OsResourceApplicationsMatch)
                def action(c):
                    self.__class__.validation_result = True

    @timing_decorator
    def validation_run(self):
        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            # 遍历Resource
            if "/Os/OsResource" in os_instance["DefinitionRef"]:
                resource_name = os_key

                # 1. 获取Resource被引用的Task所关联的Applications
                task_related_apps = set()
                task_list = OsRuleutils().get_tasklist_from_resource(resource_name, self.ecuc_os_dict)
                for task_name in task_list:
                    app_name = OsRuleutils().get_app_from_task(task_name, self.ecuc_os_dict)
                    if app_name:
                        task_related_apps.add(app_name)

                # 2. 获取Resource的OsResourceAccessingApplication配置
                configured_apps = set()
                if "OsResourceAccessingApplication" in os_instance:
                    accessing_app_ref = os_instance["OsResourceAccessingApplication"]
                    if isinstance(accessing_app_ref, dict):
                        # 处理单个引用或多个引用的情况
                        value_ref = accessing_app_ref.get("ValueRef", "")
                        if isinstance(value_ref, list):
                            # 多个引用的情况
                            for ref in value_ref:
                                if ref:
                                    app_name = OsRuleutils().get_last_item_verify(ref)
                                    if app_name:
                                        configured_apps.add(app_name)
                        elif isinstance(value_ref, str) and value_ref:
                            # 单个引用的情况
                            app_name = OsRuleutils().get_last_item_verify(value_ref)
                            if app_name:
                                configured_apps.add(app_name)
                    elif isinstance(accessing_app_ref, list):
                        for ref in accessing_app_ref:
                            if isinstance(ref, dict):
                                app_ref = ref.get("ValueRef", "")
                            else:
                                app_ref = ref
                            if app_ref:
                                app_name = OsRuleutils().get_last_item_verify(app_ref)
                                if app_name:
                                    configured_apps.add(app_name)
                    elif isinstance(accessing_app_ref, str):
                        app_name = OsRuleutils().get_last_item_verify(accessing_app_ref)
                        if app_name:
                            configured_apps.add(app_name)

                # 3. 比较两个集合是否一致
                applications_match = (task_related_apps == configured_apps)

                OsResource = os_key
                OsResourceTaskRelatedApps = list(task_related_apps)
                OsResourceConfiguredApps = list(configured_apps)
                OsResourceApplicationsMatch = applications_match

                validation_data = {}
                param_path = "/ActiveEcuC/Os/" + os_key
                validation_data.update({"OsResource": OsResource})
                validation_data.update({"OsResourceTaskRelatedApps": OsResourceTaskRelatedApps})
                validation_data.update({"OsResourceConfiguredApps": OsResourceConfiguredApps})
                validation_data.update({"OsResourceApplicationsMatch": OsResourceApplicationsMatch})

                try:
                    self.__class__.validation_result = False
                    if applications_match:
                        post(self.__class__.ruleset_name, validation_data)
                    else:
                        self.__class__.validation_result = False
                except Exception as e:
                    pass

                # 自动修复功能：当校验失败时，生成修复建议
                auto_solve_list = []
                if not self.__class__.validation_result:
                    if "OsResourceAccessingApplication" in os_instance and isinstance(os_instance["OsResourceAccessingApplication"], dict):
                        resource_param_path = os_instance["OsResourceAccessingApplication"].get("reference_item_path", f"/ActiveEcuC/Os/{os_key}/OsResourceAccessingApplication")
                    else:
                        resource_param_path = f"/ActiveEcuC/Os/{os_key}/OsResourceAccessingApplication"

                    # 找出需要添加的应用（在task_related_apps中但不在configured_apps中）
                    apps_to_add = task_related_apps - configured_apps
                    # 找出需要删除的应用（在configured_apps中但不在task_related_apps中）
                    apps_to_remove = configured_apps - task_related_apps

                    # 处理需要删除的应用
                    if apps_to_remove:
                        apps_to_remove_refs = [f"/ActiveEcuC/Os/{app_name}" for app_name in sorted(apps_to_remove)]
                        auto_solve_list.append({
                            "solve_info": f"Remove {', '.join(sorted(apps_to_remove))} from OsResourceAccessingApplication of {os_key}.",
                            "solve_type": "DeleteReference",
                            "parameter": resource_param_path,
                            "value": apps_to_remove_refs
                        })

                    # 处理需要添加的应用
                    if apps_to_add:
                        for app_name in sorted(apps_to_add):
                            auto_solve_list.append({
                                "solve_info": f"Add {app_name} to OsResourceAccessingApplication of {os_key}.",
                                "solve_type": "SetValue",
                                "parameter": resource_param_path,
                                "value": f"/ActiveEcuC/Os/{app_name}"
                            })

                # 格式："错误码", "摘要", "错误信息", "错误索引路径" "结果"
                if self.__class__.validation_result:
                    self.vlog(self.validation_code, self.validation_summary,
                              f"validation_data:{validation_data}", param_path, "pass")
                else:
                    self.vlog(self.validation_code, self.validation_summary,
                              f"validation_data:{validation_data}", param_path, "fail", auto_solve_list)
        return self.__class__.validation_result
