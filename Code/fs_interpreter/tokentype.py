from enum import Enum, unique, auto

@unique
class TokenType(Enum):

    # Single-character tokens
    LEFT_BRACK      = auto()
    RIGHT_BRACK     = auto()
    LEFT_PAREN      = auto()
    RIGHT_PAREN     = auto()
    LEFT_BRACE      = auto()
    RIGHT_BRACE     = auto()
    COMMA           = auto() 
    DOT             = auto()
    MINUS           = auto()
    PLUS            = auto()
    SEMICOLON       = auto()
    SLASH           = auto()
    STAR            = auto()

    # One or two character tokens
    BANG            = auto()
    BANG_EQUAL      = auto()
    EQUAL           = auto()
    EQUAL_EQUAL     = auto()
    GREATER         = auto()
    GREATER_EQUAL   = auto()
    LESS            = auto()
    LESS_EQUAL      = auto()
    ARROW           = auto()

    # Literals
    IDENTIFIER      = auto()
    STRING          = auto()
    NUMBER          = auto()

    # Keywords
    GRAPH           = auto()
    DIGRAPH         = auto()
    NODE            = auto()
    EDGE            = auto()
    SUBGRAPH        = auto()
    RANKDIR         = auto()
    LABEL           = auto()
    SHAPE           = auto()
    COLOR           = auto()
    STYLE           = auto()
    FONTSIZE        = auto()
    FLOWSCRIPT      = auto()
    JOB_TYPE        = auto()
    CIRCLE          = auto()
    INPUT           = auto()
    NIL             = auto()
    TEST            = auto()
    IF_TRUE         = auto()
    ELSE            = auto()
    DIAMOND         = auto()

    # End of file token
    EOF             = auto()