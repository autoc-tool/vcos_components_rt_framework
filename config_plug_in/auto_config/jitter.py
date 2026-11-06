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
Jitter Optimization Module

Version Information:
------------------
Current Version: V1.0.1
Initial Release: December 15, 2024
Author: yangpan

Version History:
--------------
V1.0.0 (Initial Release):
- Implemented basic Auto jitter optimization functionality
- Support for both Gliwa and Ftrace file inputs
- Implemented genetic algorithm for task scheduling optimization
- Added multi-core parallel optimization capability

V1.0.1 (Latest):
- Enhanced overload handling: Added CET adjustment using PCET or standard deviation
  when CET max values cannot meet requirements
- Improved logging: Added detailed multi-process logging for iteration parameters
  and timing information
- Added automatic input file detection and handling with latest file selection
- Optimized output formatting for better readability

This module implements a genetic algorithm based optimization for AUTOSAR OS task scheduling.
It aims to minimize jitter and optimize task execution timing across multiple cores.

Key components:
- Task scheduling simulation
- Multi-core parallel optimization
- HTML configuration parsing
- Genetic algorithm implementation
"""

# Standard library imports
import json
import logging
import multiprocessing
import os
import random
import re
import time
from enum import Enum
from multiprocessing import Pool

# Third-party imports
from ui_adaptor.ui_interaction.ui_logger import logger

# Local application imports
from .scheduler import Scheduler, Task
from .trace_data_parse import (
    parse_ftrace_log,parse_binary_data,
    organize_tasks_by_core as organize_ftrace_tasks
)

# Global variables
TraceInfoList = []  # Clear list, start fresh
SimTaskList = []

# Loss function types
LOSS_WEIGHT = 1      # Weight loss (RT+IPT)
LOSS_RT = 2          # Response time loss
LOSS_IPT = 3         # Initial waiting time loss
LOSS_START_TIME = 4  # Start time loss

# Default configuration structure
DEFAULT_CONFIG = {
    "sch_tbl": {
        # Format: "OsScheduleTable_CoreX": {
        #     "sch_tbl_name": "OsScheduleTable_CoreX",
        #     "task_offsets": []
        # }
    },
    "alarm": {
        # Format: "task_name": {
        #     "alarm_list": [],
        #     "offset": None
        # }
    }
}

class DataSourceType(Enum):
    """Data source type enumeration"""
    GLIWA_HTML = 1
    FTRACE_LOG = 2
    BINARY_LOG = 3

# Global variable to control data source type
CURRENT_DATA_SOURCE = DataSourceType.FTRACE_LOG

PCET_SAMPLES_ENABLE = True
STD_DEV_ENABLE = False

# Ensure mutual exclusivity of PCET and STD_DEV
if CURRENT_DATA_SOURCE != DataSourceType.FTRACE_LOG:
    PCET_SAMPLES_ENABLE = False
    STD_DEV_ENABLE = False
elif PCET_SAMPLES_ENABLE:
    STD_DEV_ENABLE = False
elif STD_DEV_ENABLE:
    PCET_SAMPLES_ENABLE = False

def set_data_source(source_type: DataSourceType):
    """Set the data source type"""
    global CURRENT_DATA_SOURCE
    CURRENT_DATA_SOURCE = source_type

MPLOG_CONSOLE_OUTPUT_ENABLE = False

def setup_mp_logger(core_name, config_base_path):
    """Set up a multiprocessing logger for a specific core.

    Args:
        core_name: Name of the core (e.g. "Core0")
        config_base_path: Base path for configuration files

    Returns:
        logging.Logger: Configured logger for this core
    """
    # Create a logger with the core name
    mp_logger = logging.getLogger(f"JitterOpt_{core_name}")
    mp_logger.setLevel(logging.INFO)

    # Create formatter
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')

    if MPLOG_CONSOLE_OUTPUT_ENABLE:
        # Create console handler (commented out to reduce console output)
        console_handler = logging.StreamHandler()
        console_handler.setLevel(logging.INFO)
        console_handler.setFormatter(formatter)

    # Create Export directory if it doesn't exist
    export_dir = os.path.join(config_base_path, 'Export')
    os.makedirs(export_dir, exist_ok=True)

    # Create file handler with timestamp in Export directory
    timestamp = time.strftime("%Y%m%d_%H%M%S")
    log_filename = f"JitterOpt_{core_name}_{timestamp}.log"
    log_path = os.path.join(export_dir, log_filename)
    file_handler = logging.FileHandler(log_path)
    file_handler.setLevel(logging.INFO)
    file_handler.setFormatter(formatter)

    # Remove any existing handlers
    mp_logger.handlers.clear()

    # Add handlers to logger (only file handler now)
    if MPLOG_CONSOLE_OUTPUT_ENABLE:
        mp_logger.addHandler(console_handler)  # Commented out console handler
    mp_logger.addHandler(file_handler)

    return mp_logger

class Algorithm:
    """
    Genetic algorithm implementation for optimizing task offsets.

    Uses evolutionary computation to find optimal task activation offsets that
    minimize jitter and improve overall scheduling performance.

    Attributes:
        task_list (list): List of tasks to optimize
        core_num (str): Core identifier
        loss_option (int): Selected loss function type
        sample_num (int): Population size for genetic algorithm
        mutation_rate (float): Probability of mutation
        epochs (int): Number of generations to evolve
    """
    def __init__(
            self,
            tasks,
            core_num,
            loss_option,
            time_unit,
            mp_logger,
            sample_num=100,
            mutation_rate=0.1,
            epochs=100
        ):
        self.task_list = tasks
        self.core_num = core_num
        self.time_unit = time_unit
        self.loss_option = loss_option
        self.task_num = len(tasks)
        self.sample_num = sample_num
        self.mutation_rate = mutation_rate
        self.epochs = epochs
        self.mp_logger = mp_logger  # Save mp_logger instance

        self.initialize_tasks()

    # Input task parameters
    def initialize_tasks(self):
        """
        Initialize task parameters and scheduler

        - Calculate maximum simulation time
        - Calculate theoretical minimum execution time
        - Initialize task periods and execution times
        - Calculate offset range
        """
        tasks_copy_init = [
            Task(
                task_name=task.task_name,
                priority=task.priority,
                period=task.period,
                CET=task.CET,
                arrival_time=task.arrival_time,
                activite_num=task.activite_num
            ) for task in self.task_list
        ]

        scheduler = Scheduler()
        for task in tasks_copy_init:
            scheduler.add_task(task)

        tasks_copy_init = [
            t for t in tasks_copy_init
            if not (t.period >= 100000 and float(t.CET/t.period) <= 0.01)
        ]

        max_simulation_time = scheduler.calculate_max_time(tasks_copy_init)
        scheduler.run(max_simulation_time)
        # scheduler.print_total_ipt()  # Print the Initial total IPT for all tasks
        # scheduler.print_total_rt()  # Print the Initial total RT for all tasks

        # Calculate theoretical minimum execution time and maximum IPT
        self.min_cet = sum(
            (max_simulation_time / task.period) * task.CET
            for task in self.task_list
        )  # theoretical value, may too high when load is full
        self.max_ipt = scheduler.total_ipt
        self.max_time = max_simulation_time

        # Initialize task parameters
        self.periods = [task.period for task in self.task_list]
        self.CETs = [task.CET for task in self.task_list]
        self.offset_combination = [(0, self.periods[i] - self.CETs[i]) for i in range(self.task_num)]

        self.mp_logger.info(f"Theory  Response Time (RT) for all tasks:{self.min_cet}")
        self.mp_logger.info(f"Initial Response Time (RT) for all tasks:{scheduler.total_rt}")
        self.mp_logger.info(f"Scheduler Simulation Period:{self.max_time}")

    # Init the offset combination
    def initialize_population(self, optimized_tasks, previous_solution):
        """
        Initialize population for genetic algorithm

        Args:
            optimized_tasks: Number of tasks to optimize
            previous_solution: Previous optimization solution if available

        Returns:
            List of individuals (offset combinations)
        """
        population = []
        for _ in range(self.sample_num):
            individual = []
            for i in range(optimized_tasks):
                if i == 0:
                    individual.append(0.0)
                else:
                    if previous_solution and i < len(previous_solution):
                        individual.append(previous_solution[i])
                    else:
                        offset = random.randint(
                            self.offset_combination[i][0],
                            self.offset_combination[i][1] // self.time_unit
                        ) * self.time_unit
                        offset = round(offset)
                        individual.append(offset)
            population.append(individual)
        return population

    # Mix the chosen parent samples
    def crossover(self, parent_sample1, parent_sample2, optimized_tasks):
        """
        Perform crossover between two parent individuals

        Args:
            parent_sample1: First parent individual
            parent_sample2: Second parent individual
            optimized_tasks: Number of tasks to optimize

        Returns:
            Tuple of two child individuals
        """
        if optimized_tasks > 1:
            mutate_point = random.randint(1, optimized_tasks - 1)
            child1 = [0.0] + parent_sample1[1:mutate_point] + parent_sample2[mutate_point:]
            child2 = [0.0] + parent_sample2[1:mutate_point] + parent_sample1[mutate_point:]
        else:
            child1 = [0.0]
            child2 = [0.0]
        return child1, child2

    def mutation(self, individual, optimized_tasks):
        """
        Perform mutation on an individual

        Args:
            individual: Individual to mutate
            optimized_tasks: Number of tasks to optimize

        Returns:
            None (mutates individual in-place)
        """
        for i in range(optimized_tasks):
            if i == 0:
                individual[i] = 0.0
            else:
                if random.random() < self.mutation_rate:
                    offset = random.randint(
                        self.offset_combination[i][0],
                        self.offset_combination[i][1] // self.time_unit
                    ) * self.time_unit
                    individual[i] = offset

    # Select the parent samples from simulation model results
    def selection(self, population, simulation_values):
        """
        Select parent individuals for reproduction

        Args:
            population: Current population
            simulation_values: Fitness values for each individual

        Returns:
            List of selected parent individuals
        """
        adjusted_simulation_values = [1 / (v + 1e-8) for v in simulation_values]
        total_adjusted = sum(adjusted_simulation_values)
        selection_probs = [av / total_adjusted for av in adjusted_simulation_values]
        parents = random.choices(population, weights=selection_probs, k=2)
        return parents

    # Add new samples to population
    def evolve(self, population, optimized_tasks):
        """
        Genetic algorithm evolution function

        Args:
            population: Current population
            optimized_tasks: Number of tasks to optimize

        Returns:
            tuple: (new_population, best_individual, best_simulation)
            - new_population: New generation population
            - best_individual: Current best individual
            - best_simulation: Current best fitness value
        """
        new_population = []
        simulation_values = []
        best_individual = None
        best_simulation = float('inf')
        total_individuals = len(population)

        for idx, individual in enumerate(population, 1):
            offsets = individual[:optimized_tasks]
            """ START CALL scheduler.run to get model loss value """
            scheduler_starttime = time.time()
            scheduler = Scheduler()
            # Deep copy prevent modification on initial data
            tasks_copy = [
                Task(
                    task_name=task.task_name,
                    priority=task.priority,
                    period=task.period,
                    CET=task.CET,
                    arrival_time=task.arrival_time,
                    activite_num=task.activite_num
                ) for task in self.task_list
            ]

            for i, task in enumerate(tasks_copy):
                if i < optimized_tasks:
                    offset = offsets[i]
                    new_arrival_time = round(task.arrival_time + offset)
                else:
                    new_arrival_time = task.arrival_time
                # Add new task to scheduler
                new_task = Task(
                    task_name=task.task_name,
                    priority=task.priority,
                    period=task.period,
                    CET=task.CET,
                    arrival_time=new_arrival_time,
                    activite_num=task.activite_num
                )
                scheduler.add_task(new_task)
            scheduler.run(self.max_time)
            scheduler_time = time.time() - scheduler_starttime

            # Format output scheduler simulation time
            self.mp_logger.info(f"{'='*80}")
            self.mp_logger.info(f"Individual {idx}/{total_individuals}")
            self.mp_logger.info(f"Scheduler simulation time: {scheduler_time:.3f} seconds")
            """ END OF CALL scheduler """

            # ----- Loss Function -----
            if self.loss_option == LOSS_WEIGHT:  # Option1. SET WEIGHT(RT+IPT) AS LOSS
                ipt_target_weight = 0.01
                normalized_rt = abs(scheduler.total_rt - self.min_cet) / self.min_cet * 100
                normalized_ipt = scheduler.total_ipt / (self.max_ipt * ipt_target_weight) * 100
                weight_rt = 0.5
                weight_ipt = 0.5
                evaluation_value = weight_rt * normalized_rt + weight_ipt * normalized_ipt
            elif self.loss_option == LOSS_RT:  # Option2. SET RT AS LOSS
                evaluation_value = max(0, scheduler.total_rt - self.min_cet)
            elif self.loss_option == LOSS_IPT:  # Option3. SET IPT AS LOSS
                evaluation_value = scheduler.total_ipt
            elif self.loss_option == LOSS_START_TIME:  # Option4. SET rt AS LOSS AND make offset as small as possible
                evaluation_value = (
                    abs(scheduler.total_rt - self.min_cet) / self.min_cet * 100 +
                    sum(individual[:optimized_tasks]) / sum(self.periods) * 10
                )

            simulation_values.append(evaluation_value)
            if evaluation_value < best_simulation:
                best_simulation = evaluation_value
                best_individual = individual.copy()
                self.mp_logger.info(f"Found better solution!")
                self.mp_logger.info(f"Evaluation value: {evaluation_value:.3f}")
                self.mp_logger.info(f"Simulation time: {scheduler_time:.3f} seconds")
            self.mp_logger.info(f"{'='*80}\n")

        # Create new individual
        while len(new_population) < self.sample_num:
            parents = self.selection(population, simulation_values)
            child1, child2 = self.crossover(parents[0], parents[1], optimized_tasks)

            self.mutation(child1, optimized_tasks)
            self.mutation(child2, optimized_tasks)
            new_population.extend([child1, child2])

        new_population = new_population[:self.sample_num]
        return new_population, best_individual, best_simulation

    # iterate calculation on epochs
    def iteration(self, optimized_tasks, previous_solution=None):
        """
        Runs genetic algorithm iterations to optimize task offsets.

        Args:
            optimized_tasks (int): Number of tasks to optimize
            previous_solution (list, optional): Previous optimization results

        Returns:
            tuple: (best_individual, best_simulation, best_simulation_list)
            - best_individual: Best found offset combination
            - best_simulation: Best fitness value achieved
            - best_simulation_list: History of best fitness values
        """
        population = self.initialize_population(optimized_tasks, previous_solution)
        best_individual = None
        best_simulation = float('inf')
        best_simulation_list = []

        for generation in range(self.epochs):
            generation_starttime = time.time()
            population, gen_best_individual, gen_best_simulation = self.evolve(
                population, optimized_tasks
            )

            if gen_best_simulation < best_simulation:
                best_simulation = gen_best_simulation
                best_individual = gen_best_individual.copy()
                if best_simulation < 1e-7:
                    generation_time = time.time() - generation_starttime
                    break

            best_simulation_list.append(gen_best_simulation)
            generation_time = time.time() - generation_starttime
            self.mp_logger.info(
                f"{self.core_num} Generation {generation + 1} best model loss value: "
                f"{gen_best_simulation}"
            )
            self.mp_logger.info(
                f"{self.core_num} Generation {generation + 1} spend time: "
                f"{generation_time:.3f} seconds"
            )

        return best_individual, best_simulation, best_simulation_list

    def algorithm(self):
        """
        Main algorithm method that optimizes task offsets

        Returns:
            tuple: (results, best_simulation)
            - results: List of optimized offsets
            - best_simulation: Best simulation value achieved
        """
        results = []
        for optimized_tasks in range(self.task_num, self.task_num + 1):
            self.mp_logger.info(f"Optimizing first task1 to task{optimized_tasks}")
            previous_solution = results if len(results) == optimized_tasks - 1 else None
            best_individual, best_simulation, best_simulation_list = self.iteration(
                optimized_tasks, previous_solution
            )
            results = best_individual[:optimized_tasks]
            self.mp_logger.info(
                f"First {optimized_tasks} task's best model loss value: {best_simulation}"
            )

        self.mp_logger.info(f"{self.core_num} Offset Combination:{results}")
        self.mp_logger.info(f"{self.core_num} model loss Value after optimization:{best_simulation}")

        return results, best_simulation


class CoreManager:
    """
    Manages task distribution and configuration across multiple cores.

    Handles task allocation to cores and maintains core-specific configurations.
    """
    def _binary_search_n_value(self, valid_tasks, tasks_by_name, max_iterations=5):
        """
        Use binary search to find suitable N value

        Args:
            valid_tasks: List of valid tasks
            tasks_by_name: Mapping from task name to task information
            max_iterations: Maximum number of iterations

        Returns:
            tuple: (found N value, whether a suitable value was found)
        """
        left, right = 1.0, 3.0  # N's search range [1,3]
        best_n = 0
        iterations = 0

        while iterations < max_iterations:
            n = (left + right) / 2
            # Use current N value to calculate utilization
            utilization = self._calculate_utilization(
                valid_tasks,
                tasks_by_name,
                use_max=False,
                use_std_dev=True,
                n_value=n
            )

            if (1.0 - utilization) < 0.1:  # Found suitable value
                return n, True
            elif utilization > 1.0:
                logger.info(f"Utilization {utilization:.3f} exceeds 1.0 with N={n}. Exiting search.")
                return best_n, False  # Exit immediately, return the last best_n
            else:
                best_n = n  # Save the last available N value
                left = n  # Update the left boundary

            iterations += 1

        return best_n, False

    def _calculate_utilization(
        self,
        valid_tasks,
        tasks_by_name,
        use_max=True,
        use_std_dev=False,
        n_value=3.0
    ):
        """
        Calculate core utilization

        Args:
            valid_tasks: List of valid tasks
            tasks_by_name: Mapping from task name to task information
            use_max: Whether to use maximum CET value
            use_std_dev: Whether to use standard deviation adjustment
            n_value: Multiple of standard deviation

        Returns:
            float: CPU utilization (value between 0.0 and 1.0)
        """
        utilization_sum = 0.0
        task_utilizations = []  # Store utilization of each task for logging

        for i, (task_name, task_params) in enumerate(valid_tasks):
            original_task = tasks_by_name[task_name]
            period = float(original_task['period'])

            if period <= 0:
                logger.warning(f"Task {task_name} has invalid period: {period}")
                continue

            if use_max:
                cet = float(task_params.get('cet_max', 0))
            else:
                cet = float(task_params.get('cet_avg', 0))
                if (use_std_dev and
                    CURRENT_DATA_SOURCE == DataSourceType.FTRACE_LOG and
                    STD_DEV_ENABLE):
                    std_dev = float(task_params.get('cet_std_dev', 0))
                    cet += float(n_value * std_dev)
                    task_params['cet_avg'] = round(cet)  # Update current task's CET value
                    valid_tasks[i] = (task_name, task_params)  # Update valid_tasks with task parameters
                    logger.ui_info(
                        f"Updated {task_name} cet_avg to {round(cet)}, n_value is {n_value}"
                    )  # Add log to confirm update

            task_utilization = cet / period
            utilization_sum += task_utilization
            task_utilizations.append((task_name, task_utilization, cet, period))

        # Detailed utilization log
        for task_name, task_util, cet, period in task_utilizations:
            logger.ui_info(
                f"Task {task_name:<40} CET={cet:>8.2f}us, Period={period:>8.2f}us, "
                f"Utilization={task_util:>6.3f}"
            )

        return utilization_sum

    def _try_percentile_cet(self, valid_tasks, tasks_by_name, core_name):
        """
        Find suitable execution time values between CET_Avg and CET_Max
        Use percentile data from PCET list (from 99% to 90%) to try to find a suitable CET value
        so that CPU utilization is less than 1

        Args:
            valid_tasks: List of valid tasks
            tasks_by_name: Mapping from task name to task information
            core_name: Name of the core

        Returns:
            tuple: (success, confidence, current_utilization)
        """
        if CURRENT_DATA_SOURCE != DataSourceType.FTRACE_LOG:
            return False, None, None

        max_tries = 10  # Maximum number of attempts (from 99% to 90%)
        current_try = 0

        while current_try < max_tries:
            confidence = 99 - current_try  # Current confidence

            # Update each task's CET value to the current percentile value
            for i, (task_name, task_params) in enumerate(valid_tasks):
                # Get PCET values from valid_tasks
                pcet_values = task_params['PCET']  # Directly get PCET values from task_params
                if len(pcet_values) > current_try:
                    new_cet = pcet_values[current_try]
                    task_params['cet_avg'] = new_cet  # Update current task's CET value
                    valid_tasks[i] = (task_name, task_params)  # Update valid_tasks with task parameters
                    logger.ui_info(
                        f"Updated {task_name} cet_avg to {new_cet} at {confidence}th percentile"
                    )  # Add log to confirm update

            # Calculate CPU utilization using updated CET values
            current_utilization = self._calculate_utilization(
                valid_tasks, tasks_by_name, use_max=False
            )

            # Record the result of each attempt
            logger.ui_info(
                f"{core_name}: Trying {confidence}th percentile CET values, "
                f"utilization = {current_utilization:.3f}"
            )

            # Check if suitable utilization is found
            if current_utilization < 1.0:
                logger.ui_info(f"{core_name}: Found suitable CET values at {confidence}th percentile")
                logger.ui_info(f"{core_name}: Final utilization = {current_utilization:.3f}")
                return True, confidence, current_utilization

            current_try += 1

        # If all percentiles have been tried and no suitable value is found, return failure
        logger.ui_info(f"{core_name}: Failed to find suitable CET values using percentile method")
        return False, None, None

    def __init__(self, tasks_by_core, cores_dict):
        """
        Initialize CoreManager with tasks and core configurations

        Args:
            tasks_by_core: Dictionary mapping core names to task lists
            cores_dict: Dictionary containing core configuration parameters
        """
        self.cores = []
        for core_name, tasks in tasks_by_core.items():
            core = Core(core_name)
            tasks_by_name = {task['name']: task for task in tasks}
            core_tasks = cores_dict.get(core_name, {})

            # Validate and collect valid tasks
            valid_tasks = []
            for task_name, task_params in core_tasks.items():
                cet_max = int(task_params.get('cet_max', 0))
                cet_avg = int(task_params.get('cet_avg', 0))

                if cet_max <= 0 or cet_avg <= 0:
                    logger.ui_info(f"{core_name}: Invalid execution times for task {task_name}")
                    continue

                if task_name not in tasks_by_name:
                    logger.ui_info(f"{core_name}: Task {task_name} not found in tasks_by_core")
                    continue

                task_params['valid'] = True
                original_task = tasks_by_name[task_name]
                task_params['priority'] = original_task['priority']
                valid_tasks.append((task_name, task_params))

            if not valid_tasks:
                logger.ui_info(f"{core_name}: No valid tasks found")
                continue

            # Sort tasks by priority from high to low
            valid_tasks.sort(key=lambda x: x[1]['priority'], reverse=True)
            original_task_count = len(valid_tasks)

            logger.ui_info(f"{core_name}: Processing {len(valid_tasks)} tasks")
            logger.ui_info("=" * 80)

            iteration = 0
            max_iterations = len(valid_tasks)  # Maximum number of iterations equals the number of tasks

            while iteration < max_iterations:
                # First try using maximum CET values
                utilization_sum = self._calculate_utilization(
                    valid_tasks, tasks_by_name, use_max=True
                )
                logger.ui_info(
                    f"{core_name} Iteration {iteration + 1}: "
                    f"Max CET utilization = {utilization_sum:.3f}"
                )

                if utilization_sum < 1.0:
                    use_max_cet = True
                    logger.ui_info(f"{core_name}: Success - Using maximum CET values")
                    break

                # If maximum CET utilization > 1, try using average CET values
                avg_utilization = self._calculate_utilization(
                    valid_tasks, tasks_by_name, use_max=False
                )
                logger.ui_info(f"{core_name}: Average CET utilization = {avg_utilization:.3f}")

                if avg_utilization >= 1.0:
                    # If average execution time utilization exceeds 1, exit with error
                    logger.ui_info(
                        f"{core_name}: Failed - Average CET utilization "
                        f"{avg_utilization:.3f} exceeds 100%"
                    )
                    logger.ui_info(
                        f"{core_name}: Core overload detected, please check task configuration"
                    )
                    return False, None  # Exit directly, terminate subsequent processes

                # If average CET utilization < 1
                if CURRENT_DATA_SOURCE == DataSourceType.FTRACE_LOG:
                    # FTRACE data source: attempt CET value adjustment
                    success = False

                    # First try using PCET percentile data
                    if PCET_SAMPLES_ENABLE:
                        success, confidence, adjusted_utilization = self._try_percentile_cet(
                            valid_tasks, tasks_by_name, core_name
                        )
                        if success:
                            use_max_cet = False
                            logger.ui_info(
                                f"{core_name}: Success - Using {confidence}th percentile CET values "
                                f"(utilization={adjusted_utilization:.3f})"
                            )
                            break

                    # If PCET method fails, try using standard deviation method
                    if not success and STD_DEV_ENABLE:
                        n_value, found = self._binary_search_n_value(valid_tasks, tasks_by_name)
                        if found:
                            use_max_cet = False
                            self.n_value = n_value
                            adjusted_utilization = self._calculate_utilization(
                                valid_tasks,
                                tasks_by_name,
                                use_max=False,
                                use_std_dev=True,
                                n_value=n_value
                            )
                            logger.ui_info(
                                f"{core_name}: Success - Using adjusted CET values "
                                f"(N={n_value:.2f}, utilization={adjusted_utilization:.3f})"
                            )
                            break

                    # If both methods fail, remove the lowest priority task and continue the loop
                    if not success:
                        if len(valid_tasks) <= 1:
                            logger.ui_info(
                                f"{core_name}: Failed - Cannot achieve utilization < 1.0 "
                                f"even with highest priority task"
                            )
                            return False, None

                        removed_task = valid_tasks.pop()
                        logger.ui_info(f"{core_name}: Removing lowest priority task {removed_task[0]}")
                        iteration += 1
                        continue  # Continue to the next loop

            # Task configuration summary
            tasks_removed = original_task_count - len(valid_tasks)
            if tasks_removed > 0:
                logger.ui_info(f"{core_name}: Removed {tasks_removed} low priority tasks")

            # Create and add final task configuration
            if valid_tasks:
                logger.ui_info(f"{core_name}: Final configuration")
                logger.ui_info("-" * 80)

                for task_name, task_params in valid_tasks:
                    original_task = tasks_by_name[task_name]
                    if use_max_cet:
                        cet = int(task_params.get('cet_max', 0))
                    else:
                        cet = int(task_params.get('cet_avg', 0))
                        if (CURRENT_DATA_SOURCE == DataSourceType.FTRACE_LOG and
                            STD_DEV_ENABLE and
                            not use_max_cet):
                            std_dev = int(task_params.get('cet_std_dev', 0))
                            cet += int(self.n_value * std_dev)

                    task_obj = Task(
                        task_name=task_name,
                        priority=original_task['priority'],
                        period=round(original_task['period']),
                        CET=round(cet),
                        arrival_time=0,
                        activite_num=0
                    )
                    core.add_task(task_obj)

                    logger.ui_info(
                        f"Task: {task_name:<40} Priority: {original_task['priority']:<3} "
                        f"Period: {original_task['period']:>8}us CET: {cet:>8}us"
                    )

                final_utilization = self._calculate_utilization(
                    valid_tasks,
                    tasks_by_name,
                    use_max=use_max_cet,
                    use_std_dev=not use_max_cet and
                               CURRENT_DATA_SOURCE == DataSourceType.FTRACE_LOG,
                    n_value=getattr(self, 'n_value', 3.0)
                )
                logger.ui_info(f"{core_name}: Final utilization = {final_utilization:.3f}")

            self.cores.append(core)

class Core:
    """
    Represents a core in the system with its associated tasks

    Attributes:
        core_name: Name of the core
        task_list: List of tasks assigned to this core
    """
    def __init__(self, core_name):
        """
        Initialize a core with its name

        Args:
            core_name: Name of the core (e.g., "Core0")
        """
        self.core_name = core_name
        self.task_list = []

    def add_task(self, task):
        """
        Add a task to this core

        Args:
            task: Task object to add
        """
        self.task_list.append(task)

    def find_task(self, task_name):
        """
        Find a task by name

        Args:
            task_name: Name of the task to find

        Returns:
            Task object if found, None otherwise
        """
        for task in self.task_list:
            if task.task_name == task_name:
                return task
        return None

# Optimized core task configuration
tasks_by_core = {
    "Core0": [
        {
            "name": "Core0_BSW_Task_1ms",
            "period": 1000,
            "priority": 7,
            "cet": 100    # Core Execution Time in microseconds
        }
    ],
    "Core1": [
        {
            "name": "Core1_BSW_Task_1ms",
            "period": 1000,
            "priority": 7,
            "cet": 100
        }
    ]
    # ... Core2 and Core3 configurations can be added in the same structure
}

def optimize_core(core, mp_logger):
    """
    Optimizes task scheduling for a single core.

    Args:
        core (Core): Core configuration to optimize
        mp_logger: Logger for this core's process

    Returns:
        tuple: (core_name, results, best_simulation, core_cfg, elapsed_time)
    """
    try:
        mp_logger.info(f"Starting optimization for {core.core_name}")
        if not core.task_list:
            mp_logger.error(f"No tasks found for {core.core_name}")
            return core.core_name, None, None, None, 0

        start_time = time.time()

        # Log task configuration information
        mp_logger.info("Task Configuration:")
        mp_logger.info("-" * 80)
        mp_logger.info(f"{'Task Name':<40} {'Priority':<8} {'Period(us)':<12} {'CET(us)'}")
        mp_logger.info("-" * 80)
        for task in core.task_list:
            mp_logger.info(f"{task.task_name:<40} {task.priority:<8} {task.period:<12} {task.CET}")

        algorithm = Algorithm(
            core.task_list,
            core.core_name,
            loss_option=LOSS_RT,
            time_unit=100,
            mp_logger=mp_logger
        )

        # Modify Algorithm class's log output
        def algorithm_logger(self, message):
            mp_logger.info(f"{self.core_num}: {message}")

        # Modify Scheduler class's log output
        def scheduler_logger(scheduler, message):
            mp_logger.info(f"Scheduler: {message}")

        results, best_simulation = algorithm.algorithm()
        # results = [round(x) * 100 for x in results]
        elapsed_time = time.time() - start_time

        # Create task configuration
        core_cfg = [
            {
                "offset": offset,
                "task_name": task,
                "runnable_offsets": {}
            }
            for task, offset in zip(core.task_list, results)
        ]

        mp_logger.info(f"\n{'='*80}")
        mp_logger.info(f"{core.core_name} optimization completed")
        mp_logger.info(f"{'-'*80}")
        mp_logger.info(f"{'Total execution time:':<30} {elapsed_time:.2f} seconds")
        mp_logger.info(f"{'Best simulation value:':<30} {best_simulation:.1f}")
        mp_logger.info(f"\nFinal task offsets:")
        mp_logger.info(f"{'-'*80}")
        # Calculate the longest task name length for alignment
        max_name_length = max(len(task.task_name) for task in core.task_list)
        # Format output for each task's offset value
        for task, offset in zip(core.task_list, results):
            mp_logger.info(f"{task.task_name:<{max_name_length+5}}: {offset:>8}us")
        mp_logger.info(f"{'='*80}\n")

        return core.core_name, results, best_simulation, core_cfg, elapsed_time

    except Exception as e:
        mp_logger.error(f"Error optimizing {core.core_name}: {str(e)}", exc_info=True)
        return core.core_name, None, None, None, 0

def terminate_pool(pool):
    """
    Terminates the pool of processes forcefully.

    Args:
        pool (multiprocessing.Pool): The pool of processes to terminate.
    """
    try:
        if pool:
            pool.terminate()
            pool.join()
    except Exception as e:
        logger.error(f"Error terminating pool: {str(e)}")

def update_task_offset(schedule_table, task_name, new_offset):
    """
    Updates the offset of an existing task in the schedule table

    Args:
        schedule_table (dict): Schedule table configuration
        task_name (str): Name of the task to update
        new_offset (int): New offset value

    Returns:
        bool: True if task was found and updated, False otherwise
    """
    for task in schedule_table["task_offsets"]:
        if task["task_name"] == task_name:
            task["offset"] = new_offset
            return True
    return False

def parallel_optimize_cores(core_manager: CoreManager, all_sch_tbl_cfg: dict, os_counter_dict: dict, config_base_path: str) -> dict:
    """
    Performs parallel optimization of tasks across multiple cores and updates jitter configuration.

    Args:
        core_manager (CoreManager): Manager containing core configurations
        all_sch_tbl_cfg (dict): Schedule table configuration for all cores
        os_counter_dict (dict): OS counter configuration for each core
        config_base_path: Base path for configuration files

    Returns:
        dict: Optimized jitter configurations for all cores
    """
    # Initialize jitter_cfg with empty sections
    jitter_cfg = {
        "sch_tbl": {},
        "alarm": {}
    }

    # Create a mapping of core names to their schedule table names and counter configs
    schedule_table_names = {}

    # First pass: Collect all schedule tables for each core
    for sch_tbl_name in all_sch_tbl_cfg:
        # Use regular expression to match parts containing "CoreX"
        core_match = re.search(r'Core\d+', sch_tbl_name)
        if core_match:
            core_name = core_match.group()
            # If this core doesn't have a list yet, create a new one
            if core_name not in schedule_table_names:
                schedule_table_names[core_name] = []
            # Add the schedule table to the corresponding core's list
            schedule_table_names[core_name].append(sch_tbl_name)
        else:
            logger.warning(f"Could not extract core name from schedule table: {sch_tbl_name}")
            continue

    # Verify if all schedule tables are correctly assigned
    all_mapped_tables = [table for tables in schedule_table_names.values() for table in tables]
    missing_tables = set(all_sch_tbl_cfg.keys()) - set(all_mapped_tables)
    if missing_tables:
        logger.warning(f"Some schedule tables were not mapped to any core: {missing_tables}")

    # Create counter tick mapping for each core
    counter_ticks = {}
    for counter_name, counter_config in os_counter_dict.items():
        core_num = counter_name[-1]  # Extract core number from SystemTimerX
        ticks_per_second = float(counter_config["OsSecondsPerTick"])
        counter_ticks[f"Core{core_num}"] = ticks_per_second

    # Create a set of all tasks in schedule tables for quick lookup
    scheduled_tasks = {}  # Record which schedule tables each task belongs to
    for sch_tbl_name, sch_tbl_data in all_sch_tbl_cfg.items():
        for task_names in sch_tbl_data["task_offset_dict"].values():
            for task_name in task_names:
                if task_name not in scheduled_tasks:
                    scheduled_tasks[task_name] = []  # Initialize as an empty list
                if sch_tbl_name not in scheduled_tasks[task_name]:  # Avoid adding the same schedule table multiple times
                    scheduled_tasks[task_name].append(sch_tbl_name)

    # Initialize schedule tables in jitter_cfg
    for core_name, sch_tbl_list in schedule_table_names.items():
        ticks_per_second = counter_ticks.get(core_name, 0.00000001)  # Default to 10ns if not found
        skip_optimization = ticks_per_second >= 0.001  # Skip if tick is >= 1ms

        # Process all schedule tables for this core
        for sch_tbl_name in sch_tbl_list:
            jitter_cfg["sch_tbl"][sch_tbl_name] = {
                "sch_tbl_name": sch_tbl_name,
                "task_offsets": []
            }

            # If tick >= 1ms, set all offsets to 0
            if skip_optimization:
                sch_tbl_data = all_sch_tbl_cfg[sch_tbl_name]
                for offset, task_names in sch_tbl_data["task_offset_dict"].items():
                    for task_name in task_names:
                        task_cfg = {
                            "offset": 0,
                            "task_name": task_name,
                            "runnable_offsets": {}
                        }
                        jitter_cfg["sch_tbl"][sch_tbl_name]["task_offsets"].append(task_cfg)
                continue

            # Add existing task offsets from schedule table configuration
            sch_tbl_data = all_sch_tbl_cfg[sch_tbl_name]
            for offset, task_names in sch_tbl_data["task_offset_dict"].items():
                for task_name in task_names:
                    task_cfg = {
                        "offset": int(offset),
                        "task_name": task_name,
                        "runnable_offsets": {}
                    }
                    jitter_cfg["sch_tbl"][sch_tbl_name]["task_offsets"].append(task_cfg)

    num_processes = max(1, min(multiprocessing.cpu_count(), len(core_manager.cores)))
    if len(core_manager.cores) == 0:
        logger.warning("No cores to optimize")
        return jitter_cfg

    # Record overall start time
    overall_start_time = time.time()
    logger.ui_info(f"Starting parallel optimization with {num_processes} processes")
    logger.ui_info(f"Please wait patiently ... ")

    try:
        with Pool(processes=num_processes) as pool:
            # Modify optimize_core to include counter tick information
            results = pool.starmap(optimize_core_with_ticks,
                [(core, counter_ticks.get(core.core_name, 0.00000001), config_base_path)
                 for core in core_manager.cores])

            for core_name, opt_results, best_sim, core_cfg, elapsed_time in results:
                if opt_results is not None:
                    core_schedule_tables = schedule_table_names.get(core_name, [])
                    ticks_per_second = counter_ticks.get(core_name, 0.00000001)

                    # Skip optimization if tick >= 1ms
                    if ticks_per_second >= 0.001:
                        continue

                    for task_cfg in core_cfg:
                        task_name = task_cfg["task_name"].task_name
                        offset_us = task_cfg["offset"]

                        # Convert offset to be multiple of tick period
                        tick_period_us = ticks_per_second * 1_000_000  # Convert to microseconds
                        num_ticks = round(offset_us / tick_period_us)
                        adjusted_offset = int(num_ticks)

                        # Check if the task is in the schedule table
                        if task_name in scheduled_tasks:
                            # Iterate through all schedule tables the task belongs to
                            for sch_tbl_name in scheduled_tasks[task_name]:
                                # Only update the schedule table that belongs to the current core
                                if sch_tbl_name in core_schedule_tables:
                                    if not update_task_offset(
                                        jitter_cfg["sch_tbl"][sch_tbl_name],
                                        task_name,
                                        adjusted_offset
                                    ):
                                        task_cfg["offset"] = adjusted_offset
                                        jitter_cfg["sch_tbl"][sch_tbl_name]["task_offsets"].append(task_cfg)
                        else:
                            # If the task is not in any schedule table, handle it as an alarm
                            offset_ms = (num_ticks * tick_period_us) / 1000.0
                            offset_str = f"{offset_ms:.2f}"

                            if task_name not in jitter_cfg["alarm"]:
                                jitter_cfg["alarm"][task_name] = {"alarm_list": [], "offset": None}
                            jitter_cfg["alarm"][task_name]["offset"] = offset_str

                    logger.ui_info(f"{core_name} optimization results:")
                    logger.ui_info(f"  Best simulation value: {best_sim}")
                    logger.ui_info(f"  Individual execution time: {elapsed_time:.2f} seconds")

        # Calculate actual total execution time
        total_time = time.time() - overall_start_time
        logger.ui_info(f"Total parallel execution time: {total_time:.2f} seconds")
        return jitter_cfg

    except Exception as e:
        logger.error("Error in parallel optimization", exc_info=True)
        terminate_pool(pool)
        return {"sch_tbl": {}, "alarm": {}}

def optimize_core_with_ticks(core, ticks_per_second, config_base_path):
    """
    Optimizes task scheduling for a single core with tick period consideration.

    Args:
        core (Core): Core configuration to optimize
        ticks_per_second (float): OS counter ticks per second for this core
        config_base_path: Base path for configuration files

    Returns:
        tuple: (core_name, results, best_simulation, core_cfg, elapsed_time)
    """
    # Create a separate logger for each core
    mp_logger = setup_mp_logger(core.core_name, config_base_path)

    # Skip optimization if tick period >= 1ms
    if ticks_per_second >= 0.001:
        mp_logger.info(f"{core.core_name}: Skipping optimization due to tick period >= 1ms")
        return core.core_name, [0] * len(core.task_list), 0, [], 0

    return optimize_core(core, mp_logger)

def extract_and_update_tasks_by_core(task_dict: dict) -> None:
    """
    Extract task information from task_dict and update the global tasks_by_core data structure
    Tasks are sorted by priority from high to low (higher number means higher priority)

    Args:
        task_dict: Dictionary containing task configuration information
    """
    global tasks_by_core
    tasks_by_core = {}

    # Iterate through each core
    for core_name, core_tasks in task_dict.items():
        tasks_by_core[core_name] = []

        # Iterate through all tasks under this core
        for task_name, task_info in core_tasks.items():
            # Get task priority
            priority = int(task_info["priority"])

            # Calculate task period (take the minimum value of all runnable periods)
            min_period = float('inf')
            for timing in task_info["timing"]:
                period = int(float(timing["runnable_period"]) * 1000)  # Convert to microseconds
                min_period = min(min_period, period)

            # Create task configuration
            task_config = {
                "name": task_name,
                "period": min_period,
                "priority": priority,
                "cet": 100  # Keep the default Core Execution Time
            }

            tasks_by_core[core_name].append(task_config)

        # Sort the task list for each core by priority from high to low
        tasks_by_core[core_name].sort(key=lambda x: x["priority"], reverse=True)

def validate_schedule_table_config(all_sch_tbl_cfg: dict) -> bool:
    """Validate the schedule table configuration"""
    if not isinstance(all_sch_tbl_cfg, dict):
        logger.error("Schedule table configuration is not a valid dictionary.")
        return False

    for sch_tbl_name, sch_tbl_data in all_sch_tbl_cfg.items():
        if not isinstance(sch_tbl_data, dict):
            logger.error(f"Schedule table {sch_tbl_name} is not a valid dictionary.")
            return False
        if "task_offset_dict" not in sch_tbl_data:
            logger.error(f"Schedule table {sch_tbl_name} is missing 'task_offset_dict'.")
            return False
    return True

def find_latest_file(directory: str, pattern: str) -> str:
    """
    Find the latest file matching the pattern in the specified directory

    Args:
        directory: Directory path to search
        pattern: File pattern (e.g., *.html or *.log)

    Returns:
        str: Full path of the latest file, returns an empty string if not found
    """
    try:
        # Ensure the directory exists
        if not os.path.exists(directory):
            logger.error(f"Directory not found: {directory}")
            return ""

        # Get all matching files
        matching_files = []
        for file in os.listdir(directory):
            if file.lower().endswith(pattern):
                file_path = os.path.join(directory, file)
                matching_files.append((file_path, os.path.getmtime(file_path)))

        if not matching_files:
            return ""

        # Sort by modification time and return the latest file
        latest_file = max(matching_files, key=lambda x: x[1])[0]
        return latest_file

    except Exception as e:
        logger.error(f"Error finding latest file: {str(e)}")
        return ""

def get_input_file(config_base_path: str, data_source_type: DataSourceType) -> str:
    """
    Get the input file based on the data source type

    Args:
        config_base_path: Base path for configuration files
        data_source_type: Data source type

    Returns:
        str: Input file path

    Raises:
        FileNotFoundError: Raises when the required file is not found
    """
    import_dir = os.path.join(config_base_path, 'Import')

    if not os.path.exists(import_dir):
        os.makedirs(import_dir)
        logger.ui_info(f"Import directory created: {import_dir}")
        raise FileNotFoundError(f"Please add required files to: {import_dir}")
    if data_source_type == DataSourceType.FTRACE_LOG:
        # DataSourceType.FTRACE_LOG
        input_file = find_latest_file(import_dir, '.log')
        if not input_file:
            logger.ui_info("No log files found in Import directory")
            logger.ui_info("Please add ftrace log file (*.log) to:")
            logger.ui_info(import_dir)
            raise FileNotFoundError("Ftrace log file not found")
    if data_source_type == DataSourceType.BINARY_LOG:
        # DataSourceType.BINARY_LOG
        input_file = find_latest_file(import_dir, '.bin')
        if not input_file:
            logger.ui_info("No log files found in Import directory")
            logger.ui_info("Please add binary log file (*.bin) to:")
            logger.ui_info(import_dir)
            raise FileNotFoundError("Binary log file not found")

    return input_file

def jitter_auto_optimization(config_base_path, all_sch_tbl_cfg, task_dict, os_counter_dict,opt):
    """
    Main function for automatic optimization of schedule table configuration

    Args:
        config_base_path: Base path for configuration files
        all_sch_tbl_cfg: Schedule table configuration
        task_dict: Task dictionary

    Returns:
        dict: Optimized jitter configuration
    """
    logger.ui_info("Starting jitter optimization")
    logger.ui_info(f"Data source type: {CURRENT_DATA_SOURCE.name}")

    if not validate_schedule_table_config(all_sch_tbl_cfg):
        logger.error("Invalid schedule table configuration")
        return DEFAULT_CONFIG
    try:
        # Choose parsing method based on global data source type
        if CURRENT_DATA_SOURCE == DataSourceType.FTRACE_LOG:
            ftrace_parse_start_time = time.time()
            try:
                ftrace_file = get_input_file(config_base_path, DataSourceType.FTRACE_LOG)
                logger.ui_info(f"Using Ftrace log file: {ftrace_file}")
                task_params = parse_ftrace_log(ftrace_file)
                organize_tasks = organize_ftrace_tasks
            except FileNotFoundError as e:
                logger.error(str(e))
                return DEFAULT_CONFIG
        elif CURRENT_DATA_SOURCE == DataSourceType.BINARY_LOG:
            ftrace_parse_start_time = time.time()
            try:
                binary_file = get_input_file(config_base_path, DataSourceType.BINARY_LOG)
                logger.ui_info(f"Using binary log file: {binary_file}")
                task_params = parse_binary_data(binary_file)
                organize_tasks = organize_ftrace_tasks
            except FileNotFoundError as e:
                logger.error(str(e))
                return DEFAULT_CONFIG
        else:
            logger.ui_info(f"Date source is not Ftrace file!")

        if not task_params:
            logger.error("No task parameters found in input file")
            return DEFAULT_CONFIG

        extract_and_update_tasks_by_core(task_dict)
        cores_dict = organize_tasks(task_params)
        core_manager = CoreManager(tasks_by_core, cores_dict)

        algorithm_start_time = time.time()
        jitter_cfg = parallel_optimize_cores(core_manager, all_sch_tbl_cfg, os_counter_dict, config_base_path)
        total_time = time.time() - algorithm_start_time

        # Save optimization results to JSON file
        json_output = os.path.join(config_base_path, 'Export', 'jitter_cfg.json')
        try:
            # Ensure output directory exists
            os.makedirs(os.path.dirname(json_output), exist_ok=True)

            with open(json_output, 'w', encoding='utf-8') as f:
                json.dump(jitter_cfg, f, indent=2, ensure_ascii=False)
            logger.ui_info(f"Successfully saved jitter configuration to: {json_output}")

        except IOError as e:
            logger.error(f"Failed to save jitter configuration: {str(e)}")
        except Exception as e:
            logger.error(f"Unexpected error while saving jitter configuration: {str(e)}")

        ftrace_parse_time = algorithm_start_time - ftrace_parse_start_time
        logger.ui_info(f"Ftrace log file parse completed in {ftrace_parse_time:.2f} seconds")
        logger.ui_info(f"Optimization algorithm completed in {total_time:.2f} seconds")
        return jitter_cfg

    except Exception as e:
        logger.error("Jitter optimization failed", exc_info=True)
        return DEFAULT_CONFIG
    finally:
        logger.ui_info("Jitter optimization completed")