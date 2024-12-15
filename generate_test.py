import random

def main():
    # Adjust these parameters as desired
    num_entries = 1000
    key_range = 500  # Unique keys
    Dimension = 128  # Must match C++ Dimension

    # Ensure unique keys
    keys = random.sample(range(1, key_range + 1), min(num_entries, key_range))

    with open("input.txt", "w") as f:
        for key in keys:
            # Generate a fixed-size vector with random floats
            vector = [round(random.uniform(0.0, 100.0), 6) for _ in range(Dimension)]
            vector_str = ' '.join(map(str, vector))
            f.write(f"{key} {vector_str}\n")

    print("Generated input.txt with test data.")

if __name__ == "__main__":
    main()
