from typing import Optional, Dict, Any
from fsToken import Token
from runtimeError import runtimeError

class Environment:
    def __init__(self) -> None:
        self.store: Dict[str, Any] = {}

    def define(self, name: str, value: Any) -> None:
        self.store[name] = value

    def get(self, name: Token) -> Any:
        if name.lexeme in self.store:
            return self.store.get(name.lexeme)

        raise runtimeError(name, f"[Line: {name.line}]: Undefined variable '" + name.lexeme + "'.")
    
    def assign(self, name: Token, value: Any) -> None:
        if name.lexeme in self.store:
            self.values[name.lexeme] = value
            return

        raise runtimeError(name, f"[Line: {name.line}]: Undefined variable '" + name.lexeme + "'.")
    
    # tells me whether there is a variable of the same name already
    # I use it to ensure two jobs don't share the name.
    def exist(self, name: Token):
        return name.lexeme in self.store
    

    def print(self):
        for key, value in self.store.items():
            print(key, value)