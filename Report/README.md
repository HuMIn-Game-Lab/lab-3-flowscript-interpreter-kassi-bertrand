# Kassi - Program 3 Report

## Updates

### BNF Rules

In this iteration, I updated the BNF rules of `FlowScript`. The goal was to make them simpler and shorter. Below are the following changes that I made.

#### Removed the `Start` and `End` Nodes 

In previous iteration of the BNF rules, I said that all FlowScript programs should start at `Start` node and end at a `End` node.

```dot
digraph FlowScript {
    Start [shape=Mdiamond];
    End [shape=Msquare];
}
```

In this iteration, I am ditching this rule. This rule was introduced purely for aesthetic reasons, no practically purposes. The result of this action is that this removes the following types of statements from `FlowScript`'s BNF rules:

- `<StartPoint>`
- `<EndPoint>`

#### Removed clutter in the `FunctionDeclaration` rule

I decided to ditch the color and label attribute when declaring a subgraph (i.e. a function) in `FlowScript`. In the previous iteration of the BNF rules, the `<FunctionDeclaration>` production looked like this:

```bnf
<Function> ::= "subgraph" <VariableIdentifier> "{" 
                  "node" "[" "style=filled" "]" ";" 
                  "edge" "[" "style=dashed" "]" ";" 
                  (<JobDeclaration> | <Dependency>)* 
                  "label" "=" <String> ";" 
                  "color" "=" "blue" ";" 
              "}"
```
Now, it looks like this:

```bnf
<FunctionDeclaration> ::= "subgraph" <IDENTIFIER> "{" 
                      (<Declaration>)* 
                  "}"
```

It's simpler, and makes it easier to parse. Also removing those statements does not make it harder for the user to distinguish between standalone jobs and "functions", which are just job chains. Jobs feeding on each other's output.

**NOTE: I renamed the rule, and chose a name I thought was more descriptive**.

#### Updated the `dependency` production

As a recall, this rule is used to express dependencies between jobs the user have created. In the previous iteration of the BNF rules, the `<Dependency>` production look like this:

```bnf
<Dependency> ::= <VariableIdentifier> "->" <VariableIdentifier> ";"
```

Now, it looks like this:

```bnf
<Dependency> ::= <IDENTIFIER> "->" <IDENTIFIER> ( "->" <IDENTIFIER> )* ";"
```

The updated version of the rule is slightly more complex, but is more robust. This version handles dependency chains (involving more than two jobs) like:

```txt
A -> B -> C -> D -> E;
```
which was not possible in the previous version of the rule.

#### Introduced a `primary` production

#### Made the distinction between a `Declaration` and a `Statement`

With all the previously mentioned updates, the BNF rules for `FlowScript` look like this:

```bnf
<FlowScript> ::= "digraph" "FlowScript" "{" <Declaration>* EOF "}"

<Declaration> ::= <VarDeclaration> | <FunctionDeclaration> | <JobDeclaration> | <Statement>

<Statement> ::= <Dependency> | <Conditional> | <Comment>

<JobDeclaration> ::= <IDENTIFIER> "[" "jobType" "=" <STRING> "shape" "=" "circle" "input" "=" <IDENTIFIER> "]" ";"

<VariableDeclaration> ::= <IDENTIFIER> "=" <Primary> | <IDENTIFIER> ";"

<FunctionDeclaration> ::= "subgraph" <IDENTIFIER> "{" 
                      (<Declaration>)* 
                  "}"

<Dependency> ::= <IDENTIFIER> "->" <IDENTIFIER> ( "->" <IDENTIFIER> )* ";"

<Conditional> ::= <IDENTIFIER> "[" "jobType" "=" "logical_conditional" "testType" "=" <String> "if_true" "=" <IDENTIFIER> "else" "=" <IDENTIFIER> "label" "=" <String> "shape" "=" "diamond" "]" ";"

<Block> ::= "{" <Declaration>* "}"

<Primary> ::= <NUMBER> | <STRING> | <IDENTIFIER> | "nil"

<IDENTIFIER> ::= (letter | "_") (letter | digit | "_")*

<Comment> ::= "//" any sequence of characters until end of line

<String> ::= any sequence of characters WITHIN double quotes
```

## Overview of the `FlowScript` interpreter

This is the meat 🍗🥩 of the report. I will try best to be as thorough as possible. I will go through the compilation process and explain how each part was achieved.

**NOTE: The source code of the interpreter is located in the `Code/fs_interpreter` folder**.

For this lab, the interpreter was written as a standalone Python script, and the entry point of the program is the `flowscript.py` file. In the file, the function that kicks of the interpreter itself is the run function, and it looks like this:

```py
def run(source: str):
    lexer = scanner.Scanner(source)
    tokens = lexer.scan_tokens()

    parser = fsParser.Parser(tokens)
    statements = parser.parse()

    if FlowScript.had_error:
        return
    
    if len(statements) == 0:
        return
    
    # for token in tokens:
    #     print(token)

    # At this point, the parser have IDENTIFIED All TYPE of statements the user have typed,
    # Now, it is the responsibility of the interpreter to 'execute' those statements.
    interpreter = Interpreter()
    interpreter.interpret(statements)
```

Let's go over each step together.

### Scanning

The first step in the compilation process of a `FlowScript` code is **scanning**, and the part of the interpreter in charge of "scanning 👀" the source code is the **lexer** which is short for lexical analyzer. The goal of the "scanning" step is to turn the source code which is a raw string into a series of **tokens**. The `FlowScript` lexer achieves this by grouping relevant characters together. Simply put, here is a nice illustration showing what the `FlowScript` lexer does.

![lexer illustration](./img/0.png)

Each blob of characters formed by the lexer is called a **lexeme**. In the process of forming those "lexeme" the `FlowScript` lexer also include additional information such as the line number. The lexeme, the line number, plus any other data gathered by the lexer is what is considered a **Token**. For instance, here the definition of a `Token` in `FlowScript`:

```py
class Token:
    def __init__(self, type: TokenType, lexeme: str, literal: object, line: int) -> None:
        self.type = type
        self.lexeme = lexeme
        self.literal = literal
        self.line = line

    def __str__(self) -> str:
        return f"{self.type}, {self.lexeme}, {self.literal}"
```

Each detected token has a:
- **Type** which tells which kind of token it is. Since FlowScript has keywords, we must different between them and identifiers created by the users.
- **Lexeme** which is the raw substring in the source code
- **Literal** which is the actual value of the token. For instance we may encounter a string representing a number. The literal will be numerical representation of the number.
- **Line**: Which is where in the source code this token was detected.

**NOTE**: You can find the different types of tokens FlowScript supports int he `tokentype.py` file.

**NOTE**: The lexer of the `FlowScript` programming language is implemented in the `scanner.py` file, inside the `fs_interpreter` folder, which itself is within the `Code` folder.

### Parsing

After scanning, the source code is no longer is a series of characters, but rather a flat list of tokens. For instance, the following FlowScript code (available in the `Data` folder under the `FlowScript` folder):

```dot
digraph FlowScript {
    compile_input = "{\"jobChannels\": 268435456, \"jobType\": 1, \"makefile\": \"./Data/testCode/Makefile\", \"isFilePath\": true}";
    parsing_input = "{\"jobChannels\": 536870912, \"jobType\": 2, \"content\": \"\"}";

    A[jobType="COMPILE_JOB" shape=circle input=compile_input];
    B[jobType="PARSING_JOB" shape=circle input=parsing_input];

    A -> B;
}
```

is turned into this series of Tokens:

```py
TokenType.DIGRAPH, digraph, None
TokenType.FLOWSCRIPT, FlowScript, None
TokenType.LEFT_BRACE, {, None
TokenType.IDENTIFIER, compile_input, None
TokenType.EQUAL, =, None
TokenType.STRING, "{\"jobChannels\": 268435456, \"jobType\": 1, \"makefile\": \"./Data/testCode/Makefile\", \"isFilePath\": true}", {"jobChannels": 268435456, "jobType": 1, "makefile": "./Data/testCode/Makefile", "isFilePath": true}
TokenType.SEMICOLON, ;, None
TokenType.IDENTIFIER, parsing_input, None
TokenType.EQUAL, =, None
TokenType.STRING, "{\"jobChannels\": 536870912, \"jobType\": 2, \"content\": \"\"}", {"jobChannels": 536870912, "jobType": 2, "content": ""}
TokenType.SEMICOLON, ;, None
TokenType.IDENTIFIER, A, None
TokenType.LEFT_BRACK, [, None
TokenType.JOB_TYPE, jobType, None
TokenType.EQUAL, =, None
TokenType.STRING, "COMPILE_JOB", COMPILE_JOB
TokenType.SHAPE, shape, None
TokenType.EQUAL, =, None
TokenType.CIRCLE, circle, None
TokenType.INPUT, input, None
TokenType.EQUAL, =, None
TokenType.IDENTIFIER, compile_input, None
TokenType.RIGHT_BRACK, ], None
TokenType.SEMICOLON, ;, None
TokenType.IDENTIFIER, B, None
TokenType.LEFT_BRACK, [, None
TokenType.JOB_TYPE, jobType, None
TokenType.EQUAL, =, None
TokenType.STRING, "PARSING_JOB", PARSING_JOB
TokenType.SHAPE, shape, None
TokenType.EQUAL, =, None
TokenType.CIRCLE, circle, None
TokenType.INPUT, input, None
TokenType.EQUAL, =, None
TokenType.IDENTIFIER, parsing_input, None
TokenType.RIGHT_BRACK, ], None
TokenType.SEMICOLON, ;, None
TokenType.IDENTIFIER, A, None
TokenType.ARROW, ->, None
TokenType.IDENTIFIER, B, None
TokenType.SEMICOLON, ;, None
TokenType.RIGHT_BRACE, }, None
TokenType.EOF, , None
```

The parser will now go through all of these tokens, and recognize the different types of statements that one can form in `FlowScript`. The parser will go on and detect statements like.

- Job Declaration
- Dependency relationship between two or more jobs
- Variable declarations
- Variable assignments.

Simply put, the parser implements the **BNF** rules of `FlowScript`.

## Error detection and handling

## Limitations

- As of now, one cannot reuse functions in other `FlowScript` in other parts of the same file, or in other files. However, implementing this functionality should involve minimal additional work considering the foundations that have been lay out during this lab.

- As of now, there is no built-in checks to prevent the users to create a circular dependency.