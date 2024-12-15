import sys
import bisect

def read_input_vectors(file_path):
    c_values = []
    with open(file_path, 'r') as f:
        for line_num, line in enumerate(f, 1):
            parts = line.strip().split()
            if len(parts) != 102:
                print(f"Warning: Line {line_num} in '{file_path}' has {len(parts)} fields instead of 102.")
                continue
            try:
                c_value = float(parts[101])
                c_values.append(c_value)
            except ValueError:
                print(f"Error: Non-float c_value on line {line_num} in '{file_path}'.")
                continue
    return c_values

def read_queries(file_path):
    queries = []
    with open(file_path, 'r') as f:
        for line_num, line in enumerate(f, 1):
            parts = line.strip().split()
            if len(parts) != 103:
                print(f"Warning: Line {line_num} in '{file_path}' has {len(parts)} fields instead of 103.")
                continue
            try:
                c_min = float(parts[101])
                c_max = float(parts[102])
                queries.append((c_min, c_max))
            except ValueError:
                print(f"Error: Non-float c_min/c_max on line {line_num} in '{file_path}'.")
                continue
    return queries

def read_output_counts(file_path):
    counts = []
    with open(file_path, 'r') as f:
        for line_num, line in enumerate(f, 1):
            try:
                count = int(line.strip())
                counts.append(count)
            except ValueError:
                print(f"Error: Non-integer count on line {line_num} in '{file_path}'.")
                counts.append(-1)
    return counts

def perform_range_queries(c_values, queries):
    sorted_c = sorted(c_values)
    results = []
    for idx, (c_min, c_max) in enumerate(queries, 1):
        left = bisect.bisect_left(sorted_c, c_min)
        right = bisect.bisect_right(sorted_c, c_max)
        count = right - left
        results.append(count)
    return results

def main():
    if len(sys.argv) != 4:
        print("Usage: python3 test_output.py <input.txt> <queries.txt> <output.txt>")
        sys.exit(1)

    input_file = sys.argv[1]
    queries_file = sys.argv[2]
    output_file = sys.argv[3]

    print("Reading input vectors...")
    c_values = read_input_vectors(input_file)
    print(f"Total vectors read: {len(c_values)}")

    print("Reading queries...")
    queries = read_queries(queries_file)
    print(f"Total queries read: {len(queries)}")

    print("Reading output counts...")
    output_counts = read_output_counts(output_file)
    print(f"Total output counts read: {len(output_counts)}")

    if len(queries) != len(output_counts):
        print(f"Error: Number of queries ({len(queries)}) does not match number of output counts ({len(output_counts)}).")
        sys.exit(1)

    print("Performing range queries in Python...")
    expected_counts = perform_range_queries(c_values, queries)

    print("Comparing results...")
    all_match = True
    for i, (expected, actual) in enumerate(zip(expected_counts, output_counts), 1):
        if expected != actual:
            print(f"Mismatch in query {i}: Expected {expected}, Got {actual}")
            all_match = False

    if all_match:
        print("All counts match! Test passed.")
    else:
        print("Some counts do not match. Test failed.")

if __name__ == "__main__":
    main()
