from __future__ import annotations

import ctypes
import struct
from typing import Any, List, Tuple

import numpy as np
import sysv_ipc

SIZEOF_UINT8_T: int = 1
SIZEOF_UINT64_T: int = 8
SIZEOF_DOUBLE: int = ctypes.sizeof(ctypes.c_double)
SIZEOF_COLOR: int = 3 * SIZEOF_UINT8_T

DEBUG_STORE_IMAGES: int = 0b00000001
DEBUG_STORE_ABSOLUTE_ERRORS: int = 0b00000010

TERMINATE_ON_MIN_RELATIVE_ERROR: int = 0b00000001
TERMINATE_ON_UNAVAILABLE_CONNECTION: int = 0b00000010


class Thread:
    _alpha: int
    _thickness_in_micrometers: int
    _color: np.array

    def __init__(
        self, alpha: int, thickness_in_micrometers: int, color: np.array
    ):
        self._alpha = alpha
        self._thickness_in_micrometers = thickness_in_micrometers
        self._color = color

    @property
    def alpha(self) -> int:
        return self._alpha

    @property
    def thickness_in_micrometers(self) -> int:
        return self._thickness_in_micrometers

    @property
    def color(self) -> np.array:
        return self._color

    def __str__(self) -> str:
        return f"Thread({self._alpha}, {self._thickness_in_micrometers}, " \
               f"{self._color})"


class InputData:
    NULL_BYTE: bytes = b"\x00"

    _image_width: int
    _thread_order_size: int
    _debug_flags: int
    _radius_in_micrometers: int
    _background_color: np.array
    _point_amount: int
    _thread_amount: int
    _termination_flags: int
    _max_iterations: int
    _min_relative_error: float
    _relative_error_streak: int
    _threads: List[Thread]
    _thread_order: List[int]
    _start_points: List[int]
    _target: np.array
    _importance: np.array

    _output_data: OutputData
    _memory_size: int

    def __init__(
        self,
        image_width: int,
        debug_store_images: bool,
        debug_store_absolute_errors: bool,
        radius_in_micrometers: int,
        background_color: np.array,
        point_amount: int,
        terminate_on_min_relative_error: bool,
        terminate_on_unavailable_connection: bool,
        max_iterations: int,
        min_relative_error: float,
        relative_error_streak: int,
        threads: List[Thread],
        thread_order: List[int],
        start_points: List[int],
        target: np.array,
        importance: np.array,
    ):
        self._image_width = image_width
        self._thread_order_size = len(thread_order)
        self._debug_flags = (
            (
                DEBUG_STORE_IMAGES
                if debug_store_images else 0
            )
            | (
                DEBUG_STORE_ABSOLUTE_ERRORS
                if debug_store_absolute_errors else 0
            )
        )
        self._radius_in_micrometers = radius_in_micrometers
        self._background_color = background_color
        self._point_amount = point_amount
        self._thread_amount = len(threads)
        self._termination_flags = (
            (
                TERMINATE_ON_MIN_RELATIVE_ERROR
                if terminate_on_min_relative_error else 0
            )
            | (
                TERMINATE_ON_UNAVAILABLE_CONNECTION
                if terminate_on_unavailable_connection else 0
            )
        )
        self._max_iterations = max_iterations
        self._min_relative_error = min_relative_error
        self._relative_error_streak = relative_error_streak
        self._threads = threads
        self._thread_order = thread_order
        self._start_points = start_points
        self._target = target
        self._importance = importance

        self._output_data = OutputData(self)
        self._memory_size = None

    @property
    def memory_size(self) -> int:
        return (
            SIZEOF_UINT64_T + SIZEOF_UINT64_T + SIZEOF_UINT8_T
            + SIZEOF_UINT64_T + SIZEOF_COLOR + SIZEOF_UINT64_T
            + SIZEOF_UINT64_T + SIZEOF_UINT8_T + SIZEOF_UINT64_T
            + SIZEOF_DOUBLE + SIZEOF_UINT64_T
            + self._thread_amount * (
                SIZEOF_UINT64_T + SIZEOF_UINT64_T + SIZEOF_COLOR
            )
            + self._thread_order_size * SIZEOF_UINT64_T
            + self._thread_amount * SIZEOF_UINT64_T
            + self._target.nbytes
            + self._importance.nbytes
        )

    @property
    def output_data(self) -> OutputData:
        return self._output_data

    def _pack(
        self, format_: str, buffer: bytearray, offset: int, value: Any
    ) -> int:
        struct.pack_into(format_, buffer, offset, value)
        return offset + struct.calcsize(format_)

    def pack_into(self, buffer: bytearray):
        offset = 0
        offset = self._pack("Q", buffer, offset, self._image_width)
        offset = self._pack("Q", buffer, offset, self._thread_order_size)
        offset = self._pack("B", buffer, offset, self._debug_flags)
        offset = self._pack("Q", buffer, offset, self._radius_in_micrometers)
        offset = self._pack(
            "3s", buffer, offset, self._background_color.tobytes()
        )
        offset = self._pack("Q", buffer, offset, self._point_amount)
        offset = self._pack("Q", buffer, offset, self._thread_amount)
        offset = self._pack("B", buffer, offset, self._termination_flags)
        offset = self._pack("Q", buffer, offset, self._max_iterations)
        offset = self._pack("d", buffer, offset, self._min_relative_error)
        offset = self._pack("Q", buffer, offset, self._relative_error_streak)
        for thread in self._threads:
            offset = self._pack("B", buffer, offset, thread._alpha)
            offset = self._pack(
                "Q", buffer, offset, thread._thickness_in_micrometers
            )
            offset = self._pack("3s", buffer, offset, thread._color.tobytes())
        for thread_id in self._thread_order:
            offset = self._pack("Q", buffer, offset, thread_id)
        for start_point in self._start_points:
            offset = self._pack("Q", buffer, offset, start_point)
        offset = self._pack(
            f"{self._target.size}s", buffer, offset, self._target.tobytes()
        )
        offset = self._pack(
            f"{self._importance.size}s", buffer, offset,
            self._importance.tobytes()
        )
        self._memory_size = offset
        output_data_size = self._output_data.memory_size
        self._pack(
            f"{output_data_size}s", buffer, offset,
            self.NULL_BYTE * output_data_size
        )


class Instruction:
    SIZE: int = 3 * SIZEOF_UINT64_T
    _start_index: int
    _end_index: int
    _thread_index: int

    def __init__(self, start_index: int, end_index: int, thread_index: int):
        self._start_index = start_index
        self._end_index = end_index
        self._thread_index = thread_index

    @property
    def start_index(self) -> int:
        return self._start_index

    @property
    def end_index(self) -> int:
        return self._end_index

    @property
    def thread_index(self) -> int:
        return self._thread_index

    def __str__(self) -> str:
        return f"Instruction({self._start_index}, {self._end_index}, " \
               f"{self._thread_index})"


class OutputData:
    _image_width: int
    _max_iterations: int
    _debug_flags: int
    _input_data: InputData

    _instruction_amount: int
    _absolute_error: int
    _normalized_error: float
    _result: np.array
    _instructions: List[Instruction]
    _debug_images: List[np.array]
    _debug_absolute_errors: List[np.array]

    def __init__(self, input_data: InputData):
        self._image_width = input_data._image_width
        self._max_iterations = input_data._max_iterations
        self._debug_flags = input_data._debug_flags
        self._input_data = input_data

    @property
    def memory_size(self) -> int:
        size = (
            SIZEOF_UINT64_T + SIZEOF_UINT64_T + SIZEOF_DOUBLE
            + self._image_width ** 2 * SIZEOF_COLOR
            + self._max_iterations * Instruction.SIZE
        )
        if self._debug_flags:
            size += (
                self._max_iterations * self._image_width ** 2
                * SIZEOF_COLOR
                + self._max_iterations * self._image_width ** 2
                * SIZEOF_UINT64_T
            )
        return size

    @property
    def debug_flags(self) -> int:
        return self._debug_flags

    @property
    def instruction_amount(self) -> int:
        return self._instruction_amount

    @property
    def absolute_error(self) -> int:
        return self._absolute_error

    @property
    def normalized_error(self) -> float:
        return self._normalized_error

    @property
    def result(self) -> np.array:
        return self._result

    @property
    def instructions(self) -> List[Instruction]:
        return self._instructions

    @property
    def debug_images(self) -> List[np.array]:
        return self._debug_images

    @property
    def debug_absolute_errors(self) -> List[np.array]:
        return self._debug_absolute_errors

    def _unpack(self, format_: str, buffer: bytearray, offset: int) -> int:
        value = struct.unpack_from(format_, buffer, offset)
        return offset + struct.calcsize(format_), *value

    def _unpack_array(
        self,
        buffer: bytearray,
        dtype: np.dtype,
        count: int,
        offset: int,
        shape: Tuple[int, ...]
    ) -> Tuple[int, np.array]:
        value = np.frombuffer(
            buffer, dtype=dtype, count=count, offset=offset
        ).reshape(shape)
        return offset + value.nbytes, value

    def unpack_from(self, buffer: bytearray):
        offset = self._input_data.memory_size

        offset, self._instruction_amount = self._unpack("Q", buffer, offset)
        offset, self._absolute_error = self._unpack("Q", buffer, offset)
        offset, self._normalized_error = self._unpack("d", buffer, offset)
        offset, self._result = self._unpack_array(
            buffer, np.uint8, self._image_width ** 2 * SIZEOF_COLOR, offset,
            (self._image_width, self._image_width, SIZEOF_COLOR)
        )
        self._instructions = []
        for _ in range(self._max_iterations):
            offset, start_index, end_index, thread_index = self._unpack(
                "QQQ", buffer, offset
            )
            self._instructions.append(
                Instruction(start_index, end_index, thread_index)
            )

        if self._debug_flags & DEBUG_STORE_IMAGES:
            self._debug_images = []
            for _ in range(self._max_iterations):
                offset, array = self._unpack_array(
                    buffer, np.uint8, self._image_width ** 2 * SIZEOF_COLOR,
                    offset,
                    (self._image_width, self._image_width, SIZEOF_COLOR)
                )
                self._debug_images.append(array)

        if self._debug_flags & DEBUG_STORE_ABSOLUTE_ERRORS:
            self._debug_absolute_errors = []
            for _ in range(self._max_iterations):
                offset, array = self._unpack_array(
                    buffer, np.uint64, self._image_width ** 2, offset,
                    (self._image_width, self._image_width)
                )
                self._debug_absolute_errors.append(array)


class SharedData:
    SHARE_MODE: int = int("666", 8)

    _input_data: InputData
    _output_data: OutputData
    _memory: sysv_ipc.SharedMemory

    def __init__(self, input_data: InputData):
        self._input_data = input_data
        self._output_data = input_data.output_data
        self._memory = sysv_ipc.SharedMemory(
            None,
            flags=sysv_ipc.IPC_CREX,
            size=self._input_data.memory_size + self._output_data.memory_size,
            mode=self.SHARE_MODE
        )

    def __del__(self):
        self._memory.remove()

    def share(self) -> Tuple[int, int]:
        buffer = bytearray(
            self._input_data.memory_size + self._output_data.memory_size
        )
        self._input_data.pack_into(buffer)
        self._memory.write(buffer)
        return self._memory.key, self._memory.size

    def read(self):
        buffer = bytearray(self._memory.read())
        self._output_data.unpack_from(buffer)

    @property
    def output_data(self) -> OutputData:
        return self._output_data
