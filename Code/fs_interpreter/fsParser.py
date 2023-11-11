import sys
import flowscript
import Stmt
import Expr

from typing import List
from fsToken import Token
from tokentype import TokenType


class Parser:

    class ParseError(RuntimeError):
        pass
    
    def __init__(self, tokens: List[Token]) -> None:
        self.tokens = tokens
        self.current = 0

    def parse(self):
        statements = []
        while not self.is_at_end():
            statements.append(self.declaration())
        return statements
    
    # GRAMMAR RULES implementation for EXPRESSIONS
    def assignment(self):
        expr: Expr = self.equality()

        if self.match(TokenType.EQUAL):
            equals: Token = self.previous()
            value: Expr = self.assignment()

            if isinstance(expr, Expr.Variable):
                name = expr.name
                return Expr.Assign(name, value)
            
            self.error(equals, "Invalid assignment target.")
        
        return expr
    
    def equality(self):
        expr: Expr = self.primary()
        return expr
    
    def primary(self):
        if self.match(TokenType.NIL):
            return Expr.Literal(None)
        
        if self.match(TokenType.STRING):
            return Expr.Literal(self.previous().literal)
        
        if self.match(TokenType.IDENTIFIER):
            return Expr.Variable(self.previous())
        
        # Raise a Parse Error we are dealing with something we don't know
        raise self.error(self.peek(), "Unexpected expression.")


    # GRAMMAR RULES implementation for STATEMENTS
    def declaration(self):
        try:
            # This is the program entry point.
            if self.match(TokenType.DIGRAPH):
                return self.flow_script_entry_point()
            
            # At this point, we don't know which type of statement. So, we check
            if self.match(TokenType.IDENTIFIER):
                
                if self.check(TokenType.LEFT_BRACK):
                    # This is a job declaration statement
                    return self.job_declaration() 
                elif self.check(TokenType.EQUAL):
                    # This is variable declaration
                    return self.var_declaration_assignment()
                elif self.check(TokenType.ARROW):
                    return self.dependency_statement()
                else:
                    raise self.error(self.previous(), "FlowScript does not allow identifier to be by themselves. They must be assigned a value if they are a variable, or used to identify a job")

            # This is a function declaraction
            if self.match(TokenType.SUBGRAPH):
                return self.func_decl_statement()
                    
            return self.statement()
        
        except self.ParseError as error:
            # Parser enters Panic mode, synchronize
            self.synchronize()
            return None

    def flow_script_entry_point(self):
        # self.consume(TokenType.DIGRAPH, f"Expect 'digraph' keyword")
        self.consume_on_same_line(TokenType.FLOWSCRIPT, f"The entry point of FlowScript should be named 'FlowScript', you wrote {self.tokens[self.current].lexeme}")
        self.consume(TokenType.LEFT_BRACE, "Opening brace expected")
        return Stmt.Block(self.block())
    
    def job_declaration(self):
        job_id: Token = self.previous()
        self.consume(TokenType.LEFT_BRACK, "When declaring a job '[' must follow the job identifier.")

        # Parse JobType
        self.consume(TokenType.JOB_TYPE, "Expect the 'jobType' keyword.")
        self.consume(TokenType.EQUAL, "Expect '=' after 'jobType'.")
        job_type = self.consume(TokenType.STRING, "Expect registered type after 'jobType'.")

        if job_type.literal == "CONDITIONAL":
            return self.conditional_job_declaration(job_id)
        
        # Parse shape
        self.consume(TokenType.SHAPE, "Expect 'shape' keyword.")
        self.consume(TokenType.EQUAL, "Expect '=' after 'shape'.")
        self.consume(TokenType.CIRCLE, "Registered jobs must have a 'circle' shape.")

        # Parsing input
        self.consume(TokenType.INPUT, "Expect 'input' parameter. When declared Jobs must be given an JSON input")
        self.consume(TokenType.EQUAL, "Expect '=' after 'input'.")
        input = self.assignment() # input can of type 'Expr.Variable' , or a 'STRING', but we take it as is :)
        self.consume(TokenType.RIGHT_BRACK, "Expect a closing ']' after the input")
        self.consume(TokenType.SEMICOLON, "Semicolon expected at the end of statement.")

        return Stmt.JobDeclaration(job_id, job_type, input)

    def conditional_job_declaration(self, job_id: Token):
        # Parse shape for conditional jobs
        self.consume(TokenType.SHAPE, "Expect 'shape' keyword.")
        self.consume(TokenType.EQUAL, "Expect '=' after 'shape'.")
        self.consume(TokenType.DIAMOND, "Conditional jobs must have a 'diamond' shape.")

        # Parse the test type for conditional jobs
        self.consume(TokenType.TEST, "Expect 'testType' keyword.")
        self.consume(TokenType.EQUAL, "Expect '=' after 'testType'.")
        test_type = self.consume(TokenType.STRING, "Expect string for 'testType'.")

        # Parse the if_true part
        self.consume(TokenType.IF_TRUE, "Expect 'if_true' keyword.")
        self.consume(TokenType.EQUAL, "Expect '=' after 'if_true'.")
        if_true_job_id = self.assignment()

        # parse the else part
        self.consume(TokenType.ELSE, "Expect 'else' keyword.")
        self.consume(TokenType.EQUAL, "Expect '=' after 'else'.")
        else_job_id = self.assignment()

        self.consume(TokenType.RIGHT_BRACK, "Expect a closing ']' after the conditional job declaration.")
        self.consume(TokenType.SEMICOLON, "Semicolon expected at the end of statement.")

        return Stmt.ConditionalJob(job_id, if_true_job_id, else_job_id)

    def var_declaration_assignment(self):
        identifier: Token = self.previous()
        self.consume(TokenType.EQUAL, "Expect '='. Variables must be initialized")
        initializer: Expr = self.assignment() # initializer should be of type 'STRING' or 'Variable'
        self.consume(TokenType.SEMICOLON, "Expect ';' at the end of statement")        
        return Stmt.Var(identifier, initializer)

    def func_decl_statement(self):
        # Parse the subgraph identifier
        subgraph_identifier = self.consume_on_same_line(TokenType.IDENTIFIER, "Expect identifier after 'subgraph'")
        self.consume(TokenType.LEFT_BRACE, "Expect a '{' after identifier" + f" \"{subgraph_identifier.lexeme}\" ")

        # Parse all the statements in the body of the subgraph
        statements: List[Stmt.Stmt] = self.block()

        # A function is essentially just a block, BUT with a name. So, it will be treated by the interpreter as a block
        # I chose the return a dedicated object to capture some additional information.
        return Stmt.Function(subgraph_identifier, statements)

    def dependency_statement(self):
        dependencies = []

        # The first element in the dependencies chain have been parsed already. Let's add it.
        source = self.previous()

        while self.match(TokenType.ARROW):
            # Parse the next identifier after each "->"
            target = self.consume(TokenType.IDENTIFIER, "Expect target identifier in dependency")
            dependencies.append((source, target))
            source = target # Update source for the next iteration
        
        self.consume(TokenType.SEMICOLON, "Expect ';' after dependency declaration statement")

        # Return the statement object with all the dependencies arranged as tuples
        return Stmt.Dependency(dependencies)

    def statement(self):
        if self.match(TokenType.LEFT_BRACE):
            return Stmt.Block(self.block()) 
        return self.expression_statement()
    
    def expression_statement(self):
        expr: Expr = self.assignment()
        self.consume(TokenType.SEMICOLON, "Expect ';' after the expression")
        return Stmt.Expression(expr)
    
    def block(self) -> List[Stmt.Stmt]:
        statements = []

        while not self.check(TokenType.RIGHT_BRACE) and not self.is_at_end():
            statements.append(self.declaration())
        

        self.consume(TokenType.RIGHT_BRACE, "Expect '}' after block")
        return statements
    
    # CLASS HELPERS
    
    # Does the the current token has any of the given types?
    def match(self, *types: TokenType):
        for token_type in types:
            if self.check(token_type):
                self.advance()
                return True
        
        return False
    
    # Is the current token the expected type? Make advance. If no... error and show message
    def consume(self, token_type: TokenType, message: str):
        if self.check(token_type):
            return self.advance()
        
        raise self.error(self.previous(), message)
    
    # consume, but ensures that the token to be consumed is on the same line as the previous
    def consume_on_same_line(self, token_type: TokenType, message: str):
        if self.check(token_type) and self.previous().line == self.peek().line:
            return self.advance()

        raise self.error(self.previous(), message)

    
    def check(self, token_type: TokenType):
        if self.is_at_end():
            return False
        
        return self.peek().type == token_type
    
    def advance(self) -> Token:
        if not self.is_at_end():
            self.current += 1
        
        return self.previous()
    
    def is_at_end(self) -> bool:
        return self.peek().type == TokenType.EOF
    
    # Give me the current token
    def peek(self) -> Token:
        return self.tokens[self.current]
    
    def peek_next(self) -> Token:
        return self.tokens[self.current + 1]
    
    # Give me the token before the current token
    def previous(self) -> Token:
        return self.tokens[self.current - 1]
    
    def error(self, token, message):
        flowscript.FlowScript.error_token(token, message)
        return self.ParseError()
    
    # discard tokens until we’re right at the beginning of the next statement.
    # NOTE: A statement are delimited by semicolons, but sometimes the user might forget them
    # in such case, the parser will look for a opening/closing brace
    def synchronize(self):
        self.advance()

        while not self.is_at_end():
            
            if self.peek().type == TokenType.SUBGRAPH and self.peek_next().type == TokenType.IDENTIFIER:
                return
            
            elif self.peek().type == TokenType.IDENTIFIER and self.peek_next().type == TokenType.LEFT_BRACK:
                return
            
            elif self.peek().type == TokenType.IDENTIFIER and self.peek_next().type == TokenType.ARROW:
                return
            
            elif self.peek().type == TokenType.RIGHT_BRACE and self.peek_next().type == TokenType.EOF:
                return

            
            self.advance()