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

import functools
from collections import defaultdict

def get_last_item(x):
    """string.split('/') ,return the last part; if string=<empty>,return '' """
    if x is None or x == "<empty>":
        return ""
    else:
        return x.split('/')[-1]

def sort_dict_by_key(in_dict):
    return {k:v for k, v in sorted(in_dict.items())}

def get_dict_path(in_dict, path_list):
    res = in_dict
    for path in path_list:
        res = res.get(path, {})
        if not res:
            break
    return res

def recursive_group(data, keys):
    if not keys:
        return data

    key = keys[0]
    grouped = defaultdict(list)

    for d in data:
        grouped[d[key]].append(d)

    for k, v in grouped.items():
        grouped[k] = recursive_group(v, keys[1:])

    return grouped

def resolve_ref_list(ref_type, cfg_dict):
    ref_list = list()
    # When only one reference is configurated,
    # the ValueRef is an instance of str instead of list.
    if isinstance(cfg_dict[ref_type]["ValueRef"], str):
        ref_list.append(get_last_item(
            cfg_dict[ref_type]["ValueRef"]))
    elif isinstance(cfg_dict[ref_type]["ValueRef"], list):
        for item_dir in cfg_dict[ref_type]["ValueRef"]:
            ref_list.append(get_last_item(item_dir))
    return sorted(list(set(ref_list)), key=lambda x: (len(x), x.lower()))

def get_module_list(context, module_path):
    mod_list = {}
    for name, value in context.items():
        if "DefinitionRef" in value and module_path in value["DefinitionRef"]:
            mod_list[name] = value
    return mod_list

def cache_once(cache_registry):

    def decorator(func):
        cached_result = None
        cached_called = False

        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            nonlocal cached_called, cached_result
            if not cached_called:
                cached_result = func(*args, **kwargs)
                cached_called = True
            return cached_result

        def clear_cache():
            nonlocal cached_called, cached_result
            cached_result = None
            cached_called = False

        wrapper.clear_cache = clear_cache
        cache_registry.append(wrapper)
        return wrapper
    return decorator
