import argparse
import numpy as np
from collections import defaultdict

def load_input(input_file):
    """
    Loads input vectors from input.txt.
    Returns a dictionary mapping ID to vector (numpy array).
    """
    id_to_vector = {}
    with open(input_file, 'r') as f:
        for line_num, line in enumerate(f, 1):
            if not line.strip():
                continue
            parts = line.strip().split()
            if len(parts) != 102:
                print(f"Warning: Line {line_num} in '{input_file}' does not have 102 elements.")
                continue
            id_float = float(parts[0])
            id_int = int(id_float)  # Assuming IDs are unique integers stored as floats
            vector = np.array([float(x) for x in parts[1:101]], dtype=np.float32)
            c_value = float(parts[101])
            id_to_vector[id_int] = (vector, c_value)
            if line_num % 100000 == 0:
                print(f"Loaded {line_num} vectors.")
    print(f"Total vectors loaded: {len(id_to_vector)}")
    return id_to_vector

def load_queries(queries_file):
    """
    Loads queries from queries.txt.
    Returns a list of tuples: (query_id, query_vector, c_min, c_max)
    """
    queries = []
    with open(queries_file, 'r') as f:
        for line_num, line in enumerate(f, 1):
            if not line.strip():
                continue
            parts = line.strip().split()
            if len(parts) != 103:
                print(f"Warning: Line {line_num} in '{queries_file}' does not have 103 elements.")
                continue
            query_id_float = float(parts[0])
            query_id = int(query_id_float)  # Assuming query IDs are unique integers stored as floats
            query_vector = np.array([float(x) for x in parts[1:101]], dtype=np.float32)
            c_min = float(parts[101])
            c_max = float(parts[102])
            queries.append((query_id, query_vector, c_min, c_max))
            if line_num % 10000 == 0:
                print(f"Loaded {line_num} queries.")
    print(f"Total queries loaded: {len(queries)}")
    return queries

def load_output(output_file, k):
    """
    Loads output IDs from output.txt.
    Returns a list of lists, each containing k IDs.
    """
    output_ids = []
    with open(output_file, 'r') as f:
        for line_num, line in enumerate(f, 1):
            if not line.strip():
                output_ids.append([0] * k)  # Assuming 0 indicates no match
                continue
            id_strs = line.strip().split()
            try:
                ids = [int(float(id_str)) for id_str in id_strs[:k]]
                # Pad with 0s if fewer than k IDs are present
                while len(ids) < k:
                    ids.append(0)
                output_ids.append(ids)
            except ValueError:
                print(f"Warning: Line {line_num} in '{output_file}' has invalid IDs.")
                output_ids.append([0] * k)
            if line_num % 100000 == 0:
                print(f"Loaded {line_num} output ID lists.")
    print(f"Total output ID lists loaded: {len(output_ids)}")
    return output_ids

def compute_recall(id_to_vector, queries, output_ids, k, truth_save_path=None):
    """
    Computes recall by comparing output_ids against true top-k closest IDs.
    Saves the computed truth data to a file if truth_save_path is provided.
    Returns recall as a float.
    """
    correct = 0
    total = 0
    truth_data = []  # To store the computed truth for each query

    for idx, (query_id, query_vec, c_min, c_max) in enumerate(queries):
        # Find candidate IDs within [c_min, c_max]
        candidates = [id_ for id_, (vec, c_val) in id_to_vector.items() if c_min <= c_val <= c_max]
        if not candidates:
            # No candidates found; expect output to contain all 0s
            expected_ids = [0] * k
        else:
            # Compute Euclidean distances
            vectors = np.array([id_to_vector[id_][0] for id_ in candidates])
            diffs = vectors - query_vec
            dists = np.linalg.norm(diffs, axis=1)
            # Get indices of the top-k smallest distances
            if len(dists) < k:
                topk_indices = np.argsort(dists)[:len(dists)]
            else:
                topk_indices = np.argpartition(dists, k-1)[:k]
                topk_indices = topk_indices[np.argsort(dists[topk_indices])]
            expected_ids = [candidates[i] for i in topk_indices]
            # Pad with 0s if fewer than k neighbors are found
            while len(expected_ids) < k:
                expected_ids.append(0)

        # Save the computed truth (expected IDs)
        truth_data.append((query_id, expected_ids))

        # Get program's output IDs for this query
        if idx >= len(output_ids):
            print(f"Warning: Not enough output ID lists for query {idx + 1}.")
            program_ids = [0] * k
        else:
            program_ids = output_ids[idx]

        # Compare the expected_ids and program_ids
        # Recall@k counts how many expected IDs are present in program_ids
        hits = set(expected_ids) & set(program_ids)
        correct += len(hits)
        total += k

        if (idx + 1) % 10000 == 0:
            print(f"Processed {idx + 1} queries.")

    # Save truth data to file if a path is provided
    if truth_save_path:
        with open(truth_save_path, 'w') as f:
            for query_id, expected_ids in truth_data:
                expected_ids_str = ' '.join(map(str, expected_ids))
                f.write(f"{query_id} {expected_ids_str}\n")
        print(f"Truth data written to: {truth_save_path}")

    recall = correct / total if total > 0 else 0.0
    return recall

def main(input_file, queries_file, output_file, k, truth_save_path="truth.txt"):
    print("Loading input vectors...")
    id_to_vector = load_input(input_file)
    
    print("Loading queries...")
    queries = load_queries(queries_file)
    
    print("Loading program output...")
    output_ids = load_output(output_file, k)
    
    print("Computing recall and saving truth data...")
    recall = compute_recall(id_to_vector, queries, output_ids, k, truth_save_path)
    
    print(f"Recall@{k}: {recall * 100:.2f}%")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Verify C++ program output and compute recall for top-k neighbors.")
    parser.add_argument('--input', type=str, required=True, help='Path to input.txt.')
    parser.add_argument('--queries', type=str, required=True, help='Path to queries.txt.')
    parser.add_argument('--output', type=str, required=True, help='Path to output.txt.')
    parser.add_argument('--k', type=int, required=True, help='Number of nearest neighbors (k).')
    parser.add_argument('--truth', type=str, default="truth.txt", help='Path to save truth data.')

    args = parser.parse_args()

    if args.k <= 0:
        print("Error: k must be a positive integer.")
        exit(1)

    main(args.input, args.queries, args.output, args.k, args.truth)