from abc import ABC, abstractmethod
from typing import List

class Visitor(ABC):
	@abstractmethod
	def visit_assign_expr(self, expr):
		pass

	@abstractmethod
	def visit_literal_expr(self, expr):
		pass

	@abstractmethod
	def visit_variable_expr(self, expr):
		pass


class Expr(ABC):
	@abstractmethod
	def accept(self, visitor: Visitor):
		pass

class Assign(Expr):
	def __init__(self, name, value):
		self.name = name
		self.value = value
	
	def accept(self, visitor: Visitor):
		return visitor.visit_assign_expr(self)

class Literal(Expr):
	def __init__(self, value):
		self.value = value
	
	def accept(self, visitor: Visitor):
		return visitor.visit_literal_expr(self)

class Variable(Expr):
	def __init__(self, name):
		self.name = name
	
	def accept(self, visitor: Visitor):
		return visitor.visit_variable_expr(self)

