from abc import ABC, abstractmethod
from typing import List

class Visitor(ABC):
	@abstractmethod
	def visit_block_stmt(self, stmt):
		pass

	@abstractmethod
	def visit_expression_stmt(self, stmt):
		pass

	@abstractmethod
	def visit_function_stmt(self, stmt):
		pass

	@abstractmethod
	def visit_var_stmt(self, stmt):
		pass

	@abstractmethod
	def visit_jobdeclaration_stmt(self, stmt):
		pass

	@abstractmethod
	def visit_conditionaljob_stmt(self, stmt):
		pass

	@abstractmethod
	def visit_dependency_stmt(self, stmt):
		pass


class Stmt(ABC):
	@abstractmethod
	def accept(self, visitor: Visitor):
		pass

class Block(Stmt):
	def __init__(self, statements):
		self.statements = statements
	
	def accept(self, visitor: Visitor):
		return visitor.visit_block_stmt(self)

class Expression(Stmt):
	def __init__(self, expression):
		self.expression = expression
	
	def accept(self, visitor: Visitor):
		return visitor.visit_expression_stmt(self)

class Function(Stmt):
	def __init__(self, name, statements):
		self.name = name
		self.statements = statements
	
	def accept(self, visitor: Visitor):
		return visitor.visit_function_stmt(self)

class Var(Stmt):
	def __init__(self, name, expr):
		self.name = name
		self.expr = expr
	
	def accept(self, visitor: Visitor):
		return visitor.visit_var_stmt(self)

class JobDeclaration(Stmt):
	def __init__(self, job_identifier, job_type, input):
		self.job_identifier = job_identifier
		self.job_type = job_type
		self.input = input
	
	def accept(self, visitor: Visitor):
		return visitor.visit_jobdeclaration_stmt(self)

class ConditionalJob(Stmt):
	def __init__(self, job_identifier, if_true_job_id, else_job_id):
		self.job_identifier = job_identifier
		self.if_true_job_id = if_true_job_id
		self.else_job_id = else_job_id
	
	def accept(self, visitor: Visitor):
		return visitor.visit_conditionaljob_stmt(self)

class Dependency(Stmt):
	def __init__(self, dependencies):
		self.dependencies = dependencies
	
	def accept(self, visitor: Visitor):
		return visitor.visit_dependency_stmt(self)

