from tokentype import TokenType
from fsToken import Token
from typing import List, Optional
import flowscript

class Scanner:
    # keyword-map
    keywords = {
        "graph"     : TokenType.GRAPH,
        "digraph"   : TokenType.DIGRAPH,
        "node"      : TokenType.NODE,
        "edge"      : TokenType.EDGE,
        "subgraph"  : TokenType.SUBGRAPH,
        "rankdir"   : TokenType.RANKDIR,
        "label"     : TokenType.LABEL,
        "shape"     : TokenType.SHAPE,
        "color"     : TokenType.COLOR,
        "style"     : TokenType.STYLE,
        "fontsize"  : TokenType.FONTSIZE,
        "FlowScript": TokenType.FLOWSCRIPT,
        "jobType"   : TokenType.JOB_TYPE,
        "circle"    : TokenType.CIRCLE,
        "input"     : TokenType.INPUT,
        "nil"       : TokenType.NIL,
        "test"      : TokenType.TEST,
        "if_true"   : TokenType.IF_TRUE,
        "else"      : TokenType.ELSE,
        "diamond"   : TokenType.DIAMOND
    }

    def __init__(self, source: str):
        self.source: str            = source
        self.tokens: List[Token]    = []
        self.start: int             = 0
        self.current: int           = 0
        self.line: int              = 1

    def scan_tokens(self) -> List[Token]:
        while not self.is_at_end():
            self.start = self.current
            self.scan_token()
        
        self.tokens.append(Token(TokenType.EOF, "", None, self.line))
        return self.tokens

    def is_at_end(self) -> bool:
        return self.current >= len(self.source)
    
    def scan_token(self) -> None:
        c = self.advance()

        # Handle lexeme that are single character long in FlowScript
        if c == '[':
            self.add_token(TokenType.LEFT_BRACK)
        elif c == ']':
            self.add_token(TokenType.RIGHT_BRACK)
        elif c == '(':
            self.add_token(TokenType.LEFT_PAREN)
        elif c == ')':
            self.add_token(TokenType.RIGHT_PAREN)
        elif c == '{':
            self.add_token(TokenType.LEFT_BRACE)
        elif c == '}':
            self.add_token(TokenType.RIGHT_BRACE)
        elif c == ',':
            self.add_token(TokenType.COMMA)
        elif c == '.':
            self.add_token(TokenType.DOT)
        elif c == '+':
            self.add_token(TokenType.PLUS)
        elif c == ';':
            self.add_token(TokenType.SEMICOLON)
        elif c == '*':
            self.add_token(TokenType.STAR)
        
        # Handle lexeme that can be followed by another
        elif c == '-':
            self.add_token(TokenType.ARROW if self.match('>') else TokenType.MINUS)
        elif c == '!':
            self.add_token(TokenType.BANG_EQUAL if self.match('=') else TokenType.BANG)
        elif c == '=':
            self.add_token(TokenType.EQUAL_EQUAL if self.match('=') else TokenType.EQUAL)
        elif c == '<':
            self.add_token(TokenType.LESS_EQUAL if self.match('=') else TokenType.LESS)
        elif c == '>':
            self.add_token(TokenType.GREATER_EQUAL if self.match('=') else TokenType.GREATER)
        
        # Handle lexeme that are lonnnnng like (like a comment ¯\_(ツ)_/¯) OR whitespace
        elif c == '/':
            if self.match('/'):
                while self.peek() != '\n' and not self.is_at_end():
                    self.advance() # A comment goes until the end of the line.
            else:
                self.add_token(TokenType.SLASH)
        
        # Ignore whitespaces altogether
        elif c in {' ', '\r', '\t'}:
            pass
        
        elif c == '\n':
            self.line += 1
        elif c == '"':
            self.string()
        else:
            # Handling Number literals
            if self.is_digit(c):
                self.number()
            elif self.is_alpha(c):
                self.identifier()
            else:
                flowscript.FlowScript.error(self.line, f"'{c}' is an unexpected character.")

    def advance(self) -> None:
        char = self.source[self.current]
        self.current += 1
        return char

    def add_token(self, token_type: TokenType, literal: Optional[object] = None) -> None:
        text = self.source[self.start: self.current]
        self.tokens.append(Token(token_type, text, literal, self.line))

    def match(self, expected: str) -> bool:
        if self.is_at_end():
            return False
        if self.source[ self.current ] != expected:
            return False
        
        self.current += 1
        return True
    
    # Returns the current character, but does NOT advance
    def peek(self) -> str:
        if self.is_at_end():
            return '\0'
        return self.source[ self.current ]
    
    # Just like "peek" but look at the next character. This is one a character lookahead
    def peek_next(self) -> str:
        if self.current + 1 >= len(self.source):
            return '\0'
        
        return self.source[ self.current + 1 ]
    
    def is_digit(self, c: str) -> bool:
        return '0' <= c <= '9'
    
    def string(self) -> None:
        while self.peek() != '"' and not self.is_at_end():
            if self.peek() == '\n':
                self.line += 1
            
            self.advance()
        
        if self.is_at_end():
            flowscript.FlowScript.error(self.line, "Unterminated string.")
            return
        
        # The closing "
        self.advance()

        # Trim the surrounding quotes.
        value =  self.source[ self.start + 1: self.current - 1 ]
        self.add_token(TokenType.STRING, value)

    def number(self) -> None:
        # as long as the current char is a number
        while self.is_digit(self.peek()):
            self.advance()
        
        # if we meet a DOT, and the next char is a digit... then it's a fractional number
        if self.peek() == '.' and self.is_digit(self.peek_next()):
            self.advance()

            # consume the remaining digits
            while self.is_digit(self.peek()):
                self.advance()
        
        # Add the token
        value = float(self.source[ self.start: self.current ])
        self.add_token(TokenType.NUMBER, value)

    def identifier(self) -> None:
        while self.is_alpha_numeric(self.peek()):
            self.advance()
        
        text = self.source[ self.start: self.current ]
        token_type = self.keywords.get(text)
        
        if token_type is None:
            token_type = TokenType.IDENTIFIER
        
        self.add_token(token_type)

    def is_alpha(self, c: str):
        return ('a' <= c <= 'z') or ('A' <= c <= 'Z') or c == '_'
    
    def is_alpha_numeric(self, c: str) -> bool:
        return self.is_alpha(c) or self.is_digit(c)
        