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
"""Load a project through VCOS Studio's CLI API, validate and generate modules.

Runs in its own process because Studio keeps global state.
usage: studio_runner.py <studio configurator dir> <project.vcosproject> <Module> [...]
Prints one JSON line prefixed with RESULT_JSON=.
"""
import json
import os
import sys


def main():
    studio, project, *modules = sys.argv[1:]
    sys.path.insert(0, studio)
    sys.path.insert(0, os.path.join(studio, "third_party"))
    os.chdir(studio)

    from cmd_api.auto_cli_api import AutoCliApi
    from func_module.health_monitor.logger import logger

    messages = []
    logger.add_handler(lambda msg: messages.append(str(msg)), level="ERROR", format="{message}")

    result = {"load": AutoCliApi.load_project(os.path.abspath(project))}
    if result["load"]:
        validation, logs = AutoCliApi.excute_validation(modules, "error")
        result["validate"] = validation
        result["failures"] = [
            {"module": module, "mark": mark, "message": entry[0], "path": entry[1], "fix": entry[4]}
            for module, module_logs in (logs or {}).items()
            for mark, block in module_logs.get("logs", {}).items()
            for entry in block.get("info", [])
            if "fail" in entry
        ]
        result["generate"] = bool(AutoCliApi.excute_generation(modules))
    result["errors"] = messages
    print("RESULT_JSON=" + json.dumps(result, default=str))


if __name__ == "__main__":
    main()
