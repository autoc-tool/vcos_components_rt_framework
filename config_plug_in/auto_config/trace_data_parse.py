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
"""
Ftrace Log Parser Module

This module parses ftrace log files to extract task timing information:
- Task names and priorities
- Task execution times (max/avg/std_dev)
- Task periods
- Other task metrics
"""

import json
import math
import os
import re
import sys
import traceback
from pathlib import Path
from typing import Dict, Any, Optional, List, Tuple
from datetime import datetime
from PySide6.QtWidgets import QApplication, QDialog, QVBoxLayout, QHBoxLayout, QRadioButton, QLabel, QPushButton, QMessageBox, QButtonGroup, QWidget, QFileDialog, QLineEdit
from PySide6.QtCore import Qt

# Import matplotlib for plotting
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import MaxNLocator

# Add ftrace parser to path
base_directory = os.path.dirname(os.path.abspath(__file__))
ftrace_parser_directory = os.path.abspath(os.path.join(
    base_directory, '..', '..', '..', '..', '..', 'tools', 'halo_trace'))
sys.path.append(ftrace_parser_directory)
from ftrace_manager import FtraceParser
from trace_util import TraceComputeUtils

class FtraceLogParser:
    """
    Parser for ftrace log files containing task timing information.

    Extracts task execution metrics including:
    - Task names and priorities
    - Execution times (max/avg/std_dev)
    - Task periods
    """
    def __init__(self, ftrace_file: str):
        self.ftrace_file = ftrace_file
        self.input_task_dict = {}
        self.core_pattern = re.compile(r'.*Core\d+.*')
        self.priority_cache = {}

    def collect_task_names(self) -> None:
        """Collect all task names from ftrace file that contain core identifiers"""
        try:
            with open(self.ftrace_file, 'r', encoding='utf-8') as f:
                for line in f:
                    if 'sched_switch' in line:
                        for match in re.finditer(r'(?:prev|next)_comm=([^\s]+)', line):
                            task_name = match.group(1)
                            if self.core_pattern.match(task_name):
                                self.input_task_dict[task_name] = [None, None]
                                if task_name not in self.priority_cache:
                                    if f'prev_comm={task_name}' in line:
                                        prio_match = re.search(r'prev_prio=(\d+)', line)
                                        if prio_match:
                                            self.priority_cache[task_name] = int(
                                                prio_match.group(1))
                                    elif f'next_comm={task_name}' in line:
                                        prio_match = re.search(r'next_prio=(\d+)', line)
                                        if prio_match:
                                            self.priority_cache[task_name] = int(
                                                prio_match.group(1))
        except Exception as e:
            print(f"Error collecting task names: {str(e)}")
            raise

    def get_task_priority(self, task_name: str) -> int:
        """
        Get task priority from cache or ftrace file

        Args:
            task_name: Name of the task to find priority for

        Returns:
            Task priority as integer or -1 if not found
        """
        if task_name in self.priority_cache:
            return self.priority_cache[task_name]

        try:
            with open(self.ftrace_file, 'r', encoding='utf-8') as f:
                for line in f:
                    if task_name in line and 'sched_switch' in line:
                        for prefix, prio_prefix in [
                            ('prev_comm=', 'prev_prio='),
                            ('next_comm=', 'next_prio=')
                        ]:
                            if f'{prefix}{task_name}' in line:
                                prio_match = re.search(f'{prio_prefix}(\\d+)', line)
                                if prio_match:
                                    priority = int(prio_match.group(1))
                                    self.priority_cache[task_name] = priority
                                    return priority
        except Exception as e:
            print(f"Error getting priority for task {task_name}: {str(e)}")

        return -1

    def get_task_parameters(self) -> Dict[str, Dict[str, Any]]:
        """
        Extract task parameters from ftrace file

        Returns:
            Dictionary mapping task names to their parameters
        """
        try:
            self.collect_task_names()

            if not self.input_task_dict:
                print("Warning: No valid tasks found in ftrace file")
                return {}

            success, task_info = FtraceParser.get_ftrace_task_info(
                self.ftrace_file, self.input_task_dict)

            if not success or not task_info:
                print("Failed to parse ftrace file or no task info found")
                return {}

            task_params = {}
            for task_name, params in task_info.items():
                try:
                    if not self.core_pattern.search(task_name):
                        continue

                    priority = self.get_task_priority(task_name)
                    period = round(params.get("dt_avg", 0) * 1e6) if params.get("dt_avg", 0) > 0 else 0
                    cet_max = round(params.get("cet_max", 0) * 1e6)
                    cet_avg = round(params.get("cet_avg", 0) * 1e6)
                    cet_std_dev = round(params.get("cet_std_dev", 0) * 1e6)

                    # Get PCET list and convert units to microseconds
                    pcet = params.get("PCET", [])
                    if pcet:
                        pcet = [round(x) for x in pcet]  # Convert to microseconds and round

                    # Get DT list (deadline time samples)
                    dt_list = params.get("dt_list", [])
                    if dt_list:
                        dt_list = [round(x * 1e6) for x in dt_list]  # Convert to microseconds and round

                    # Get CET list (execution time samples)
                    cet_list = params.get("cet_list", [])
                    if cet_list:
                        cet_list = [round(x * 1e6) for x in cet_list]  # Convert to microseconds and round

                    if period > 0 and (cet_max > 0 or cet_avg > 0):
                        # Calculate jitter: |DT max - Period|
                        jitter = 0
                        if dt_list:
                            dt_max = max(dt_list)
                            jitter = abs(dt_max - period) if period > 0 else 0

                        task_params[task_name] = {
                            'priority': priority,
                            'period': period,
                            'jitter': jitter,
                            'cet_max': cet_max,
                            'cet_avg': cet_avg,
                            'cet_std_dev': cet_std_dev,
                            'PCET': pcet,
                            'dt_list': dt_list,
                            'cet_list': cet_list
                        }
                except Exception as e:
                    print(f"Error processing task {task_name}")
                    continue

            return task_params

        except Exception as e:
            print(f"Error in get_task_parameters: {str(e)}")
            raise


def parse_ftrace_log(ftrace_file: str) -> Optional[Dict[str, Dict[str, Any]]]:
    """
    Parse ftrace log file and extract task information

    Args:
        ftrace_file: Path to ftrace log file

    Returns:
        Dictionary of task parameters or None if parsing fails
    """
    try:
        # Normalize file path
        ftrace_path = Path(ftrace_file).resolve()
        if not ftrace_path.exists():
            print(f"Error: Ftrace file not found: {ftrace_path}")
            return None
        if not ftrace_path.is_file():
            print(f"Error: Path is not a file: {ftrace_path}")
            return None

        # Check if file is readable
        if not os.access(ftrace_path, os.R_OK):
            print(f"Error: No read permission for file: {ftrace_path}")
            return None

        parser = FtraceLogParser(str(ftrace_path))
        task_params = parser.get_task_parameters()

        # Validate returned data structure
        if not isinstance(task_params, dict):
            print(f"Error: Invalid task parameters format. "
                  f"Expected dict, got {type(task_params)}")
            return None
        if not task_params:
            print("Warning: No valid task parameters found in ftrace file")
            return None

        # Validate data content
        for task_name, params in task_params.items():
            if not isinstance(params, dict):
                print(f"Error: Invalid parameters format for task {task_name}")
                return None
            required_keys = {'priority', 'period', 'jitter', 'cet_max', 'cet_avg', 'cet_std_dev', 'PCET'}
            if not all(key in params for key in required_keys):
                print(f"Error: Missing required parameters for task {task_name}")
                return None

        return task_params

    except FileNotFoundError as e:
        print(f"File not found error: {str(e)}")
        return None
    except PermissionError as e:
        print(f"Permission error: {str(e)}")
        return None
    except json.JSONDecodeError as e:
        print(f"JSON parsing error: {str(e)}")
        return None
    except Exception as e:
        print(f"Unexpected error while parsing ftrace log: {str(e)}")
        print(f"Error type: {type(e).__name__}")
        traceback.print_exc()
        return None
class BinaryTraceParser:
    """
    Parser for binary trace data files containing task timing information.
    """
    def __init__(self, bin_file: str):
        self.bin_file = bin_file
        self.data_dir = os.path.dirname(os.path.abspath(bin_file))
        self.device_name = os.path.basename(bin_file).split('_data.bin')[0]
        self.config_file = os.path.join(self.data_dir, "config.json")
        self.priority_map = self._load_priority_map()

    def _load_priority_map(self):
        priority_map = {}
        try:
            with open(self.config_file, 'r', encoding='utf-8') as f:
                config_data = json.load(f)
            for tid, tinfo in config_data[self.device_name]["Task"].items():
                priority_map[tinfo[0]] = tinfo[1]
        except Exception:
            pass
        return priority_map

    def get_task_parameters(self) -> Dict[str, Dict[str, Any]]:
        """
        Extract task parameters from binary file using TraceComputeUtils.
        Returns:
            Dictionary mapping task names to their parameters (单位全部us)
        """
        if not os.path.exists(self.config_file):
            print(f"[ERROR] config.json not found in {self.data_dir}!")
            return {}

        input_task_dict = {}
        success, task_info = TraceComputeUtils.trace_get_task_statistics(self.data_dir, input_task_dict)
        if not (success and task_info):
            print("Binary parsing failed to extract task data")
            return {}

        if len(task_info) == 1 and self.device_name in task_info:
            task_info = task_info[self.device_name]

        task_params = {}
        for task_name, params in task_info.items():
            try:
                # 统一输出结构，全部转us
                period = round(params.get("dt_avg", 0) / 1000) if params.get("dt_avg", 0) > 0 else 0
                cet_max = round(params.get("cet_max", 0) / 1000)
                cet_avg = round(params.get("cet_avg", 0) / 1000)

                # 正确获取dt_list和cet_list
                dt_list = [round(x / 1000) for x in params.get("dt_list", [])]
                cet_list = [round(x / 1000) for x in params.get("cet_list", [])]

                # 计算CET标准差（单位us）
                if cet_list:
                    mean = sum(cet_list) / len(cet_list)
                    variance = sum((x - mean) ** 2 for x in cet_list) / len(cet_list)
                    cet_std_dev = round(variance ** 0.5)
                else:
                    cet_std_dev = 0

                pcet = []
                if cet_list:
                    pcet = [int(round(x)) for x in calculate_percentiles(sorted(cet_list), [99,98,97,96,95,94,93,92,91,90])]

                priority = self.priority_map.get(task_name, -1)
                if period > 0 or cet_max > 0 or cet_avg > 0:
                    # Calculate jitter: |DT max - Period|
                    jitter = 0
                    if dt_list:
                        dt_max = max(dt_list)
                        jitter = abs(dt_max - period) if period > 0 else 0

                    task_params[task_name] = {
                        'priority': priority,
                        'period': period,
                        'jitter': jitter,
                        'cet_max': cet_max,
                        'cet_avg': cet_avg,
                        'cet_std_dev': cet_std_dev,
                        'PCET': pcet,
                        'dt_list': dt_list,
                        'cet_list': cet_list
                    }
            except Exception as e:
                continue

        return task_params

def parse_binary_data(bin_file: str) -> Optional[Dict[str, Dict[str, Any]]]:
    """
    Parse binary trace data file and extract task information
    """
    parser = BinaryTraceParser(bin_file)
    return parser.get_task_parameters()

def calculate_percentiles(data, percentiles):
    """Calculate the specified percentiles of a list of numbers."""
    data.sort()
    results = []
    for percentile in percentiles:
        k = (len(data) - 1) * (percentile / 100)
        f = math.floor(k)
        c = math.ceil(k)
        if f == c:
            results.append(data[int(k)])
        else:
            d0 = data[int(f)] * (c - k)
            d1 = data[int(c)] * (k - f)
            results.append(d0 + d1)
    return results

def print_formatted_output(task_params: Dict[str, Dict[str, Any]], plot_type: str = None, show_jitter: bool = False) -> None:
    """
    Print formatted task information with improved layout

    Args:
        task_params: Dictionary of task parameters
        plot_type: Type of samples to display (dt or cet)
        show_jitter: Whether to show Jitter column and hide PCET column
    """
    # Calculate the maximum length needed for PCET values only if not showing jitter
    max_pcet_length = 0
    if not show_jitter:
        for params in task_params.values():
            pcet_str = f"[{', '.join(str(int(x)) for x in params['PCET'])}]"
            max_pcet_length = max(max_pcet_length, len(pcet_str))

    print("\nTask Parameters By Core:")

    # Adjust separator line length based on whether showing jitter or PCET
    if show_jitter:
        print("=" * 150)
        # Format string for jitter mode (no PCET column)
        header_format = (
            f"{'Task Name':<40} {'Priority':<10} {'Period(us)':<12} {'Jitter(us)':<12} {'CET Max(us)':<12} "
            f"{'CET Avg(us)':<12} {'CET StdDev(us)':<12} | {'Samples':<8}"
        )
    else:
        print("=" * 160)
        # Format string for normal mode (with PCET, no jitter)
        header_format = (
            f"{'Task Name':<40} {'Priority':<10} {'Period(us)':<12} {'CET Max(us)':<12} "
            f"{'CET Avg(us)':<12} {'CET StdDev(us)':<12} {'PCET(us)':<{max_pcet_length}} | {'Samples':<8}"
        )

    # Group tasks by core
    cores_dict = {}
    for task_name, params in task_params.items():
        core_match = re.search(r'Core(\d+)', task_name)
        if core_match:
            core_name = f"Core{core_match.group()}"
            if core_name not in cores_dict:
                cores_dict[core_name] = []
            cores_dict[core_name].append((task_name, params))

    # Print tasks by core
    for core_name, tasks in sorted(cores_dict.items()):
        print(f"\n{core_name}:")
        if show_jitter:
            print("-" * 150)  # Shorter line for jitter mode
        else:
            print("-" * 160)  # Longer line for normal mode
        print(header_format)
        if show_jitter:
            print("-" * 150)
        else:
            print("-" * 160)

        for task_name, params in sorted(tasks, key=lambda x: x[1]['priority'], reverse=True):
            # Get sample count based on plot_type
            # Only show DT samples if plot_type is explicitly 'dt'
            # Otherwise default to CET samples
            if plot_type == 'dt':
                samples_count = len(params.get('dt_list', []))
            else:  # Default to CET samples for no argument or cet argument
                samples_count = len(params.get('cet_list', []))

            samples_str = f"{samples_count:>8}"  # Right-align sample count without "samples" text

            if show_jitter:
                # Jitter mode: show jitter column, hide PCET column
                jitter = params.get('jitter', 0)
                print(
                    f"{task_name:<40} {params['priority']:<10} {params['period']:<12} "
                    f"{jitter:<12} {params['cet_max']:<12} {params['cet_avg']:<12} {params['cet_std_dev']:<12} "
                    f"| {samples_str}"
                )
            else:
                # Normal mode: show PCET column, hide jitter column
                pcet_str = f"[{', '.join(str(int(x)) for x in params['PCET'])}]"
                pcet_str = f"{pcet_str:<{max_pcet_length}}"  # Fixed PCET column width
                print(
                    f"{task_name:<40} {params['priority']:<10} {params['period']:<12} "
                    f"{params['cet_max']:<12} {params['cet_avg']:<12} {params['cet_std_dev']:<12} "
                    f"{pcet_str} | {samples_str}"
                )

        if show_jitter:
            print("-" * 150)
        else:
            print("-" * 160)


def organize_tasks_by_core(
    task_params: Dict[str, Dict[str, Any]]
) -> Dict[str, Dict[str, Dict[str, Any]]]:
    """
    Organize task parameters by core and sort tasks by priority (high to low)

    Args:
        task_params: Raw task parameters dictionary

    Returns:
        Dictionary organized by core with tasks sorted by priority:
        {
            "Core0": {
                "Task1": {
                    "priority": int,
                    "period": int,
                    "cet_max": int,
                    "cet_avg": int,
                    "cet_std_dev": int,
                    "dt_list": [...]
                },
                ...
            },
            "Core1": {
                ...
            },
            ...
        }
    """
    cores_dict = {}
    # Regular expression to match core identifiers like "Core0", "Core1", etc.
    core_pattern = re.compile(r"Core\d+")

    # First, organize tasks by core
    for task_name, params in task_params.items():
        # Search for core identifier in the task name
        core_match = core_pattern.search(task_name)
        if core_match:
            core_name = core_match.group()
        else:
            # If no core identifier is found, skip the task
            print(f"Warning: No core identifier found in task name '{task_name}'. "
                  f"Skipping task.")
            continue

        if core_name not in cores_dict:
            cores_dict[core_name] = {}
        cores_dict[core_name][task_name] = params

    # Then, sort tasks in each core by priority (high to low)
    sorted_cores_dict = {}
    for core_name, core_tasks in cores_dict.items():
        # Sort tasks by priority (high to low)
        sorted_tasks = dict(sorted(
            core_tasks.items(),
            key=lambda x: x[1]['priority'],
            reverse=True  # High to low
        ))
        sorted_cores_dict[core_name] = sorted_tasks

    return sorted_cores_dict


def save_cores_dict_to_json(
    cores_dict: Dict[str, Dict[str, Dict[str, Any]]],
    file_path: str
) -> None:
    """
    Save cores dictionary to a JSON file

    Args:
        cores_dict: Dictionary containing core-wise task parameters
        file_path: Path to save the JSON file
    """
    try:
        # Ensure directory exists
        output_dir = Path(file_path).parent
        output_dir.mkdir(parents=True, exist_ok=True)

        # Write to JSON file with pretty formatting
        with open(file_path, 'w', encoding='utf-8') as f:
            json.dump(cores_dict, f, indent=4, sort_keys=False)
        print(f"\nSuccessfully saved cores dictionary to: {file_path}")
    except Exception as e:
        print(f"Error saving JSON file: {str(e)}")


def plot_dt_samples(
    task_name: str,
    dt_list: List[float],
    output_dir: str,
    period: int = 0,
    fixed_y_range: bool = True
) -> None:
    """
    Plot deadline time samples for a task and save the graph

    Args:
        task_name: Name of the task
        dt_list: List of deadline time samples (in microseconds)
        output_dir: Directory to save the plot image
        period: Task period in microseconds (if available)
        fixed_y_range: If True, use a fixed Y-axis range based on period
    """
    if not dt_list:
        print(f"Warning: No DT samples available for task '{task_name}'")
        return

    try:
        # Create directory if it doesn't exist
        os.makedirs(output_dir, exist_ok=True)

        # Get statistics for better visualization
        min_dt = min(dt_list)
        max_dt = max(dt_list)
        mean_dt = np.mean(dt_list)
        std_dt = np.std(dt_list)

        # Create a figure with a specific layout to control legend placement
        fig, ax = plt.subplots(figsize=(12, 8), dpi=120)

        # Create x-axis (sample index)
        x = np.arange(len(dt_list))

        # Plot the main data
        ax.plot(x, dt_list, color='#1f77b4', linewidth=0.8, alpha=0.7)

        # Don't plot points for large datasets to reduce visual clutter
        if len(dt_list) < 1000:
            ax.scatter(x, dt_list, color='#ff7f0e', s=10, alpha=0.7)

        # Add reference lines
        ax.axhline(y=mean_dt, color='r', linestyle='-', linewidth=1.5,
                  label=f'Mean: {mean_dt:.2f} μs')
        ax.axhline(y=mean_dt + std_dt, color='g', linestyle='--', linewidth=1,
                  label=f'Mean + StdDev: {mean_dt + std_dt:.2f} μs')
        ax.axhline(y=mean_dt - std_dt, color='g', linestyle='--', linewidth=1,
                  label=f'Mean - StdDev: {mean_dt - std_dt:.2f} μs')

        # Add period reference line if provided
        if period > 0:
            ax.axhline(y=period, color='purple', linestyle='-.', linewidth=1.5,
                      label=f'Period: {period} μs')

        # Add grid with better styling
        ax.grid(True, linestyle='--', alpha=0.3)

        # Set labels and title with better font
        ax.set_xlabel('Sample Index', fontsize=12, fontweight='bold')
        ax.set_ylabel('Deadline Time (μs)', fontsize=12, fontweight='bold')
        ax.set_title(f'Deadline Time Samples for {task_name}', fontsize=14, fontweight='bold')

        # Create legend in the upper left
        legend1 = ax.legend(
            loc='upper left',
            frameon=True,
            fancybox=True,
            shadow=True
        )

        # 纵轴严格以Mean为中心，上下对称，上下偏移 4 * std_dt
        y_lower = mean_dt - 4 * std_dt
        y_upper = mean_dt + 4 * std_dt
        ax.set_ylim(y_lower, y_upper)

        ax.axhline(y=y_lower, color='gray', linestyle=':', linewidth=0.8)
        ax.axhline(y=y_upper, color='gray', linestyle=':', linewidth=0.8)

        # For large datasets, use fewer x-axis labels to avoid crowding
        if len(dt_list) > 1000:
            ax.xaxis.set_major_locator(MaxNLocator(integer=True, nbins=10))
        else:
            ax.xaxis.set_major_locator(MaxNLocator(integer=True))

        # Create statistics text box
        stats_text = (
            f"Statistics:\n"
            f"Samples: {len(dt_list)}\n"
            f"Min: {min_dt:.2f} μs\n"
            f"Max: {max_dt:.2f} μs\n"
            f"Mean: {mean_dt:.2f} μs\n"
            f"StdDev: {std_dt:.2f} μs"
        )

        # Add annotation box in the upper right corner
        ax.annotate(
            stats_text,
            xy=(0.99, 0.99),
            xycoords='axes fraction',
            fontsize=10,
            bbox=dict(boxstyle="round,pad=0.5", facecolor='white', alpha=0.8),
            ha='right',
            va='top'
        )

        # Adjust layout to make room for all elements
        fig.tight_layout()

        # Save the figure with higher quality
        safe_task_name = re.sub(r'[^\w\-_.]', '_', task_name)  # Replace invalid characters
        file_path = os.path.join(output_dir, f"{safe_task_name}_DT_Samples.png")
        fig.savefig(file_path, dpi=300, bbox_inches='tight')
        plt.close(fig)

        print(f"Created DT samples plot for '{task_name}' at: {file_path}")

    except Exception as e:
        print(f"Error creating plot for task '{task_name}': {str(e)}")
        traceback.print_exc()


def plot_all_task_dt_samples(
    task_params: Dict[str, Dict[str, Any]],
    output_dir: str
) -> None:
    """
    Plot deadline time samples for all tasks and save the graphs

    Args:
        task_params: Dictionary of task parameters
        output_dir: Directory to save the plot images
    """
    print("\nGenerating DT sample plots for all tasks...")

    # Track the number of plots created
    plot_count = 0

    for task_name, params in task_params.items():
        dt_list = params.get('dt_list', [])
        period = params.get('period', 0)
        if dt_list:
            plot_dt_samples(task_name, dt_list, output_dir, period)
            plot_count += 1

    if plot_count > 0:
        print(f"\nSuccessfully created {plot_count} DT sample plots")
    else:
        print("\nNo DT sample plots were created. No tasks have DT samples.")


def plot_cet_samples(
    task_name: str,
    cet_list: List[float],
    output_dir: str,
    fixed_y_range: bool = True
) -> None:
    """
    Plot execution time samples for a task and save the graph

    Args:
        task_name: Name of the task
        cet_list: List of execution time samples (in microseconds)
        output_dir: Directory to save the plot image
        fixed_y_range: If True, use a fixed Y-axis range based on data
    """
    if not cet_list:
        print(f"Warning: No CET samples available for task '{task_name}'")
        return

    try:
        # Create directory if it doesn't exist
        os.makedirs(output_dir, exist_ok=True)

        # Get statistics for better visualization
        min_cet = min(cet_list)
        max_cet = max(cet_list)
        mean_cet = np.mean(cet_list)
        std_cet = np.std(cet_list)

        # Create a figure with a specific layout to control legend placement
        fig, ax = plt.subplots(figsize=(12, 8), dpi=120)

        # Create x-axis (sample index)
        x = np.arange(len(cet_list))

        # Plot the main data
        ax.plot(x, cet_list, color='#1f77b4', linewidth=0.8, alpha=0.7)

        # Don't plot points for large datasets to reduce visual clutter
        if len(cet_list) < 1000:
            ax.scatter(x, cet_list, color='#ff7f0e', s=10, alpha=0.7)

        # Add reference lines
        ax.axhline(y=mean_cet, color='r', linestyle='-', linewidth=1.5,
                  label=f'Mean: {mean_cet:.2f} μs')
        ax.axhline(y=mean_cet + std_cet, color='g', linestyle='--', linewidth=1,
                  label=f'Mean + StdDev: {mean_cet + std_cet:.2f} μs')
        ax.axhline(y=mean_cet - std_cet, color='g', linestyle='--', linewidth=1,
                  label=f'Mean - StdDev: {mean_cet - std_cet:.2f} μs')

        # Add grid with better styling
        ax.grid(True, linestyle='--', alpha=0.3)

        # Set labels and title with better font
        ax.set_xlabel('Sample Index', fontsize=12, fontweight='bold')
        ax.set_ylabel('Execution Time (μs)', fontsize=12, fontweight='bold')
        ax.set_title(f'Execution Time Samples for {task_name}', fontsize=14, fontweight='bold')

        # Create legend in the upper left
        legend1 = ax.legend(
            loc='upper left',
            frameon=True,
            fancybox=True,
            shadow=True
        )

        y_lower = mean_cet - 4 * std_cet
        y_upper = mean_cet + 4 * std_cet
        ax.set_ylim(y_lower, y_upper)

        ax.axhline(y=y_lower, color='gray', linestyle=':', linewidth=0.8)
        ax.axhline(y=y_upper, color='gray', linestyle=':', linewidth=0.8)

        # For large datasets, use fewer x-axis labels to avoid crowding
        if len(cet_list) > 1000:
            ax.xaxis.set_major_locator(MaxNLocator(integer=True, nbins=10))
        else:
            ax.xaxis.set_major_locator(MaxNLocator(integer=True))

        # Create statistics text box
        stats_text = (
            f"Statistics:\n"
            f"Samples: {len(cet_list)}\n"
            f"Min: {min_cet:.2f} μs\n"
            f"Max: {max_cet:.2f} μs\n"
            f"Mean: {mean_cet:.2f} μs\n"
            f"StdDev: {std_cet:.2f} μs"
        )

        # Add annotation box in the upper right corner
        ax.annotate(
            stats_text,
            xy=(0.99, 0.99),
            xycoords='axes fraction',
            fontsize=10,
            bbox=dict(boxstyle="round,pad=0.5", facecolor='white', alpha=0.8),
            ha='right',
            va='top'
        )

        # Adjust layout to make room for all elements
        fig.tight_layout()

        # Save the figure with higher quality
        safe_task_name = re.sub(r'[^\w\-_.]', '_', task_name)  # Replace invalid characters
        file_path = os.path.join(output_dir, f"{safe_task_name}_CET_Samples.png")
        fig.savefig(file_path, dpi=300, bbox_inches='tight')
        plt.close(fig)

        print(f"Created CET samples plot for '{task_name}' at: {file_path}")

    except Exception as e:
        print(f"Error creating CET plot for task '{task_name}': {str(e)}")
        traceback.print_exc()


def plot_all_task_cet_samples(
    task_params: Dict[str, Dict[str, Any]],
    output_dir: str
) -> None:
    """
    Plot execution time samples for all tasks and save the graphs

    Args:
        task_params: Dictionary of task parameters
        output_dir: Directory to save the plot images
    """
    print("\nGenerating CET sample plots for all tasks...")

    # Track the number of plots created
    plot_count = 0

    for task_name, params in task_params.items():
        cet_list = params.get('cet_list', [])
        if cet_list:
            plot_cet_samples(task_name, cet_list, output_dir)
            plot_count += 1

    if plot_count > 0:
        print(f"\nSuccessfully created {plot_count} CET sample plots")
    else:
        print("\nNo CET sample plots were created. No tasks have CET samples.")


def select_log_file():
    """
    弹窗选择trace文件路径，带有提示和路径栏
    """
    app = QApplication.instance() or QApplication([])
    dialog = QDialog()
    dialog.setWindowTitle("Select Trace File")
    dialog.resize(750, 180)
    dialog.setMinimumWidth(700)

    layout = QVBoxLayout(dialog)
    label = QLabel("Please select the Trace file (.log or .bin):")
    label.setStyleSheet("font-size: 15px; font-weight: bold; margin-bottom: 10px;")
    layout.addWidget(label)

    # 路径输入栏和选择按钮
    path_layout = QHBoxLayout()
    path_edit = QLineEdit()
    path_edit.setPlaceholderText("Click 'Browse...' to select a log or binary file")
    path_edit.setMinimumHeight(28)
    path_edit.setStyleSheet("font-size: 13px;")
    browse_btn = QPushButton("Browse...")
    browse_btn.setMinimumHeight(28)
    browse_btn.setFixedWidth(110)
    browse_btn.setStyleSheet("font-size: 13px; margin-left: 8px;")
    path_layout.addWidget(path_edit)
    path_layout.addWidget(browse_btn)
    layout.addLayout(path_layout)

    # OK/Cancel按钮
    btn_layout = QHBoxLayout()
    ok_btn = QPushButton("OK")
    cancel_btn = QPushButton("Cancel")
    ok_btn.setMinimumHeight(28)
    cancel_btn.setMinimumHeight(28)
    ok_btn.setStyleSheet("font-size: 13px; min-width: 80px;")
    cancel_btn.setStyleSheet("font-size: 13px; min-width: 80px; margin-left: 10px;")
    btn_layout.addStretch(1)
    btn_layout.addWidget(ok_btn)
    btn_layout.addWidget(cancel_btn)
    btn_layout.addStretch(1)
    layout.addSpacing(10)
    layout.addLayout(btn_layout)

    # 选择文件逻辑
    def on_browse():
        file_path, _ = QFileDialog.getOpenFileName(dialog, "Select Trace File", "", "Trace Files (*.log *.bin);;Log Files (*.log);;Binary Files (*.bin);;All Files (*)")
        if file_path:
            path_edit.setText(file_path)
    browse_btn.clicked.connect(on_browse)

    # OK/Cancel逻辑
    def on_ok():
        if path_edit.text():
            dialog.accept()
        else:
            path_edit.setFocus()
    ok_btn.clicked.connect(on_ok)
    cancel_btn.clicked.connect(dialog.reject)

    # 居中显示
    dialog.setLayout(layout)
    dialog.setWindowModality(Qt.ApplicationModal)
    dialog.move(
        app.primaryScreen().geometry().center() - dialog.rect().center()
    )

    result = dialog.exec()
    if result == QDialog.Accepted and path_edit.text():
        return path_edit.text()
    else:
        return None

def get_project_paths_from_log(log_path):
    """
    根据log文件路径推导出输出路径
    """
    # 假设log_path为 .../system_cfg/xxx/config/Import/XXX_ftrace.log 或 XXX_data.bin
    # base_dir = .../system_cfg/xxx
    config_dir = os.path.dirname(os.path.dirname(log_path))  # .../system_cfg/xxx/config
    base_dir = os.path.dirname(config_dir)                   # .../system_cfg/xxx

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    json_output = os.path.join(base_dir, "config", "Export", f"trace_task_info_{timestamp}.json")
    dt_plots_dir = os.path.join(base_dir, "config", "Export", f"DT_Sample_Plots_{timestamp}")
    cet_plots_dir = os.path.join(base_dir, "config", "Export", f"CET_Sample_Plots_{timestamp}")

    return log_path, json_output, dt_plots_dir, cet_plots_dir

# 文件存在性检查函数
def check_file_exists(file_path):
    """
    检查文件是否存在
    """
    return os.path.isfile(file_path)

if __name__ == '__main__':
    try:
        plot_type = None
        show_jitter = False

        if len(sys.argv) > 1:
            arg = sys.argv[1].lower()
            if arg == 'dt':
                plot_type = 'dt'
            elif arg == 'cet':
                plot_type = 'cet'
            elif arg == 'jitter':
                show_jitter = True
                # When showing jitter, we don't need to set plot_type for plotting
                # but we can set it for sample count display (default to CET samples)
                plot_type = None  # Don't set plot_type to avoid affecting sample count display

        app = QApplication.instance() or QApplication([])

        # 1. 弹窗选择trace文件（log或bin）
        trace_file = select_log_file()
        if not trace_file:
            print("No trace file selected. Exiting.")
            sys.exit(1)

        # 2. 推导输出路径
        TRACE_FILE, JSON_OUTPUT, DT_PLOTS_DIR, CET_PLOTS_DIR = get_project_paths_from_log(trace_file)

        print(f"\nUsing trace file: {TRACE_FILE}")

        if not check_file_exists(TRACE_FILE):
            QMessageBox.critical(None, "File Not Found",
                f"The trace file was not found at:\n\n{TRACE_FILE}\n\nPlease check if the file exists.")
            print(f"ERROR: The trace file was not found at: {TRACE_FILE}")
            sys.exit(1)
        # 3. 根据文件类型选择解析方法
        file_ext = os.path.splitext(TRACE_FILE)[1].lower()
        if file_ext == '.log':
            print("Detected log file format, using ftrace parser")
            task_params = parse_ftrace_log(TRACE_FILE)
            if not task_params:
                QMessageBox.critical(None, "Parse Error",
                    f"Failed to parse the log trace file.\n\nPlease check if the file format is correct.")
                print("Failed to parse log file.")
                sys.exit(1)
        elif file_ext == '.bin':
            print("Detected binary file format, using binary parser")
            task_params = parse_binary_data(TRACE_FILE)
            if not task_params:
                QMessageBox.critical(None, "Parse Error",
                    f"Failed to parse the binary trace file.\n\nPlease check if the file format and config.json are correct.")
                print("Failed to parse binary file.")
                sys.exit(1)
        else:
            QMessageBox.critical(None, "Unsupported File Type",
                f"Unsupported file extension: {file_ext}.\nPlease select a .log or .bin file.")
            print(f"ERROR: Unsupported file extension: {file_ext}")
            sys.exit(1)

        if task_params:
            cores_dict = organize_tasks_by_core(task_params)

            # Pass both plot_type and show_jitter to print_formatted_output function
            print_formatted_output(task_params, plot_type, show_jitter)

            # Save to JSON file
            # Ensure the output directory exists
            os.makedirs(os.path.dirname(JSON_OUTPUT), exist_ok=True)
            save_cores_dict_to_json(cores_dict, JSON_OUTPUT)
            print(f"\nSuccessfully saved task parameters to: {JSON_OUTPUT}")

            # Only generate sample point images when dt or cet parameter is specified
            if plot_type == 'dt':
                plot_all_task_dt_samples(task_params, DT_PLOTS_DIR)
            elif plot_type == 'cet':
                plot_all_task_cet_samples(task_params, CET_PLOTS_DIR)
            elif show_jitter:
                print("\nJitter mode: No plots generated. Use 'python trace_data_parse.py dt' to generate DT plots or 'python trace_data_parse.py cet' to generate CET plots.")
            else:
                print("\nSkip generating plots. Use 'python trace_data_parse.py dt' to generate DT plots or 'python trace_data_parse.py cet' to generate CET plots.")

        else:
            QMessageBox.critical(None, "Parse Error",
                f"Failed to parse the trace file.\n\nPlease check if the file format is correct.")
            print("Failed to parse task parameters")

    except Exception as e:
        QMessageBox.critical(None, "Error", f"An error occurred:\n\n{str(e)}")
        print(f"Error in main program: {str(e)}")
        traceback.print_exc()