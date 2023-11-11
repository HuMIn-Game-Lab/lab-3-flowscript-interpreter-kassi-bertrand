from fsToken import Token

class RuntimeErrorWithToken(RuntimeError):
    def __init__(self, token: Token, *args: object) -> None:
        super().__init__(*args)
        self.token = token