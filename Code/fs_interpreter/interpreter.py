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
        # Check if both name exist
        # if not raise runtime error
        
        # in hub Add source to target dependency list
        print("Depdency declared")
        pass

    def visit_conditionaljob_stmt(self, stmt: Stmt.ConditionalJob):
        # Check if identifer does not already exist
        # register it in the environemnt
        # Store the job in the hub
        print("Conditional job declared")
        pass
    
    def visit_jobdeclaration_stmt(self, stmt: Stmt.JobDeclaration):
        # TODO Check if job name is does not already exist
        # Raise runtime error if it does
        # TODO Register the name of the job in the environment
        # Store the job in the hub
        return None
    
    # CLASS HELPERS
    def evaluate(self, expr: Expr.Expr):
        return expr.accept(self)
    
    def execute(self, statement: Stmt.Stmt):
        statement.accept(self)

    def execute_block(self, statements: Stmt.Block):
        for statement in statements:
            self.execute(statement)

    def print(self):
        self.environment.print()
    
    
    # submit jobs to the job system
    def schedule_job(self):
        pass