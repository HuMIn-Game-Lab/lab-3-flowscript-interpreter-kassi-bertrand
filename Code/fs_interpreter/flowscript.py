import sys
import argparse

import scanner
import fsParser
from fsToken import Token
from tokentype import TokenType
from runtimeError import runtimeError
from interpreter import Interpreter

class FlowScript:

    had_error = False
    had_runtime_error = False

    @staticmethod
    def main():
        parser = argparse.ArgumentParser(description="Run the Lox interpreter.")
        parser.add_argument("script", nargs="?", help="Path to the Lox script to execute.")
        args = parser.parse_args()

        if args.script:
            FlowScript.run_file(args.script)

    @staticmethod
    def run_file(path: str):
        with open(path, 'r') as file:
            source = file.read()
        FlowScript.run(source)

        if FlowScript.had_error:
            sys.exit(65)
        if FlowScript.had_runtime_error:
            sys.exit(70)
    
    
    def run(source: str):
        lexer = scanner.Scanner(source)
        tokens = lexer.scan_tokens()

        parser = fsParser.Parser(tokens)
        statements = parser.parse()

        if FlowScript.had_error:
            return
        
        # for token in tokens:
        #     print(token)

        # At this point, the parser have IDENTIFIED All TYPE of statements the user have typed,
        # Now, it is the responsibility of the interpreter to 'execute' those statements.
        interpreter = Interpreter()
        interpreter.interpret(statements)
        interpreter.print()
        

    @staticmethod
    def error(line: int, message: str):
        FlowScript.report(line, "", message)

    @staticmethod
    def error_token(token: Token, message: str):
        if token.type == TokenType.EOF:
            FlowScript.report(token.line, " at end", message)
        else:
            FlowScript.report(token.line, " at '" + token.lexeme + "'", message)

    @staticmethod
    def runtime_error(error: runtimeError):
        print(error.message)
        FlowScript.had_runtime_error = True


    def report(line: int, where: str, message: str):
        print(f"[line {line}] Error{where}: {message}")
        FlowScript.had_error = True


if __name__ == "__main__":
    FlowScript.main()