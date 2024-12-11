# generate_test.py
import random

def main():
    # Adjust these parameters as desired
    num_entries = 1000
    key_range = 500
    id_range = 10_000

    with open("input.txt", "w") as f:
        for i in range(num_entries):
            key = i
            vid = random.randint(0, id_range)
            f.write(f"{key} {vid}\n")

    print("Generated input.txt with test data.")

if __name__ == "__main__":
    main()
