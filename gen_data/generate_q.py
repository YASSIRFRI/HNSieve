# generate_queries.py
import argparse
import random
import numpy as np

def generate_queries(num_queries, output_file, c_min_max_range=(0, 100)):
    with open(output_file, 'w') as f:
        for i in range(1, num_queries + 1):
            query_id = i 
            query_d = np.random.rand(100).astype(np.float32)  # 100 dims
            c_min = random.uniform(*c_min_max_range)
            c_max = random.uniform(c_min, c_min_max_range[1])  # Ensure c_max >= c_min
            query_vector = np.zeros(102, dtype=np.float32)
            query_vector[0] = i
            query_vector[1:101] = query_d
            query_vector[101] = c_min  
            # Combine all elements for the line
            line_elements = query_vector.tolist()
            line_elements[0]=query_id
            line_elements.append(c_max)  # Append c_max at the end
            #print(len(line_elements))
            line = ' '.join(map(str, line_elements))
            f.write(line + '\n')
            if i % 10000 == 0:
                print(f"Generated {i} queries.")

    print(f"Finished generating {num_queries} queries in '{output_file}'.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate query vectors for C++ program.")
    parser.add_argument('--num_queries', type=int, default=10000, help='Number of queries to generate.')
    parser.add_argument('--output', type=str, default='queries.txt', help='Output file name.')
    parser.add_argument('--c_min_max', type=float, nargs=2, default=(0, 100), help='Range for c_min and c_max.')

    args = parser.parse_args()
    
    generate_queries(args.num_queries, args.output, tuple(args.c_min_max))
