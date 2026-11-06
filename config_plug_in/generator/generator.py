############################################################################
#  Copyright (c) 2025 Li Auto Inc. and its affiliates
#  Licensed under the Apache License, Version 2.0(the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at

#      http://www.apache.org/licenses/LICENSE-2.0

#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
############################################################################

import os
import shutil

from func_module.health_monitor.logger import logger
from jinja2 import Environment, FileSystemLoader, StrictUndefined
from .utils import get_last_item, get_dict_path

from . import context

CUR_FILE_PATH = os.path.dirname(__file__)
DEFAULT_TEMPLATE_FILE_DIR = os.path.join(CUR_FILE_PATH, "template")

def get_os_core_info(ecuc_dict: dict):
    ret = {}

    for k, v in ecuc_dict["EcucHardware"].items():
        if "EcucCoreDefinition" not in k:
            continue
        coreid = str(v['EcucCoreId']['Value'])
        hw_coreid = str(get_last_item(v['EcucCoreHwRef']['ValueRef'])[-1])
        ret['Os_Core' + coreid] = {
            'core_id': 'OS_CORE_ID_' + coreid,
            'PhyId': 'OS_CORE_PHYSICAL_ID_' + hw_coreid
        }

    return ret

def render_os_template(template, out_dir, file_name, config):
    if template is not None:
        file_path = os.path.join(out_dir, file_name)
        with open(file_path, "w", encoding="utf-8") as out_stream:
            out_stream.write(template.render(config))
            out_stream.flush()
        logger.info("Finish generating %s file" % (file_name))

def render_os_callout_template(template, out_dir, file_name, config):
    if template is not None:
        file_path = os.path.join(out_dir, file_name)
        if os.path.exists(file_path):
            backup_file_path = file_path + '.bak'
            shutil.copy(file_path, backup_file_path)
        render_output = template.render(config)
        with open(file_path, "w", encoding="utf-8") as out_stream:
            out_stream.write(render_output)
            out_stream.flush()
        logger.info("Finish generating %s file" % (file_name))


def prepare_os_base_config(sys_config:dict, config_dict:dict):
    # get EcuC Configs
    ecuc_dict = get_dict_path(config_dict, ['bsw', 'ecuc', 'ArPackages', 'ActiveEcuC', 'EcuC'])
    # get core information and hw coreid information from UI input and add to os dict
    core_dict = get_os_core_info(ecuc_dict)

    # save the configuration of ui
    input_ui_dict = get_dict_path(config_dict, ['bsw', 'os', 'ArPackages', 'ActiveEcuC', 'Os'])

    config = {
        "os_ui": input_ui_dict,
        "sys_cfg": sys_config,
        "os_core": core_dict,
        "config_dict": config_dict,
        "ioc_data_dict": config_dict.get("rte",{}).get("implementation_datatypes",{})
    }
    return config


def generate_dynamic_code(sys_config: dict, config_dict: dict):
    context.Function.clear_caches()

    output_dir = os.path.realpath(
        os.path.join(sys_config["gendata_path"], "Os"))
    callout_dir = get_dict_path(sys_config, ['path', 'bswcallout_path'])
    if not os.path.exists(callout_dir):
        os.makedirs(callout_dir)

    config = prepare_os_base_config(sys_config, config_dict)

    # Build our rendering environment.
    env_ui = Environment(loader=FileSystemLoader(DEFAULT_TEMPLATE_FILE_DIR),
                         undefined=StrictUndefined)
    env_ui.filters.update(context.get_filters())
    env_ui.globals.update(context.get_functions())

    # ################################################## #
    #    TASK                                            #
    # ################################################## #
    template = env_ui.get_template("task_cfg.h.jinja2")
    render_os_template(template, output_dir, "task_cfg.h", config)

    template = env_ui.get_template("task_cfg.c.jinja2")
    render_os_template(template, output_dir, "task_cfg.c", config)

    # ################################################## #
    #    STACK                                           #
    # ################################################## #
    template = env_ui.get_template("stack_cfg.h.jinja2")
    render_os_template(template, output_dir, "stack_cfg.h", config)

    # ################################################## #
    #    APPLICATION                                     #
    # ################################################## #
    template = env_ui.get_template("application_cfg.h.jinja2")
    render_os_template(template, output_dir, "application_cfg.h", config)

    template = env_ui.get_template("application_cfg.c.jinja2")
    render_os_template(template, output_dir, "application_cfg.c", config)

    # ################################################## #
    #    CORE                                            #
    # ################################################## #
    template = env_ui.get_template("core_cfg.c.jinja2")
    render_os_template(template, output_dir, "core_cfg.c", config)

    template = env_ui.get_template("core_cfg.h.jinja2")
    render_os_template(template, output_dir, "core_cfg.h", config)

    # ################################################## #
    #    IpiCall                                         #
    # ################################################## #
    template = env_ui.get_template("ipicall_cfg.c.jinja2")
    render_os_template(template, output_dir, "ipicall_cfg.c", config)

    template = env_ui.get_template("ipicall_cfg.h.jinja2")
    render_os_template(template, output_dir, "ipicall_cfg.h", config)

    # ################################################## #
    #    ISR                                             #
    # ################################################## #
    template = env_ui.get_template("isr_cfg.c.jinja2")
    render_os_template(template, output_dir, "isr_cfg.c", config)

    if sys_config['target'] == "AURIX2G_TC397":
        template = env_ui.get_template("tricore_irq_dispatch.c.jinja2")
        render_os_template(template, output_dir, "tricore_irq_dispatch.c", config)

    template = env_ui.get_template("isr_cfg.h.jinja2")
    render_os_template(template, output_dir, "isr_cfg.h", config)

    # ################################################## #
    #    IOC                                             #
    # ################################################## #
    template = env_ui.get_template("ioc.c.jinja2")
    render_os_template(template, output_dir, "ioc.c", config)

    template = env_ui.get_template("ioc.h.jinja2")
    render_os_template(template, output_dir, "ioc.h", config)

    template = env_ui.get_template("ioc_cfg.c.jinja2")
    render_os_template(template, output_dir, "ioc_cfg.c", config)

    template = env_ui.get_template("ioc_cfg.h.jinja2")
    render_os_template(template, output_dir, "ioc_cfg.h", config)

    # ################################################## #
    #    SPINLOCK                                        #
    # ################################################## #
    template = env_ui.get_template("spinlock_cfg.h.jinja2")
    render_os_template(template, output_dir, "spinlock_cfg.h", config)

    template = env_ui.get_template("spinlock_cfg.c.jinja2")
    render_os_template(template, output_dir, "spinlock_cfg.c", config)

    # ################################################## #
    #    EVENT                                           #
    # ################################################## #
    template = env_ui.get_template("event_cfg.h.jinja2")
    render_os_template(template, output_dir, "event_cfg.h", config)

    # ################################################## #
    #    SCHEDTABLE                                      #
    # ################################################## #
    template = env_ui.get_template("schedtable_cfg.h.jinja2")
    render_os_template(template, output_dir, "schedtable_cfg.h", config)

    template = env_ui.get_template("schedtable_cfg.c.jinja2")
    render_os_template(template, output_dir, "schedtable_cfg.c", config)

    # ################################################## #
    #    RESOURCE                                        #
    # ################################################## #
    template = env_ui.get_template("resource_cfg.h.jinja2")
    render_os_template(template, output_dir, "resource_cfg.h", config)

    template = env_ui.get_template("resource_cfg.c.jinja2")
    render_os_template(template, output_dir, "resource_cfg.c", config)

    # ################################################## #
    #    COUNTER                                         #
    # ################################################## #
    template = env_ui.get_template("counter_cfg.h.jinja2")
    render_os_template(template, output_dir, "counter_cfg.h", config)

    template = env_ui.get_template("counter_cfg.c.jinja2")
    render_os_template(template, output_dir, "counter_cfg.c", config)

    # ################################################## #
    #    MEMORY PROTECTION                               #
    # ################################################## #
    template = env_ui.get_template("memory_protection_cfg.h.jinja2")
    render_os_template(template, output_dir, "memory_protection_cfg.h", config)

    template = env_ui.get_template("memory_protection_cfg.c.jinja2")
    render_os_template(template, output_dir, "memory_protection_cfg.c", config)

    # ################################################## #
    #    Hook                                            #
    # ################################################## #
    template = env_ui.get_template("hook_user_callout.c.jinja2")
    render_os_callout_template(template, callout_dir, "hook_user_callout.c", config)

    template = env_ui.get_template("hook_cfg.c.jinja2")
    render_os_template(template, output_dir, "hook_cfg.c", config)

    template = env_ui.get_template("hook_cfg.h.jinja2")
    render_os_template(template, output_dir, "hook_cfg.h", config)

    # ################################################## #
    #    rt_framework.config                             #
    # ################################################## #
    template = env_ui.get_template("rt_framework.config.jinja2")
    render_os_template(template, output_dir, "rt_framework.config", config)

    return True
