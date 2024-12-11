# verify_results.py
import sys

def main():
    input_dict = {}
    with open("input.txt", "r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            key_str, id_str = line.split()
            key = int(key_str)
            vid = int(id_str)
            if key not in input_dict:
                input_dict[key] = []
            input_dict[key].append(vid)

    for k in input_dict:
        input_dict[k].sort()

    output_dict = {}
    with open("output.txt", "r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            parts = line.split(":")
            key = int(parts[0])
            if len(parts) > 1 and parts[1]:
                ids_str = parts[1].split(",")
                ids = list(map(int, ids_str))
            else:
                ids = []
            output_dict[key] = sorted(ids)

    # Now compare
    # Check keys in input_dict
    all_keys = set(input_dict.keys()).union(set(output_dict.keys()))

    mismatch = False
    for k in all_keys:
        input_ids = input_dict.get(k, [])
        output_ids = output_dict.get(k, [])
        if input_ids != output_ids:
            mismatch = True
            print(f"Mismatch for key {k}:")
            print(f"  Input dict:  {input_ids}")
            print(f"  B+ tree out: {output_ids}")

    if not mismatch:
        print("All keys match! The B+ tree implementation is correct.")

if __name__ == "__main__":
    main()
