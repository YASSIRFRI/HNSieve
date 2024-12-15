# generate_input.py
import argparse
import random
import numpy as np

def generate_input(num_vectors, output_file):
    with open(output_file, 'w') as f:
        for i in range(1, num_vectors + 1):
            id_float = float(i)  # Unique ID as float
            vector = np.random.rand(102).astype(np.float32)  # 100 dims + ID + c_value
            vector[0] = id_float  # Store ID in the first position
            vector[101] = random.uniform(0, 100)  # c_value between 0 and 100
            line = ' '.join(map(str, vector))
            f.write(line + '\n')
            if i % 100000 == 0:
                print(f"Generated {i} vectors.")

    print(f"Finished generating {num_vectors} vectors in '{output_file}'.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate input vectors for C++ program.")
    parser.add_argument('--num_vectors', type=int, default=1000000, help='Number of vectors to generate.')
    parser.add_argument('--output', type=str, default='input.txt', help='Output file name.')
    
    args = parser.parse_args()
    
    generate_input(args.num_vectors, args.output)
