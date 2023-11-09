# Example of using the job system from python
import os
import ctypes
import json

# Define the JobSystemHandle as a ctypes void pointer
JobSystemHandle = ctypes.c_void_p

# Define the JobHandle as a ctypes void pointer
JobHandle = ctypes.c_void_p

script_dir = os.path.dirname(os.path.abspath(__file__))

# Specify the relative path from the script directory to the shared library
lib_path = os.path.join(script_dir, '../lib/libjobsystem.so')

# Load the shared library containing Job and JobSystem classes
job_system_lib = ctypes.CDLL(lib_path)

# Function to get a JobSystem instance
get_job_system_instance = job_system_lib.GetJobSystemInstance
get_job_system_instance.restype = JobSystemHandle

# Function to create a job
create_job = job_system_lib.CreateJob
create_job.argtypes = [JobSystemHandle, ctypes.c_char_p, ctypes.c_char_p]
create_job.restype = JobHandle

# Function to finish a job
finish_job = job_system_lib.FinishJob
finish_job.argtypes = [JobSystemHandle, ctypes.c_int]

# Function to queue a job
queue_job = job_system_lib.QueueJob
queue_job.argtypes = [JobSystemHandle, JobHandle]

# Function to get job status
get_job_status = job_system_lib.GetJobStatus
get_job_status.argtypes = [JobSystemHandle, ctypes.c_int]
get_job_status.restype = ctypes.c_int

# Function to display details
get_job_details = job_system_lib.GetJobDetails
get_job_details.argtypes = [JobSystemHandle]


if __name__ == "__main__":

    # Get the JobSystem instance from the shared library
    job_system_handle = get_job_system_instance()

    python_dict = {
        "jobChannels": 268435456,
        "jobType": 1,
        "makefile": "./Data/testCode/Makefile",
        "isFilePath": True
    }

    json_string = json.dumps(python_dict)
    json_bytes = json_string.encode('utf-8')
    json_data_with_null_terminator = json_bytes + b'\0'

    # Attempt to Create Compile Job
    # NOTE: The following will NOT WORK and returns a message saying "COMPILE_JOB" was not registered
    # But it is to show that the "create_job" function is accessible, and can be called.
    compile_job = create_job(job_system_handle, b"COMPILE_JOB", json_data_with_null_terminator)

    # NOTE: In the scenario where we want to be able to REGISTER, and CREATE a job from this Python file, I would:
    # - Create and expose a function that given the input JSON return an instance of the job
    # - Compile the compile_job source file as a shared library, and make sure the function of the previous step is visible
    # - From this python file, I would import both shared libraries... the job system's and the compile job's
    # - Still from this python file, I would call the expose register job type, and pass the function discussed in step 1
    # - With the registration done, I would create the job from python, and submit it to the job system

    # Display job distails
    # NOTE: Will display an empty table because no job is running.
    get_job_details(job_system_handle)
