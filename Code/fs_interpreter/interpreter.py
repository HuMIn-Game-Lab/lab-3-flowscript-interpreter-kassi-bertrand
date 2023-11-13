import Stmt
import Expr
import flowscript
import json
import ctypes
from ctypes import CFUNCTYPE, c_void_p, POINTER, cdll, c_int, c_char_p, c_char

from typing import List
from runtimeError import runtimeError
from Environment import Environment
from job_sys_functions import *


class Interpreter(Expr.Visitor, Stmt.Visitor):
    def __init__(self):
        self.environment = Environment()
        self.staging_area = {} # Where jobs are placed BEFORE being submitted to the job system

    def interpret(self, statements: List[Stmt.Stmt]):
        self.total = len(statements)
        try:
            for statement in statements:
                self.execute(statement)

            # Move jobs from the staging area to the job system.
            self.schedule_jobs()
        except runtimeError as error:
            flowscript.FlowScript.runtime_error(error)

    # Expressions
    def visit_literal_expr(self, expr: Expr.Literal):
        return expr.value
    
    def visit_variable_expr(self, expr: Expr.Variable):
        return self.environment.get(expr.name)
    
    def visit_assign_expr(self, expr: Expr.Assign):
        value = self.evaluate(expr.value)
        self.environment.assign(expr.name, value)
        return value
    
    # Statements
    def visit_expression_stmt(self, stmt: Stmt.Expression):
        self.evaluate(stmt)
        return None
    
    def visit_var_stmt(self, stmt: Stmt.Var):
        value = None
        if stmt.expr is not None:
            value = self.evaluate(stmt.expr)
        
        self.environment.define(stmt.name.lexeme, value)
        return None
    
    def visit_block_stmt(self, stmt: Stmt.Block):
        self.execute_block(stmt.statements)
        return None
    
    def visit_function_stmt(self, stmt: Stmt.Function):
        self.execute_block(stmt.statements)
        return None

    def visit_dependency_stmt(self, stmt: Stmt.Dependency):
        # For each dependencies
        dependencies = stmt.dependencies
        for dependency in dependencies:
            source = dependency[0]
            target = dependency [1]

            # ensure both identifier exist
            if not self.environment.exist(source):
                raise runtimeError(source, f"[Line: {source.line}]: Job '{source.lexeme}' was never declared. Declare it before you establishing dependency.")
            if not self.environment.exist(target):
                raise runtimeError(source, f"[Line: {target.line}]: {target.lexeme} was never declared. Declare it before you establishing dependency.")
      
        # Add "source" to the "target" dependency list
        # NOTE: Target depends on source
        self.staging_area[target.lexeme]["dependencies"].append(source.lexeme)
        return None

    def visit_conditionaljob_stmt(self, stmt: Stmt.ConditionalJob):
        job = {
            "name": stmt.job_identifier,
            "test_type": stmt.test_type,
            "job_if_true": stmt.if_true_job_id,
            "job_if_false": stmt.else_job_id
        }

        # Ensure conditional job identifier is unique
        if self.environment.exist(job["name"]):
            name = job["name"]
            raise runtimeError(name, f"[Line: {name.line}]: Identifier '{name.lexeme}' is being reused. Identifiers must be unique")
        else:
            self.environment.define(job["name"].lexeme, "CONDITIONAL_JOB")
        
        # Ensure other identifiers have declared before being used in the conditional job declaration
        if not self.environment.exist(job["job_if_true"]):
            name = job["job_if_true"]
            raise runtimeError(name, f"[Line: {name.line}]: Job identifier '{name.lexeme}' has never been declared. Make sure to declare it before referring to it.")
        
        if not self.environment.exist(job["job_if_false"]):
            name = job["job_if_false"]
            raise runtimeError(name, f"[Line: {name.line}]: Job identifier '{name.lexeme}' has never been declared. Make sure to declare it before referring to it.")
        
        # TODO Store the job in the hub
        return None
    
    def visit_jobdeclaration_stmt(self, stmt: Stmt.JobDeclaration):
        job = {
            "name": stmt.job_identifier,
            "type": stmt.job_type,
            "input": stmt.input
        }
        
        # If a job with the same identifier has declared already...
        if self.environment.exist(job["name"]):
            name = job["name"]
            raise runtimeError(name, f"[Line: {name.line}]: Identifier '{name.lexeme}' is being reused. Identifiers must be unique")
        else:
            self.environment.define(job["name"].lexeme, "COMPILE_JOB")
        
        # After resolving identifier name, resolve variables in the statement as well.
        json_input: str = self.evaluate(job["input"])

        # Ensure, that the input is valid JSON
        try:
            json.loads(json_input)
        except ValueError:
            job_name = job["name"].lexeme
            line_number = job["name"].line
            raise runtimeError(job["input"], f"[Line: {line_number}]: The input of job '{job_name}' must be a valid JSON string")
        
        # Place the job in the "staging area"
        tmp_dict = {
            #"identifier": job["type"].lexeme.encode('utf-8'),
            "type": job["type"].literal.encode('utf-8'),
            "dependencies": [], # will put string identifier of jobs here later
            "input": json_input.encode('utf-8') + b'\0'
        }
        
        self.staging_area[ job["name"].lexeme ] = tmp_dict

        return None
    
    # CLASS HELPERS
    def evaluate(self, expr: Expr.Expr):
        return expr.accept(self)
    
    def execute(self, statement: Stmt.Stmt):
        statement.accept(self)

    def execute_block(self, statements: Stmt.Block):
        for statement in statements:
            self.execute(statement)

    # TO BE DELETED
    def print(self):
        self.environment.print()


    # submit jobs to the job system
    def schedule_jobs(self):
        job_handles = {}

        # Kick off the job system
        job_system_handle = get_job_system_instance()
        init_job_system()

        # Create all job the jobs
        for job_id_string, job_infos in self.staging_area.items():
            job_identifier_cstr = ctypes.c_char_p(job_infos["type"])     
            job_handle = create_job_func(job_system_handle, job_identifier_cstr, job_infos["input"])
            
            job_handles[job_id_string] = job_handle

        # Attach dependencies
        for job_id_string, job_infos in self.staging_area.items():
            job_handle = job_handles[job_id_string]
            
            dependencies = job_infos["dependencies"]
            for dep_id in dependencies:
                dep_handle = job_handles[dep_id]

                # NOTE: second job IS dependent on the first job.
                add_dependency(job_handle, dep_handle)

        # Submit all to the job system
        print("\nInterpreter submitting jobs (˵ ͡° ͜ʖ ͡°˵): \n")
        for job_id_string, job_handle in job_handles.items():
            queue_job(job_system_handle, job_handle)
            print(f"Job {job_id_string} SUBMITTED to the JOB SYSTEM") 
        print("\n")
        print("Your jobs are running. Interact with the job system to manipulate them ╰( ͡° ͜ʖ ͡° )つ──☆*: \n")

        # The interpreter is DONE, a the job system interface for the user to interact
        # with the job system and see their jobs
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

    