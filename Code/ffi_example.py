# Example of using the job system from python
import os
import ctypes
import json
from ctypes import CFUNCTYPE, c_void_p, POINTER, cdll, c_int, c_char_p, c_char

# Define the JobSystemHandle as a ctypes void pointer
JobSystemHandle = ctypes.c_void_p

# Define the JobHandle as a ctypes void pointer
JobHandle = ctypes.c_void_p

script_dir = os.path.dirname(os.path.abspath(__file__))

# Specify the relative path from the script directory to the shared library
lib_path = os.path.join(script_dir, 'lib/libjobsystem.so')

# Load the shared library containing Job and JobSystem classes
job_system_lib = ctypes.CDLL(lib_path)

# Function to get a JobSystem instance
get_job_system_instance = job_system_lib.GetJobSystemInstance
get_job_system_instance.restype = JobSystemHandle

# Function to init and destroy the job system
init_job_system = job_system_lib.InitJobSystem
destroy_job_system = job_system_lib.DestroyJobSystemInstance
destroy_job_system.argtypes = [JobSystemHandle]
destroy_job_system.restype = c_void_p

# Function to create job
create_job = job_system_lib.CreateJob
CreateJobFunc = CFUNCTYPE(c_void_p, c_void_p, c_char_p, c_char_p)
create_job_func = CreateJobFunc(create_job)
create_job.argtypes = [JobSystemHandle, ctypes.c_char_p, ctypes.c_char_p]
create_job.restype = JobHandle

# Function to queue a job
queue_job = job_system_lib.QueueJob
queue_job.argtypes = [JobSystemHandle, JobHandle]

# Function to add dependency
add_dependency = job_system_lib.AddDependency
add_dependency.argtypes = [JobHandle, JobHandle]

# Function to finish a job
finish_job = job_system_lib.FinishJob
finish_job.argtypes = [JobSystemHandle, ctypes.c_int]

# Function to get job status
get_job_status = job_system_lib.GetJobStatus
get_job_status.argtypes = [JobSystemHandle, ctypes.c_int]
get_job_status.restype = ctypes.c_int

# Function to get job id
get_job_id = job_system_lib.GetJobID
get_job_id.argtypes = [JobSystemHandle, ctypes.c_int]
get_job_id.restype = ctypes.c_int

# DEFINE the function signature for FinishCompleted jobs and WRAP it with the function signature
finish_jobs = job_system_lib.FinishCompletedJobs
FinishJobsFunc = CFUNCTYPE(c_void_p, c_void_p)
finish_jobs_func = FinishJobsFunc(finish_jobs)
finish_jobs.argtypes = [JobSystemHandle]
finish_jobs.restype = ctypes.c_void_p

# Function to display details
get_job_details = job_system_lib.GetJobDetails
get_job_details.argtypes = [JobSystemHandle]



if __name__ == "__main__":

    # Get the JobSystem instance from the shared library
    job_system_handle = get_job_system_instance()

    # initialize job system
    init_job_system()

    # Creating a compile job
    compile_input = {
        "jobChannels": 268435456,
        "jobType": 1,
        "makefile": "./Data/testCode/Makefile",
        "isFilePath": True
    }

    compile_json_string = json.dumps(compile_input)
    compile_json_data = compile_json_string.encode('utf-8') + b'\0'

    compile_job_identifier_bytes = "COMPILE_JOB".encode('utf-8')
    compile_job_identifier_cstr = ctypes.c_char_p(compile_job_identifier_bytes)

    compile_job = create_job_func(job_system_handle, compile_job_identifier_cstr, compile_json_data)

    # Creating a parsing job
    parsing_input = {
        "jobChannels": 536870912,
        "jobType": 2,
        "content": ""
    }
    
    parsing_json_string = json.dumps(parsing_input)
    parsing_json_data = parsing_json_string.encode('utf-8') + b'\0'

    parsing_job_identifier = "PARSING_JOB".encode('utf-8')
    parsing_job_identifier_cstr = ctypes.c_char_p(parsing_job_identifier)

    parsing_job = create_job_func(job_system_handle, parsing_job_identifier_cstr, parsing_json_data)

    # Add dependency
    add_dependency(parsing_job, compile_job)

    # Queuing the jobs
    queue_job(job_system_handle, compile_job)
    queue_job(job_system_handle, parsing_job)

    # Display job distails
    # NOTE: Will display an empty table because no job is running.
    running = True

    while running:
        command = input("Enter: \"stop\", \"destroy\", \"finish\", \"status\", \"finishjob\", or \"job_types\", \"history\":\n")
        
        if command == "stop":
            running = False
        elif command == "destroy":
            finish_jobs(job_system_handle)
            destroy_job_system(job_system_handle)
            running = False
        elif command == "finish":
            finish_jobs(job_system_handle)
        elif command == "finishjob":
            try:
                jobID = int(input("Enter ID of job to finish: "))
                finish_job(job_system_handle, jobID)
            except ValueError:
                print("Invalid input. Please enter a valid job ID.")
        elif command == "history":
            get_job_details(job_system_handle)
        else:
            print("Invalid command")

