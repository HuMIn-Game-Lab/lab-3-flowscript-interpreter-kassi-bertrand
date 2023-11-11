# This script is a tool, and NOT part of the interpreter.
import os
import argparse
from typing import List

def define_visitor(writer, base_name, types):
    writer.write(f"class Visitor(ABC):\n")

    for type_str in types:
        type_name = type_str.split(":")[0].strip()
        writer.write(f"\t@abstractmethod\n")
        writer.write(f"\tdef visit_{type_name.lower()}_{base_name.lower()}(self, {base_name.lower()}):\n")
        writer.write(f"\t\tpass\n\n")

def define_base(writer, base_name):
        writer.write(f"class {base_name}(ABC):\n")
        writer.write("\t@abstractmethod\n")
        writer.write("\tdef accept(self, visitor: Visitor):\n")
        writer.write(f"\t\tpass\n")

def define_type(writer, base_name, class_name, fields):
        writer.write(f"class {class_name}({base_name}):\n")
        #constructor_args = [field.split(" ") for field in fields.split(", ")]
        #constructor_args = ', '.join(f'{name}: {type}' for name, type in constructor_args)

        field_list = [field.split(" ")[0] for field in fields.split(", ")]
        
        writer.write(f"\tdef __init__(self, {fields}):\n")
        field_list = [field.split(" ")[0] for field in fields.split(", ")]
        
        for field in field_list:
            writer.write(f"\t\tself.{field} = {field}\n")
        writer.write("\t\n")

        writer.write("\tdef accept(self, visitor: Visitor):\n")
        writer.write(f"\t\treturn visitor.visit_{class_name.lower()}_{base_name.lower()}(self)\n\n")

def generate_ast(output_dir: str, base_name: str, types: List[str]):
    path = os.path.join(output_dir, f"{base_name}.py")

    with open(path, 'w') as writer:
        writer.write("from abc import ABC, abstractmethod\n")
        writer.write("from typing import List\n")
        writer.write("\n")
        define_visitor(writer, base_name, types)
        writer.write("\n")
        define_base(writer, base_name)
        writer.write("\n")

        for type_str in types:
            class_name = type_str.split(":")[0].strip()
            fields = type_str.split(":")[1].strip()
            define_type(writer, base_name, class_name, fields)
        

def main():
    parser = argparse.ArgumentParser(description="Generate files containing operation-less classes who represent nodes in the AST.")
    parser.add_argument("output_dir", help="Output directory for generated files.")
    args = parser.parse_args()

    generate_ast(args.output_dir, "Expr", [
        "Assign   : name, value",
        "Literal  : value",
        "Variable : name"
    ])

    generate_ast(args.output_dir, "Stmt", [
        "Block          : statements",
        "Expression     : expression",
        "Function       : name, statements",
        "Var            : name, expr",
        "JobDeclaration : job_identifier, job_type, input",
        "ConditionalJob : job_identifier, if_true_job_id, else_job_id",
        "Dependency     : dependencies"
    ])


if __name__ == "__main__":
    main()


