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
import heapq
from functools import reduce
from math import gcd

def lcm(a, b):
    # Calculate the least common multiple of two numbers
    return abs(a * b) // gcd(a, b)

class Task:
    def __init__(self, task_name, priority, period, CET, arrival_time, activite_num=0):
        self.task_name = task_name
        self.priority = priority
        self.period = period
        self.CET = CET
        self.arrival_time = arrival_time
        self.activite_num = activite_num  # In case of the same priority and activation point, the execution order is determined by the order of task addition; smaller ActiviteNum is executed first
        self.execution_time_left = CET
        self.has_execution = False  # Whether it has been executed
        self.success_activite_time = 0  # Successful activation time
        self.rt_less_deadline_cnt = 0  # Count of times RT is less than deadline
        self.rt_max = 0
        self.rt_deadline_missed_cnt = 0
        self.rt_deadline_missed_time_all = 0
        self.run_cnt = 0  # Actual run count

    def __lt__(self, other):
        if self.priority == other.priority:
            assert not (self.has_execution and other.has_execution), "ready_queue same priority task can not have more than one task that has been ipt calculated!!!"
            # If a task is preempted from running, it has limited execution
            if self.has_execution:
                return True
            elif other.has_execution:
                return False
            # If both tasks have not been preempted, execute based on the order of addition
            else:
                return self.activite_num < other.activite_num

class Scheduler:
    def __init__(self, debug_mode=False, context_switch_time=0):
        self.current_time = 0
        self.ready_queue = []
        self.task_list = []
        self.completed_tasks = []
        self.debug_mode = debug_mode
        self.current_task = None
        self.last_task = None
        self.total_ipt = 0  # Initialize total IPT to 0
        self.total_rt = 0  # Initialize total Response time to 0
        self.context_switch_time = context_switch_time  # Task switch time

    def add_task(self, task):
        heapq.heappush(self.task_list, (task.arrival_time, task))
        if self.debug_mode:
            print(f"Task {task.task_name} added with arrival at {task.arrival_time}")

    def run(self, max_time):
        while self.current_time < max_time:
            # Move tasks that are due from the task list to the ready queue
            while self.task_list and self.task_list[0][0] <= self.current_time:
                _, task = heapq.heappop(self.task_list)
                # Record successful activation time
                task.success_activite_time = self.current_time
                heapq.heappush(self.ready_queue, (-task.priority, task))
                if self.debug_mode:
                    print(f"Task {task.task_name} is ready at time {self.current_time}")

            # Elect the task to run from the priority queue
            if self.ready_queue:
                _, self.current_task = heapq.heappop(self.ready_queue)  # Get the task to run

            # If there is context switch time
            if self.context_switch_time > 0 and self.last_task != self.current_task:
                if self.debug_mode:
                    print(f"context switch from time {self.current_time} to time {self.current_time + self.context_switch_time}")
                self.last_task = self.current_task
                self.current_time += self.context_switch_time
                if self.current_task:
                    heapq.heappush(self.ready_queue, (-self.current_task.priority, self.current_task))  # Put the task back into the ready queue
                continue

            if self.current_task:
                # Calculate IPT time
                if not self.current_task.has_execution:
                    self.current_task.has_execution = True
                    self.total_ipt += (self.current_time - self.current_task.arrival_time)

                self.current_task.execution_time_left -= 1
                self.last_task = self.current_task

                if self.debug_mode:
                    print(f"Task {self.current_task.task_name} is executing at time {self.current_time}")
                if self.current_task.execution_time_left == 0:
                    # Task execution completed
                    if self.debug_mode:
                        print(f"Task {self.current_task.task_name} completed at time {self.current_time}")
                    self.completed_tasks.append((self.current_task.task_name, self.current_time))
                    self.total_rt += (self.current_time + 1 - self.current_task.arrival_time)
                    self.current_task.run_cnt += 1
                    cur_rt = self.current_time - self.current_task.success_activite_time + 1

                    if self.current_task.rt_max < cur_rt:
                        self.current_task.rt_max = cur_rt
                    if cur_rt <= self.current_task.period:
                        self.current_task.rt_less_deadline_cnt += 1
                    else:
                        self.current_task.rt_deadline_missed_cnt += 1
                        self.current_task.rt_deadline_missed_time_all += (cur_rt - self.current_task.period)

                    if self.current_task.period > 0:
                        # Re-schedule the task based on its period
                        next_activation_time = self.current_task.arrival_time + self.current_task.period
                        while next_activation_time <= self.current_time:  # Activation point has expired, consider this activation lost, default all task ActivationLimit is 1
                            next_activation_time += self.current_task.period
                        if next_activation_time < max_time:
                            """ Reset some running state variables """
                            self.current_task.arrival_time = next_activation_time
                            self.current_task.has_execution = False
                            self.current_task.success_activite_time = 0
                            self.current_task.execution_time_left = self.current_task.CET
                            # Re-add to the task queue
                            heapq.heappush(self.task_list, (next_activation_time, self.current_task))
                    self.current_task = None
            else:
                self.last_task = None
            # Put the current task back into the ready queue
            if self.current_task is not None:
                heapq.heappush(self.ready_queue, (-self.current_task.priority, self.current_task))
                self.current_task = None
            self.current_time += 1

    def print_total_ipt(self):
        print(f"Total Initial Pending Time (IPT) for all tasks: {self.total_ipt}")

    def print_total_rt(self):
        print(f"Total Response Time (RT) for all tasks: {self.total_rt}")

    def calculate_max_time(self, tasks):
        # Filter out tasks with a period of 0 or negative, and convert periods to integers
        periods = [int(round(task.period)) for task in tasks if task.period > 0]

        if not periods:  # If there are no valid period values
            print(f"No valid period value, returns default value 1000")
            return 1000  # Return a default value
        try:
            """ Take twice the least common multiple of the simulation time. """
            return reduce(lcm, periods) * 2
        except Exception as e:
            print(f"Error calculating max time: {e}")
            print(f"Periods: {periods}")
            return 1000