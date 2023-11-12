import Stmt
import Expr
import flowscript

from typing import List
from runtimeError import runtimeError
from Environment import Environment


class Interpreter(Expr.Visitor, Stmt.Visitor):
    def __init__(self):
        self.environment = Environment()

    def interpret(self, statements: List[Stmt.Stmt]):
        self.total = len(statements)
        try:
            for statement in statements:
                self.execute(statement)

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
      
        
        # TODO: in hub Add source to target dependency list
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
        
        # TODO Store the job in the hub
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
        pass