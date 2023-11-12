from fsToken import Token

class runtimeError(RuntimeError):
    def __init__(self, token: Token, message: str, *args: object) -> None:
        super().__init__(*args)
        self.token = token
        self.message = message