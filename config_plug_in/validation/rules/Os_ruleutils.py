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
import os,re
class OsRuleutils():

    def __init__(self):
        pass

    def get_last_item_verify(self, x):
        """
        返回string.split('/')的最后一部分内容，若返回值为"<empty>"则返回空字符串
        参数：
            x:输入字符串
        返回值：
            result(x)
        """
        result = lambda x: x.split('/')[-1] if (isinstance(x, str) and "/" in x) else ('' if x == "<empty>" or not x else x)
        return result(x)

    def get_core_id_from_ecuc_core_def(self, ecuc_dict, ecuc_core):
        for k, v in ecuc_dict.items():
            if "DefinitionRef" in v and "/EcuC/EcucHardware" in v["DefinitionRef"]:
                if ecuc_core in v:
                    return v[ecuc_core]['EcucCoreId']['Value']
        return -1

    def get_core_number(self, appName, ecuc_os_dict, ecuc_dict):
        """
        根据application名称,查询Ecuc配置返回该application所属的核号
        参数：
            appName: osApplication名称
            ecuc_os_dict: Os UI整体字典，包含EcucHardware信息
        返回值：
            coreid
        """
        # 查找Application的OsApplicationCoreRef
        for k, v in ecuc_os_dict.items():
            if "DefinitionRef" in v and "/Os/OsApplication" in v["DefinitionRef"] and k == appName:
                if "OsApplicationCoreRef" in v and v["OsApplicationCoreRef"]:
                    OsApplicationCoreRef_full = v["OsApplicationCoreRef"].get("ValueRef", "")
                    OsApplicationCoreRef = self.get_last_item_verify(OsApplicationCoreRef_full)
                    return self.get_core_id_from_ecuc_core_def(ecuc_dict, OsApplicationCoreRef)
        return -1

    def get_ref_list(self, input_ref):
        """
        输入input_ref,判断若input_ref为list则遍历插入result_list,判断若input_ref为字符串则去空格后插入result_list,
        最后返回合成的result_list。此函数通常用于读取arxml中的ValueRef值。
        参数：
            input_ref: arxml中的ValueRef
        返回值：
            result_list
        """
        result_list = []
        if isinstance(input_ref, list):
            for i in input_ref:
                result_list.append(self.get_last_item_verify(i.strip() if isinstance(i, str) else i))
        elif isinstance(input_ref, str) and input_ref.strip() != "" and input_ref.strip() != "<empty>":
            result_list.append(self.get_last_item_verify(input_ref.strip()))
        elif isinstance(input_ref, dict):
            v = input_ref.get('ValueRef', '')
            if isinstance(v, list):
                for i in v:
                    result_list.append(self.get_last_item_verify(i.strip() if isinstance(i, str) else i))
            elif isinstance(v, str) and v.strip() != "" and v.strip() != "<empty>":
                result_list.append(self.get_last_item_verify(v.strip()))
        return result_list

    def get_tasklist_from_resource(self, resource_name, ecuc_os_dict):
        """
        查找resource所属的task list,返回task list
        参数：
            resource_name: resource名称
            ecuc_os_dict :ecuc_os_dict:Os UI整体字典
        返回值：
            task list
        """
        task_list = []
        for k, v in ecuc_os_dict.items():
            if "DefinitionRef" in v and "/Os/OsTask" in v["DefinitionRef"]:
                OsTaskResourceRef_list = []
                if isinstance(v["OsTaskResourceRef"].get("ValueRef", ""), list):
                    for i in v["OsTaskResourceRef"]["ValueRef"]:
                        OsTaskResourceRef_list.append(self.get_last_item_verify(i))
                if isinstance(v["OsTaskResourceRef"].get("ValueRef", ""), str):
                    OsTaskResourceRef_list.append(self.get_last_item_verify(v["OsTaskResourceRef"].get("ValueRef", "")))
                if resource_name in OsTaskResourceRef_list:
                    task_list.append(k)
        return task_list

    def get_app_from_task(self, task_name, ecuc_os_dict):
        """
        查找task所属的owner application,返回一个appliation name
        参数：
            task_name: task名称
            ecuc_os_dict :ecuc_os_dict:Os UI整体字典
        返回值：
            appliation name
        """
        app_name = ""
        for k, v in ecuc_os_dict.items():
            if "DefinitionRef" in v and "/Os/OsApplication" in v["DefinitionRef"]:
                OsAppTaskRef_list = []
                if isinstance(v["OsAppTaskRef"].get("ValueRef", ""), list):
                    for i in v["OsAppTaskRef"]["ValueRef"]:
                        OsAppTaskRef_list.append(self.get_last_item_verify(i))
                if isinstance(v["OsAppTaskRef"].get("ValueRef", ""), str):
                    OsAppTaskRef_list.append(self.get_last_item_verify(v["OsAppTaskRef"].get("ValueRef", "")))
                if task_name in OsAppTaskRef_list:
                    app_name = k
        return app_name

    def get_ioc_datatype(self, types_path)->list:
        """
        查找获取所有的Ioc支持的datatype类型, 返回列表
        参数：
            types_path: Os_Types.h路径
        返回值：
            ioc_datatype_list
        """
        ioc_datatype_list, os_types_list = [], []
        basic_datatype_list = (['boolean', 'char', 'double', 'float', 'float32', 'float64', 'int', 'long', 'long double', 'long int', 'long long', 'long long int', 'short', 'short int', 'signed', 'signed char',
                           'signed int', 'signed long', 'signed long int', 'signed long long', 'signed long long int', 'signed short', 'signed short int', 'sint8', 'sint8_least', 'sint16', 'sint16_least',
                           'sint32', 'sint32_least', 'sint64', 'uint8', 'uint8_least', 'uint16', 'uint16_least', 'uint32', 'uint32_least', 'uint64', 'unsigned', 'unsigned char', 'unsigned int', 'unsigned long', 'unsigned long int',
                           'unsigned long long', 'unsigned long long int', 'unsigned short', 'unsigned short int'])

        # 正则匹配Os_Types.h文件，获取所有typedef的自定义类型
        try:
            with open(types_path,'r') as f:
                content = f.read()
            pattern = r'\s*typedef.*(\b\w+[^)}]);'
            pattern2 =r'typedef(?:.|\n)*?}\s*(\w+)\W*;'
            os_types_list = re.findall(pattern, content)
            os_types_list2 = re.findall(pattern2, content)
            set1 = set(os_types_list)
            set2 = set(os_types_list2)
            union_set = set1.union(set2)
            os_types_list = list(union_set)
        except FileNotFoundError as e:
            print(f"File not found: {str(e)}")
        except Exception as e:
            print(f"An error occurred: {str(e)}")

        ioc_datatype_list.extend(basic_datatype_list)
        if os_types_list:
           ioc_datatype_list.extend(os_types_list)
        return ioc_datatype_list

    def get_module_property_value(self, module_type, module_name, property_name, ecuc_os_dict):
        """
        获取os各模块的属性的value值。
        参数：
            module_type : OsApplication,OsAlarm等模块类型
            module_name : OsApplication,OsAlarm等模块实例化名称
            property_name : 属性名
            ecuc_os_dict : ecuc_os_dict:Os UI整体字典
        返回值：
            Value类型的属性值
        """
        property_value = ""
        for os_key, os_instance in ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            if "/Os/" + module_type in os_instance[
              "DefinitionRef"] and module_name == os_key and property_name in os_instance:
                property_value = os_instance[property_name].get("Value", "")
        return property_value

    def get_module_property_valueref(self, module_type, module_name, property_name, ecuc_os_dict):
        """
        获取os各模块的属性的ValueRef值。
        参数：
            module_type : OsApplication,OsAlarm等模块类型
            module_name : OsApplication,OsAlarm等模块实例化名称
            property_name : 属性名
            ecuc_os_dict : ecuc_os_dict:Os UI整体字典
        返回值：
            ValueRef类型的属性值
        """
        result_list = []
        for os_key, os_instance in ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            if "/Os/" + module_type in os_instance[
              "DefinitionRef"] and module_name == os_key and property_name in os_instance:
                valueref = os_instance[property_name].get("ValueRef", "")
                if isinstance(valueref, list):
                    for i in valueref:
                        result_list.append(self.get_last_item_verify(i))
                elif isinstance(valueref, str) and valueref != "<empty>" and valueref != "" and valueref != "null":
                    result_list.append(self.get_last_item_verify(valueref))
        return result_list

    def get_module_cfg(self, module_type, ecuc_os_dict):
        """
        返回os模块的整体信息
        参数：
            module_type:os模块类型.e.g.OsApplication,OsAlarm等模块类型
            ecuc_os_dict:Os UI整体字典
        返回值：
            module_cfg
        """
        module_cfg = {}
        for os_key, os_instance in ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            if "/Os/" + module_type in os_instance["DefinitionRef"]:
                module_cfg.update({os_key: os_instance})
        return module_cfg

    def get_hard_info(self,ecuc_hardware_dict):
        """访问核0的硬件配置信息返回硬件类型E3640/TriCore

        Args:
            ecuc_hardware_dict (_type_): os模块中配置的硬件信息
        """
        hard_type = "E3640"
        for hard_k,hard_v in ecuc_hardware_dict.items():
            if not isinstance(hard_v,dict):
                continue
            if "EcucHardware/EcucCoreDefinition" in hard_v["DefinitionRef"]:
                EcucCoreHwRef = hard_v.get("EcucCoreHwRef","").get("ValueRef","")
                if EcucCoreHwRef.lower().find("tricore") != -1:
                   hard_type = "TriCore"
        return hard_type

    def get_module_all_property_valueref(self, module_type, property_name, ecuc_os_dict):
        """
        获取os各模块的所有属性的ValueRef值。
        参数：
            module_type : OsApplication,OsAlarm等模块类型
            property_name : 属性名
            ecuc_os_dict : ecuc_os_dict:Os UI整体字典
        返回值：
            ValueRef类型的属性值
        """
        result_list = []
        for os_key, os_instance in ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            if "/Os/" + module_type in os_instance[
              "DefinitionRef"] and property_name in os_instance:
                valueref = os_instance[property_name].get("ValueRef", "")
                if isinstance(valueref, list):
                    for i in valueref:
                        result_list.append(self.get_last_item_verify(i))
                elif isinstance(valueref, str) and valueref != "<empty>" and valueref != "":
                    result_list.append(self.get_last_item_verify(valueref))
        return result_list

    def get_counter_OsCounterMaxAllowedValue(self, counter_name, ecuc_os_dict):
        """
        返回counter_name对应的OsCounterMaxAllowedValue字段的值
        参数：
            counter_name:UI中配置的OsCounter
            ecuc_os_dict:Os UI整体字典
        返回值：
            OsCounterMaxAllowedValue
        """
        OsCounterMaxAllowedValue = 0
        for os_key, os_instance in ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            if "/Os/OsCounter" in os_instance["DefinitionRef"] and os_key == counter_name and "OsCounterMaxAllowedValue" in os_instance:
                OsCounterMaxAllowedValue = os_instance.get("OsCounterMaxAllowedValue",{}).get("Value",0)
        return OsCounterMaxAllowedValue

    def get_app_from_os_item(self, OsAppItem,OsAppItemRef,ecuc_os_dict):
        """根据OsAppItem查找其所属的Application,比如根据OsAppItem='SystemTimer3'的counter名查找其所属的'SystemApplication3'

        Args:
            OsAppItem (_type_): os 元素名称包括：task,counter,alarm,isr,ScheduleTable实例化名称
            OsAppItemRef (_type_): os 元素名称对应的Ref包括：OsAppAlarmRef,OsAppCounterRef,OsAppIsrRef,OsAppScheduleTableRef,OsAppTaskRef
            ecuc_os_dict (_type_): Os UI整体字典
        """
        app_name = ""
        for os_key, os_instance in ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            if "/Os/OsApplication" in os_instance["DefinitionRef"] and OsAppItemRef in os_instance:
                OsAppItemRef_list = os_instance.get(OsAppItemRef).get("ValueRef","")
                if not OsAppItemRef_list:
                    continue
                if OsAppItemRef_list and isinstance(OsAppItemRef_list, list):
                    # print(f"OsAppItemRef_list:{OsAppItemRef_list}")
                    for i in OsAppItemRef_list:
                        if OsAppItem == self.get_last_item_verify(i):
                            app_name = os_key
                            return app_name
                if OsAppItemRef_list and isinstance(OsAppItemRef_list,str):
                    OsAppItemRef_splited = self.get_last_item_verify(OsAppItemRef_list)
                    # print(f"OsAppItemRef_splited:{OsAppItemRef_splited}")
                    if OsAppItem == OsAppItemRef_splited:
                        app_name = os_key
                        return app_name
        return app_name

    def find_application_by_counter(self, counter_name, ecuc_os_dict):
        """
        根据Counter名查找其所属的OsApplication名
        """
        for app_key, app_inst in ecuc_os_dict.items():
            if not isinstance(app_inst, dict):
                continue
            if "/Os/OsApplication" in app_inst.get("DefinitionRef", ""):
                counter_field = app_inst.get("OsAppCounterRef", "")
                if isinstance(counter_field, dict):
                    counter_ref = counter_field.get("ValueRef", "")
                else:
                    counter_ref = counter_field
                app_counter_refs = self.get_ref_list(counter_ref)
                if counter_name in app_counter_refs:
                    return app_key
        return None

    def find_application_by_schedule_table(self, schedule_table_name, ecuc_os_dict):
        """
        根据ScheduleTable名查找其所属的OsApplication名
        """
        for app_key, app_inst in ecuc_os_dict.items():
            if not isinstance(app_inst, dict):
                continue
            if "/Os/OsApplication" in app_inst.get("DefinitionRef", ""):
                st_field = app_inst.get("OsAppScheduleTableRef", "")
                # 兼容dict、str、list
                if isinstance(st_field, dict):
                    st_ref = st_field.get("ValueRef", "")
                else:
                    st_ref = st_field
                st_refs = self.get_ref_list(st_ref)
                if schedule_table_name in st_refs:
                    return app_key
        return None

    @staticmethod
    def get_ref_value(ref):
        if isinstance(ref, dict):
            return ref.get("ValueRef", "")
        elif isinstance(ref, str):
            return ref
        else:
            return ""