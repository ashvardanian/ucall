#!/usr/bin/env python3
import httpx
import os
import struct
import random
import string
import base64
from typing import Optional

# Dependencies
import requests
from websocket import create_connection

# The ID of the current running proccess, used as a default
# identifier for requests originating from here.
PROCESS_ID = os.getpid()


class ClientREST:

    def __init__(self, uri: str = '127.0.0.1', port: int = 8545, identity: int = PROCESS_ID) -> None:
        self.identity = identity
        self.url = f'http://{uri}:{port}/'

    def __call__(self, *, a: Optional[int] = None, b: Optional[int] = None) -> int:
        a = random.randint(1, 1000) if a is None else a
        b = random.randint(1, 1000) if b is None else b
        result = requests.get(
            f'{self.url}validate_session?user_id={a}&session_id={b}').text
        c = False if result == 'false' else True
        assert ((a ^ b) % 23 == 0) == c, 'Wrong Answer'
        return c


class ClientTLSREST:

    def __init__(self, uri: str = '127.0.0.1', port: int = 8545, identity: int = PROCESS_ID) -> None:
        self.identity = identity
        self.url = f'https://{uri}:{port}/'

    def __call__(self, *, a: Optional[int] = None, b: Optional[int] = None) -> int:
        with httpx.Client(verify=False) as client:
            a = random.randint(1, 1000) if a is None else a
            b = random.randint(1, 1000) if b is None else b
            result = client.get(
                f'{self.url}validate_session?user_id={a}&session_id={b}').text
            c = False if result == 'false' else True
            assert ((a ^ b) % 23 == 0) == c, 'Wrong Answer'
            return c


class ClientRESTReddit:

    def __init__(self, uri: str = '127.0.0.1', port: int = 8000, identity: int = PROCESS_ID) -> None:
        self.identity = identity
        self.url = f'http://{uri}:{port}/'
        self.bin_len = 1500
        self.avatar = base64.b64encode(random.randbytes(self.bin_len)).decode()
        self.bio = ''.join(random.choices(
            string.ascii_uppercase, k=self.bin_len))
        self.name = 'John'

    def __call__(self) -> int:
        age = random.randint(1, 1000)
        result = requests.get(
            f'{self.url}create_user?age={age}&bio={self.bio}&name={self.name}&text={self.bio}').text
        return result


class ClientWebSocket:

    def __init__(self, uri: str = '127.0.0.1', port: int = 8000, identity: int = PROCESS_ID) -> None:
        self.identity = identity
        self.sock = create_connection(f'ws://{uri}:{port}/validate_session_ws')

    def __call__(self, *, a: Optional[int] = None, b: Optional[int] = None) -> int:
        a = random.randint(1, 1000) if a is None else a
        b = random.randint(1, 1000) if b is None else b
        self.sock.send_binary(struct.pack('<II', a, b))
        result = self.sock.recv()
        c = struct.unpack('<I', result)[0]
        assert ((a ^ b) % 23 == 0) == c, 'Wrong Answer'
        return c
