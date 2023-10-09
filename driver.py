import signal
import subprocess

import numpy as np
from PIL import Image

from shared_data import InputData, SharedData, Thread


def main():
    image_width = 256
    debug_store_images = False
    debug_store_absolute_errors = False
    radius_in_micrometers = 300_000
    background_color = np.array([0, 0, 0], dtype=np.uint8)
    point_amount = 4
    terminate_on_min_relative_error = False
    terminate_on_unavailable_connection = False
    max_iterations = 8
    min_relative_error = 0.0
    relative_error_streak = 0
    threads = [
        Thread(255, 2000, np.array([255, 0, 0], dtype=np.uint8)),
        Thread(255, 2000, np.array([0, 255, 0], dtype=np.uint8)),
        Thread(255, 2000, np.array([0, 0, 255], dtype=np.uint8)),
        Thread(255, 2000, np.array([255, 255, 255], dtype=np.uint8))
    ]
    thread_order = [0, 1, 2, 3]
    start_points = [0, 0, 0, 0]
    target_image = Image.open("images/input.png").resize(
        (image_width, image_width)
    )
    target = 255 - np.array(target_image)
    importance = np.ones([image_width, image_width], dtype=np.float64)

    input_data = InputData(
        image_width,
        debug_store_images,
        debug_store_absolute_errors,
        radius_in_micrometers,
        background_color,
        point_amount,
        terminate_on_min_relative_error,
        terminate_on_unavailable_connection,
        max_iterations,
        min_relative_error,
        relative_error_streak,
        threads,
        thread_order,
        start_points,
        target,
        importance
    )
    shared_data = SharedData(input_data)
    output_data = shared_data.output_data
    key, size = shared_data.share()
    print("Running C program...")
    print(f"./main {key} {size}")
    input()
    process = subprocess.run(["./main", str(key), str(size)])
    print("C program finished.")
    return_code = process.returncode
    if return_code != 0:
        print("Error: " + str(return_code))
        if return_code < 0:
            signal_name = signal.Signals(-return_code).name
            print(f"Failed due to signal: {signal_name}")
        return
    shared_data.read()
    instructions = output_data.instructions
    print([str(i) for i in instructions])
    result = 255 - output_data.result
    result_image = Image.fromarray(result)
    result_image.save("images/result.png")
    pass


if __name__ == "__main__":
    main()
